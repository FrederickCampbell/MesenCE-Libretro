#include "pch.h"
#include "Shared/Emulator.h"
#include "Shared/EmuSettings.h"
#include "Shared/Audio/SoundMixer.h"
#include "Shared/Audio/SoundResampler.h"
#include "Shared/Video/VideoRenderer.h"
#include "Utilities/Audio/HermiteResampler.h"

// MESENCE_HQ_AUDIO_DECIMATOR_V1
// 127-tap symmetric Kaiser-windowed low-pass FIR.
//
// Design:
//   Input:          96,000 Hz stereo
//   Output:         48,000 Hz stereo
//   Passband edge:  approximately 20 kHz
//   Cutoff:         21.5 kHz
//   Stopband:       begins at 24 kHz
//   Arithmetic:     deterministic Q30 fixed point
//   Group delay:    63 input samples, approximately 0.656 ms
//
// Only the first 63 taps and center tap are stored because the kernel is
// symmetric. The Q30 DC gain sums exactly to 1.0.
static constexpr int32_t MesenCeHqHalfCoefficients[64] = {
	4586, -8102, -16073, 10501, 37157, -3363, -67328, -22587,
	100874, 76999, -125077, -166342, 119798, 289240, -59351, -431620,
	-82765, 563079, 326301, -636087, -674681, 589557, 1105136, -357677,
	-1560955, -116046, 1948593, 860925, -2142133, -1860087, 1996686, 3034088,
	-1370812, -4231359, 156208, 5229356, 1689015, -5748992, -4108528, 5482890,
	6923154, -4135360, -9818425, 1469423, 12347240, 2645972, -13946806, -8196486,
	13963262, 14998414, -11668101, -22696219, 6232073, 30784564, 3426234, -38654000,
	-19167379, 45655406, 45551901, -51174821, -99558433, 54708098, 336875739, 480948876
};

static int16_t MesenCeRoundAndClampQ30(int64_t value)
{
	constexpr int64_t half = 1LL << 29;
	int64_t rounded;
	if(value >= 0) {
		rounded = (value + half) >> 30;
	} else {
		rounded = -(((-value) + half) >> 30);
	}
	return (int16_t)std::clamp<int64_t>(rounded, INT16_MIN, INT16_MAX);
}

SoundResampler::SoundResampler(Emulator* emu)
{
	_emu = emu;
}

SoundResampler::~SoundResampler()
{
}

double SoundResampler::GetRateAdjustment()
{
	return _rateAdjustment;
}

uint32_t SoundResampler::GetTargetRate()
{
	return (uint32_t)_previousTargetRate;
}

double SoundResampler::GetTargetRateAdjustment()
{
	AudioConfig cfg = _emu->GetSettings()->GetAudioConfig();
	bool isRecording = _emu->GetSoundMixer()->IsRecording() || _emu->GetVideoRenderer()->IsRecording();
	if(!isRecording && !cfg.DisableDynamicSampleRate) {
		//Don't deviate from selected sample rate while recording
		//TODO: Have 2 output streams (one for recording, one for the speakers)
		AudioStatistics stats = _emu->GetSoundMixer()->GetStatistics();

		if(stats.AverageLatency > 0 && _emu->GetSettings()->GetEmulationSpeed() == 100) {
			//Try to stay within +/- 3ms of requested latency
			constexpr int32_t maxGap = 3;
			constexpr int32_t maxSubAdjustment = 3600;

			int32_t requestedLatency = (int32_t)cfg.AudioLatency;
			double latencyGap = stats.AverageLatency - requestedLatency;
			double adjustment = std::min(0.0025, (std::ceil((std::abs(latencyGap) - maxGap) * 8)) * 0.00003125);

			if(latencyGap < 0 && _underTarget < maxSubAdjustment) {
				_underTarget++;
			} else if(latencyGap > 0 && _underTarget > -maxSubAdjustment) {
				_underTarget--;
			}

			//For every ~1 second spent under/over target latency, further adjust rate (GetTargetRate is called approx. 3x per frame)
			//This should slowly get us closer to the actual output rate of the sound card
			double subAdjustment = 0.00003125 * _underTarget / 180;

			if(adjustment > 0) {
				if(latencyGap > maxGap) {
					_rateAdjustment = 1 - adjustment + subAdjustment;
				} else if(latencyGap < -maxGap) {
					_rateAdjustment = 1 + adjustment + subAdjustment;
				}
			} else if(std::abs(latencyGap) < 1) {
				//Restore normal rate once we get within +/- 1ms
				_rateAdjustment = 1.0 + subAdjustment;
			}
		} else {
			_underTarget = 0;
			_rateAdjustment = 1.0;
		}
	} else {
		_underTarget = 0;
		_rateAdjustment = 1.0;
	}
	return _rateAdjustment;
}

void SoundResampler::UpdateTargetSampleRate(uint32_t sourceRate, uint32_t sampleRate)
{
	double inputRate = sourceRate;

	if(_emu->GetSettings()->GetVideoConfig().IntegerFpsMode) {
		//Adjust input sample rate when using integer fps values
		double baseFps = _emu->GetConsoleUnsafe()->GetFps();
		double roundedFps = _emu->GetFps();
		inputRate = sourceRate * (roundedFps / baseFps);
	}

	double targetRate = sampleRate * GetTargetRateAdjustment();
	if(targetRate != _previousTargetRate || inputRate != _prevInputRate) {
		_previousTargetRate = targetRate;
		_prevInputRate = inputRate;
		_resampler.SetSampleRates(inputRate, targetRate);
	}
}


void SoundResampler::ResetHqDecimator()
{
	_hqLeft.fill(0);
	_hqRight.fill(0);
	_hqWritePosition = 0;
	_hqInputPhase = 0;
}

uint32_t SoundResampler::ResampleHqDecimator(
	int16_t* inSamples,
	uint32_t sampleCount,
	int16_t* outSamples
)
{
	uint32_t outputCount = 0;

	for(uint32_t frame = 0; frame < sampleCount; frame++) {
		_hqLeft[_hqWritePosition] = inSamples[(size_t)frame * 2];
		_hqRight[_hqWritePosition] = inSamples[(size_t)frame * 2 + 1];
		_hqWritePosition = (_hqWritePosition + 1) % HqTapCount;

		_hqInputPhase++;
		if(_hqInputPhase < 2) {
			continue;
		}
		_hqInputPhase = 0;

		int64_t leftAccumulator = 0;
		int64_t rightAccumulator = 0;

		for(uint32_t tap = 0; tap < HqHalfTapCount - 1; tap++) {
			uint32_t recentIndex =
				(_hqWritePosition + HqTapCount - 1 - tap) % HqTapCount;
			uint32_t oldIndex =
				(_hqWritePosition + HqTapCount - 1 - (HqTapCount - 1 - tap))
				% HqTapCount;

			int32_t leftPair =
				(int32_t)_hqLeft[recentIndex] + (int32_t)_hqLeft[oldIndex];
			int32_t rightPair =
				(int32_t)_hqRight[recentIndex] + (int32_t)_hqRight[oldIndex];
			int32_t coefficient = MesenCeHqHalfCoefficients[tap];

			leftAccumulator += (int64_t)leftPair * coefficient;
			rightAccumulator += (int64_t)rightPair * coefficient;
		}

		constexpr uint32_t centerDelay = HqTapCount / 2;
		uint32_t centerIndex =
			(_hqWritePosition + HqTapCount - 1 - centerDelay) % HqTapCount;
		leftAccumulator +=
			(int64_t)_hqLeft[centerIndex] * MesenCeHqHalfCoefficients[HqHalfTapCount - 1];
		rightAccumulator +=
			(int64_t)_hqRight[centerIndex] * MesenCeHqHalfCoefficients[HqHalfTapCount - 1];

		outSamples[(size_t)outputCount * 2] =
			MesenCeRoundAndClampQ30(leftAccumulator);
		outSamples[(size_t)outputCount * 2 + 1] =
			MesenCeRoundAndClampQ30(rightAccumulator);
		outputCount++;
	}

	return outputCount;
}

uint32_t SoundResampler::Resample(int16_t* inSamples, uint32_t sampleCount, uint32_t sourceRate, uint32_t sampleRate, int16_t* outSamples, uint32_t maxOutCount)
{
	UpdateTargetSampleRate(sourceRate, sampleRate);

	uint32_t expectedHqOutput = (_hqInputPhase + sampleCount) / 2;
	bool useHqDecimator = sourceRate == 96000
		&& sampleRate == 48000
		&& std::abs(_prevInputRate - 96000.0) < 0.001
		&& std::abs(_previousTargetRate - 48000.0) < 0.001
		&& expectedHqOutput <= maxOutCount;

	if(useHqDecimator) {
		if(!_hqActive) {
			ResetHqDecimator();
			_resampler.Reset();
			_hqActive = true;
		}
		return ResampleHqDecimator(inSamples, sampleCount, outSamples);
	}

	if(_hqActive) {
		ResetHqDecimator();
		_resampler.Reset();
		_hqActive = false;
	}
	return _resampler.Resample<false>(inSamples, sampleCount, outSamples, maxOutCount);
}