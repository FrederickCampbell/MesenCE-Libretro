#pragma once
#include "pch.h"
#include "NES/HdPacks/HdData.h"

class NesConsole;
class EmuSettings;

class BaseHdNesPack
{
protected:
	unordered_map<int32_t, int32_t> _fallbackTiles;
	HdScreenInfo* _hdScreenInfo = nullptr;

public:
	static constexpr uint32_t CurrentVersion = 111;

	virtual uint32_t GetScale() = 0;

	HdScreenInfo* GetScreenInfo() { return _hdScreenInfo; }

	int32_t GetFallbackTile(int32_t tileIndex)
	{
		auto result = _fallbackTiles.find(tileIndex);
		if(result != _fallbackTiles.end()) {
			return result->second;
		}
		return -1;
	}

	virtual void Process(HdScreenInfo* hdScreenInfo, uint32_t* outputBuffer, OverscanDimensions& overscan) = 0;

	virtual ~BaseHdNesPack() {}
};

template<uint32_t scale>
class HdNesPack final : public BaseHdNesPack
{
private:
	struct HdBgConfig
	{
		int32_t BackgroundIndex = -1;
		int32_t BgPriority = -1;
		// Native offsets remain for legacy visibility and compatibility.
		int32_t BgScrollX = 0;
		int32_t BgScrollY = 0;
		// Real PNG-pixel offsets allow repeating Gameplay layers to move smoothly.
		int32_t BgScrollXHd = 0;
		int32_t BgScrollYHd = 0;

		// HD GAMEPLAY SPAN FAST PATH V111S3
		// Prepared once per scanline so the pixel compositor only reads cached
		// coordinates and contiguous source spans.
		HdBackgroundInfo* Background = nullptr;
		uint32_t* BgPixels = nullptr;
		int32_t BgPngWidth = 0;
		int32_t BgSourceLeftHd = 0;
		int32_t BgSourceTopHd = 0;
		int32_t BgSourceWidthHd = 0;
		int32_t BgSourceHeightHd = 0;
		int32_t BgSourceXHd[NesConstants::ScreenWidth] = {};
		int32_t BgSourceYHd = 0;
		bool BgRepeatX = false;
		bool BgRepeatY = false;
		bool BgFastOpaqueCopy = false;

		int16_t BgMinX = -1;
		int16_t BgMaxX = -1;
	};

	struct HdSyntheticSpriteInfo
	{
		HdPpuTileInfo Tile = {};
		HdPackTileInfo* ResolvedTile = nullptr;
		bool MatchResolved = false;
	};

	static constexpr uint8_t PriorityLevelsPerLayer = 10;
	static constexpr uint8_t BehindBgSpritesPriority = 0 * PriorityLevelsPerLayer;
	static constexpr uint8_t BehindBgPriority = 1 * PriorityLevelsPerLayer;
	static constexpr uint8_t BehindFgSpritesPriority = 2 * PriorityLevelsPerLayer;
	static constexpr uint8_t ForegroundPriority = 3 * PriorityLevelsPerLayer;

	NesConsole* _console = nullptr;
	EmuSettings* _settings = nullptr;
	HdPackData* _hdData = nullptr;

	uint8_t _activeBgCount[4] = {};
	HdBgConfig _bgConfig[40] = {};

	uint32_t _palette[512] = {};
	HdPackTileInfo* _cachedTile = nullptr;
	bool _cacheEnabled = false;
	bool _useCachedTile = false;
	int32_t _scrollX = 0;

	// RepeatX layers use an unwrapped scroll value per scanline so arbitrary
	// parallax ratios remain continuous across the NES 9-bit PPU wrap.
	int32_t _continuousScrollX[NesConstants::ScreenHeight] = {};
	uint16_t _lastRawScrollX[NesConstants::ScreenHeight] = {};
	bool _scrollXInitialized[NesConstants::ScreenHeight] = {};
	uint32_t _lastFrameNumber = 0;
	bool _frameNumberInitialized = false;

	// Gameplay-mode HD backgrounds use the dominant playfield scroll for every
	// scanline, preventing fixed HUD splits from freezing the top of the layer.
	int32_t _gameplayRawScrollX = 0;
	int32_t _gameplayContinuousScrollX = 0;
	int32_t _gameplayScrollY = 0;
	bool _gameplayScrollInitialized = false;

	unordered_map<HdTileKey, vector<HdPackAdditionalSpriteInfo>> _additionalTilesByKey;
	unordered_map<uint32_t, vector<HdSyntheticSpriteInfo>> _syntheticSpritesByPixel;
	unordered_map<uint32_t, HdPackTileInfo*> _syntheticMatchCache;
	bool _useLightweightPriorityCompositor = true;
	bool _lightweightPriorityDiagnostics = false;

	// HD GAMEPLAY BULK STRIP COMPOSITOR V111S4
	// True only for the current scanline when the entire lowest-priority
	// background group can be safely precomposed before tile/sprite rendering.
	bool _bulkParallaxEnabled = true;
	bool _bulkBehindBgSpritesReady = false;
	// MESENCE_RENDERER_CORRECTNESS_V1
	uint64_t _candidateOverflowCount = 0;
	uint32_t _candidateHighWatermark = 0;

	template<HdPackBlendMode blendMode>
	__forceinline void BlendColors(uint8_t output[4], uint8_t input[4]);

	__forceinline uint32_t AdjustBrightness(uint8_t input[4], int brightness);
	__forceinline void DrawColor(uint32_t color, uint32_t* outputBuffer, uint32_t screenWidth);
	__forceinline void DrawTile(HdPpuTileInfo& tileInfo, HdPackTileInfo& hdPackTileInfo, uint32_t* outputBuffer, uint32_t screenWidth);

	__forceinline HdPackTileInfo* GetCachedMatchingTile(uint32_t x, uint32_t y, HdPpuTileInfo* tile);
	__forceinline HdPackTileInfo* GetMatchingTile(uint32_t x, uint32_t y, HdPpuTileInfo* tile, bool* disableCache = nullptr);

	__forceinline void DrawBackgroundLayer(uint8_t priority, uint32_t x, uint32_t y, uint32_t* outputBuffer, uint32_t screenWidth);

	template<HdPackBlendMode blendMode>
	__forceinline void DrawBackgroundSpan(const HdBgConfig& bgConfig, uint32_t* source, uint32_t* destination, uint32_t count);
	template<HdPackBlendMode blendMode>
	__forceinline void DrawCustomBackground(const HdBgConfig& bgConfig, uint32_t* outputBuffer, int32_t xHd, int32_t yHd, uint32_t screenWidth);
	bool CanBulkRenderBehindBgSprites();
	template<HdPackBlendMode blendMode>
	__forceinline void DrawBulkBackgroundLayer(const HdBgConfig& bgConfig, uint32_t* outputBuffer, uint32_t screenWidth, uint32_t xStart, uint32_t xEnd);
	void DrawBulkBehindBgSpritesLine(uint32_t y, uint32_t* outputBuffer, uint32_t screenWidth, uint32_t xStart, uint32_t xEnd);

	void OnLineStart(HdPpuPixelInfo& lineFirstPixel, uint8_t y);
	int32_t GetLayerIndex(uint8_t priority);
	void OnBeforeApplyFilter();

	void ProcessAdditionalSprites();
	bool DrawAdditionalTiles(int32_t x, int32_t y, HdPpuTileInfo& tile, bool checkFallbackTiles);
	void BuildAdditionalTileCache(int32_t x, int32_t y, HdPpuTileInfo& tile, bool checkFallbackTiles);
	void InsertAdditionalSprite(int32_t x, int32_t y, HdPpuTileInfo& sprite, HdPackAdditionalSpriteInfo& additionalSprite);
	uint32_t BuildProvenanceId(HdPpuTileInfo& sprite, HdPackAdditionalSpriteInfo& additionalSprite);
	HdPackTileInfo* GetCachedSyntheticMatchingTile(uint32_t x, uint32_t y, HdSyntheticSpriteInfo& sprite);

	__forceinline void GetPixels(uint32_t x, uint32_t y, HdPpuPixelInfo& pixelInfo, uint32_t* outputBuffer, uint32_t screenWidth);
	__forceinline void ProcessGrayscaleAndEmphasis(HdPpuPixelInfo& pixelInfo, uint32_t* outputBuffer, uint32_t hdScreenWidth);

	void CleanupInvalidRules();
	void InitializeFallbackTiles();

public:
	HdNesPack(NesConsole* console, EmuSettings* settings, HdPackData* hdData);
	virtual ~HdNesPack();

	uint32_t GetScale() override { return scale; }

	void Process(HdScreenInfo* hdScreenInfo, uint32_t* outputBuffer, OverscanDimensions& overscan) override;
};
