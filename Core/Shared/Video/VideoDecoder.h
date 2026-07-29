#pragma once
#include "pch.h"
#include <thread>
#include "Utilities/SimpleLock.h"
#include "Utilities/AutoResetEvent.h"
#include "Shared/SettingTypes.h"
#include "Shared/RenderedFrame.h"

class BaseVideoFilter;
class ScaleFilter;
class RotateFilter;
class IRenderingDevice;
class Emulator;

class VideoDecoder
{
private:
	Emulator* _emu;

	ConsoleType _consoleType = ConsoleType::Snes;

	unique_ptr<std::thread> _decodeThread;

	SimpleLock _stopStartLock;
	AutoResetEvent _waitForFrame;
	// MESENCE_PERF_V1_0_25: Completion event for immediate Libretro wake-up.
	AutoResetEvent _frameDecoded;

	atomic<bool> _frameChanged;
	atomic<bool> _stopFlag;
	// MESENCE_DECODER_FRAME_INTEGRITY_V1
	atomic<bool> _allowFrameDropping { false };
	atomic<uint32_t> _lastDecodedFrameNumber { 0 };
	atomic<uint64_t> _busyFrameDrops { 0 };
	uint32_t _frameCount = 0;
	bool _forceFilterUpdate = false;

	double _lastAspectRatio = 0.0;

	FrameInfo _baseFrameSize = {};
	FrameInfo _lastFrameSize = {};
	RenderedFrame _frame = {};

	VideoFilterType _videoFilterType = VideoFilterType::None;
	unique_ptr<BaseVideoFilter> _videoFilter;
	unique_ptr<ScaleFilter> _scaleFilter;
	unique_ptr<RotateFilter> _rotateFilter;

	void UpdateVideoFilter();

	void DecodeThread();

public:
	VideoDecoder(Emulator* console);
	~VideoDecoder();

	void Init();

	void DecodeFrame(bool synchronous = false);
	void TakeScreenshot(string romName = "");
	void TakeScreenshot(std::stringstream& stream);

	void ForceFilterUpdate() { _forceFilterUpdate = true; }

	uint32_t GetFrameCount();
	FrameInfo GetBaseFrameInfo(bool removeOverscan);
	FrameInfo GetFrameInfo();
	double GetLastFrameScale() { return _frame.Scale; }

	void UpdateFrame(RenderedFrame frame, bool sync, bool forRewind);

	void WaitForAsyncFrameDecode();
	void SetAllowFrameDropping(bool allow)
	{
		_allowFrameDropping.store(allow, std::memory_order_release);
	}
	uint32_t GetLastDecodedFrameNumber() const
	{
		return _lastDecodedFrameNumber.load(std::memory_order_acquire);
	}
	uint64_t GetBusyFrameDropCount() const
	{
		return _busyFrameDrops.load(std::memory_order_relaxed);
	}

	bool IsRunning();
	void StartThread();
	void StopThread();
};