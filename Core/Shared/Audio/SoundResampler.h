#pragma once
#include "pch.h"
#include <array>
#include "Utilities/Audio/HermiteResampler.h"

class Emulator;

class SoundResampler
{
private:
	Emulator* _emu;

	double _rateAdjustment = 1.0;
	double _previousTargetRate = 0;
	double _prevInputRate = 0;
	int32_t _underTarget = 0;

	HermiteResampler _resampler;

	// MESENCE_HQ_AUDIO_DECIMATOR_V1
	// Stateful 127-tap symmetric fixed-point FIR used only for the exact
	// 96 kHz stereo NES mix to 48 kHz output path.
	static constexpr uint32_t HqTapCount = 127;
	static constexpr uint32_t HqHalfTapCount = 64;
	std::array<int16_t, HqTapCount> _hqLeft = {};
	std::array<int16_t, HqTapCount> _hqRight = {};
	uint32_t _hqWritePosition = 0;
	uint32_t _hqInputPhase = 0;
	bool _hqActive = false;

	void ResetHqDecimator();
	uint32_t ResampleHqDecimator(
		int16_t* inSamples,
		uint32_t sampleCount,
		int16_t* outSamples
	);

	double GetTargetRateAdjustment();
	void UpdateTargetSampleRate(uint32_t sourceRate, uint32_t sampleRate);

public:
	SoundResampler(Emulator* emu);
	~SoundResampler();

	double GetRateAdjustment();
	uint32_t GetTargetRate();

	uint32_t Resample(int16_t* inSamples, uint32_t sampleCount, uint32_t sourceRate, uint32_t sampleRate, int16_t* outSamples, uint32_t maxOutCount);
};