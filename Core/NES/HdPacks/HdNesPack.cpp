#include "pch.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include "NES/HdPacks/HdNesPack.h"
#include "NES/HdPacks/HdPackLoader.h"
#include "NES/NesConsole.h"
#include "NES/BaseMapper.h"
#include "NES/NesDefaultVideoFilter.h"
#include "Shared/MessageManager.h"
#include "Shared/EmuSettings.h"
#include "Utilities/FolderUtilities.h"
#include "Utilities/PNGHelper.h"

template<uint32_t scale>
HdNesPack<scale>::HdNesPack(NesConsole* console, EmuSettings* settings, HdPackData* hdData)
{
	_console = console;
	_settings = settings;
	_hdData = hdData;

	InitializeFallbackTiles();
	CleanupInvalidRules();
}

template<uint32_t scale>
HdNesPack<scale>::~HdNesPack()
{
}

template<uint32_t scale>
void HdNesPack<scale>::CleanupInvalidRules()
{
	//Cleanup invalid <addition> tags
	bool isChrRam = _console->GetMapper()->HasChrRam();
	if(_hdData->AdditionalSprites.size()) {
		vector<HdPackAdditionalSpriteInfo>& elems = _hdData->AdditionalSprites;
		elems.erase(std::remove_if(elems.begin(), elems.end(), [isChrRam](const HdPackAdditionalSpriteInfo& o) { return o.AdditionalTile.IsChrRamTile != isChrRam; }), elems.end());
	}
}

template<uint32_t scale>
void HdNesPack<scale>::InitializeFallbackTiles()
{
	BaseMapper* mapper = _console->GetMapper();
	uint8_t blankTile[16] = {};
	if(mapper->HasChrRom()) {
		if(_hdData->OptionFlags & (int)HdPackOptions::AutomaticFallbackTiles) {
			int32_t chrRomSize = (int32_t)mapper->GetChrRomSize();
			int32_t tileCount = chrRomSize / 16;

			HdTileKey tileKey = {};
			tileKey.IsChrRamTile = true;
			unordered_set<HdTileKey> usedTiles;
			for(auto& tile : _hdData->Tiles) {
				tileKey.TileIndex = tile->TileIndex;
				mapper->CopyChrTile(tile->TileIndex * 16, tileKey.TileData);

				usedTiles.emplace(tileKey);
			}

			for(int32_t i = 0; i < tileCount; i++) {
				mapper->CopyChrTile(i * 16, tileKey.TileData);
				if(memcmp(tileKey.TileData, blankTile, sizeof(blankTile)) == 0) {
					//Skip processing for blank/transparent tiles
					continue;
				}

				auto result = usedTiles.find(tileKey);
				if(result != usedTiles.end() && result->TileIndex != i) {
					_fallbackTiles[i] = result->TileIndex;
				}
			}
		}

		for(auto def : _hdData->FallbackTiles) {
			_fallbackTiles[def.TileIndex] = def.FallbackTileIndex;
		}
	}
}

template<uint32_t scale>
template<HdPackBlendMode blendMode>
void HdNesPack<scale>::BlendColors(uint8_t output[4], uint8_t input[4])
{
	if constexpr(blendMode == HdPackBlendMode::Alpha) {
		uint8_t invertedAlpha = 256 - input[3];
		output[0] = input[0] + (uint8_t)((invertedAlpha * output[0]) >> 8);
		output[1] = input[1] + (uint8_t)((invertedAlpha * output[1]) >> 8);
		output[2] = input[2] + (uint8_t)((invertedAlpha * output[2]) >> 8);
		output[3] = 0xFF;
	} else if constexpr(blendMode == HdPackBlendMode::Add) {
		output[0] = (uint8_t)std::min(255, (int)input[0] + (int)output[0]);
		output[1] = (uint8_t)std::min(255, (int)input[1] + (int)output[1]);
		output[2] = (uint8_t)std::min(255, (int)input[2] + (int)output[2]);
		output[3] = 0xFF;
	} else if constexpr(blendMode == HdPackBlendMode::Subtract) {
		output[0] = (uint8_t)std::max(0, (int)output[0] - (int)input[0]);
		output[1] = (uint8_t)std::max(0, (int)output[1] - (int)input[1]);
		output[2] = (uint8_t)std::max(0, (int)output[2] - (int)input[2]);
		output[3] = 0xFF;
	}
}

template<uint32_t scale>
uint32_t HdNesPack<scale>::AdjustBrightness(uint8_t input[4], int brightness)
{
	return (
		std::min(255, (brightness * ((int)input[0] + 1)) >> 8) |
		(std::min(255, (brightness * ((int)input[1] + 1)) >> 8) << 8) |
		(std::min(255, (brightness * ((int)input[2] + 1)) >> 8) << 16) |
		(input[3] << 24));
}

template<uint32_t scale>
void HdNesPack<scale>::DrawColor(uint32_t color, uint32_t* outputBuffer, uint32_t screenWidth)
{
	if constexpr(scale == 1) {
		*outputBuffer = color;
	} else if constexpr(scale == 2) {
		outputBuffer[0] = color;
		outputBuffer[1] = color;
		outputBuffer[screenWidth] = color;
		outputBuffer[screenWidth + 1] = color;
	} else {
		for(uint32_t i = 0; i < scale; i++) {
			std::fill(outputBuffer, outputBuffer + scale, color);
			outputBuffer += screenWidth;
		}
	}
}

template<uint32_t scale>
template<HdPackBlendMode blendMode>
void HdNesPack<scale>::DrawBackgroundSpan(const HdBgConfig& bgConfig, uint32_t* source, uint32_t* destination, uint32_t count)
{
	// HD GAMEPLAY BULK STRIP COMPOSITOR V111S4
	// Shared contiguous-span kernel used by both the old 8x8 fallback renderer
	// and the new full-width strip renderer.
	HdBackgroundInfo& bgInfo = *bgConfig.Background;
	if constexpr(blendMode == HdPackBlendMode::Alpha) {
		if(bgConfig.BgFastOpaqueCopy) {
			memcpy(destination, source, count * sizeof(uint32_t));
			return;
		}
	}

	if(bgInfo.Brightness == 255) {
		for(uint32_t i = 0; i < count; i++) {
			uint32_t pixelColor = source[i];
			if constexpr(blendMode == HdPackBlendMode::Alpha) {
				if(pixelColor >= 0xFF000000) {
					destination[i] = pixelColor;
				} else if(pixelColor >= 0x01000000) {
					BlendColors<blendMode>((uint8_t*)(destination + i), (uint8_t*)(&pixelColor));
				}
			} else {
				BlendColors<blendMode>((uint8_t*)(destination + i), (uint8_t*)(&pixelColor));
			}
		}
	} else {
		for(uint32_t i = 0; i < count; i++) {
			uint32_t pixelColor = AdjustBrightness((uint8_t*)(source + i), bgInfo.Brightness);
			if constexpr(blendMode == HdPackBlendMode::Alpha) {
				if(pixelColor >= 0xFF000000) {
					destination[i] = pixelColor;
				} else if(pixelColor >= 0x01000000) {
					BlendColors<blendMode>((uint8_t*)(destination + i), (uint8_t*)(&pixelColor));
				}
			} else {
				BlendColors<blendMode>((uint8_t*)(destination + i), (uint8_t*)(&pixelColor));
			}
		}
	}
}

template<uint32_t scale>
template<HdPackBlendMode blendMode>
void HdNesPack<scale>::DrawCustomBackground(const HdBgConfig& bgConfig, uint32_t* outputBuffer, int32_t xHd, int32_t yHd, uint32_t screenWidth)
{
	// V111S3-compatible 8x8 fallback. The expensive path remains available for
	// unusual priorities, non-Gameplay layers, and non-repeating backgrounds.
	if(bgConfig.Background == nullptr || bgConfig.BgPixels == nullptr
		|| bgConfig.BgSourceWidthHd <= 0 || bgConfig.BgSourceHeightHd <= 0) {
		return;
	}

	if(bgConfig.BgRepeatX) {
		if((uint32_t)xHd >= (uint32_t)bgConfig.BgSourceWidthHd) {
			xHd %= bgConfig.BgSourceWidthHd;
			if(xHd < 0) {
				xHd += bgConfig.BgSourceWidthHd;
			}
		}
	} else if(xHd < 0 || xHd + (int32_t)scale > bgConfig.BgSourceWidthHd) {
		return;
	}

	if(bgConfig.BgRepeatY) {
		if((uint32_t)yHd >= (uint32_t)bgConfig.BgSourceHeightHd) {
			yHd %= bgConfig.BgSourceHeightHd;
			if(yHd < 0) {
				yHd += bgConfig.BgSourceHeightHd;
			}
		}
	} else if(yHd < 0 || yHd + (int32_t)scale > bgConfig.BgSourceHeightHd) {
		return;
	}

	int32_t sourceYHd = yHd;
	for(uint32_t row = 0; row < scale; row++) {
		uint32_t* sourceRow = bgConfig.BgPixels
			+ ((bgConfig.BgSourceTopHd + sourceYHd) * bgConfig.BgPngWidth)
			+ bgConfig.BgSourceLeftHd;

		uint32_t remaining = scale;
		int32_t sourceXHd = xHd;
		uint32_t* destination = outputBuffer;
		while(remaining > 0) {
			uint32_t span = remaining;
			if(bgConfig.BgRepeatX) {
				span = std::min<uint32_t>(remaining, (uint32_t)(bgConfig.BgSourceWidthHd - sourceXHd));
			}
			DrawBackgroundSpan<blendMode>(bgConfig, sourceRow + sourceXHd, destination, span);
			remaining -= span;
			destination += span;
			sourceXHd = 0;
		}

		outputBuffer += screenWidth;
		sourceYHd++;
		if(bgConfig.BgRepeatY && sourceYHd >= bgConfig.BgSourceHeightHd) {
			sourceYHd = 0;
		}
	}
}

template<uint32_t scale>
bool HdNesPack<scale>::CanBulkRenderBehindBgSprites()
{
	if(!_bulkParallaxEnabled || !_useLightweightPriorityCompositor || _activeBgCount[0] == 0) {
		return false;
	}

	for(int i = 0; i < _activeBgCount[0]; i++) {
		const HdBgConfig& cfg = _bgConfig[HdNesPack::BehindBgSpritesPriority + i];
		if(cfg.Background == nullptr
			|| cfg.Background->ScrollMode != HdBackgroundScrollMode::Gameplay
			|| !cfg.BgRepeatX
			|| cfg.BgMinX != 0
			|| cfg.BgMaxX != NesConstants::ScreenWidth - 1
			|| cfg.BgSourceWidthHd <= 0
			|| cfg.BgSourceHeightHd <= 0) {
			return false;
		}
	}
	return true;
}

template<uint32_t scale>
template<HdPackBlendMode blendMode>
void HdNesPack<scale>::DrawBulkBackgroundLayer(
	const HdBgConfig& bgConfig,
	uint32_t* outputBuffer,
	uint32_t screenWidth,
	uint32_t xStart,
	uint32_t xEnd
)
{
	if(bgConfig.Background == nullptr || bgConfig.BgPixels == nullptr || xEnd <= xStart) {
		return;
	}

	uint32_t widthHd = (xEnd - xStart) * scale;
	int32_t sourceYHd = bgConfig.BgSourceYHd;
	int32_t initialSourceXHd = bgConfig.BgSourceXHd[xStart];
	if(!bgConfig.BgRepeatY
		&& (sourceYHd < 0 || sourceYHd + (int32_t)scale > bgConfig.BgSourceHeightHd)) {
		return;
	}

	for(uint32_t row = 0; row < scale; row++) {

		uint32_t* sourceRow = bgConfig.BgPixels
			+ ((bgConfig.BgSourceTopHd + sourceYHd) * bgConfig.BgPngWidth)
			+ bgConfig.BgSourceLeftHd;

		uint32_t remaining = widthHd;
		int32_t sourceXHd = initialSourceXHd;
		uint32_t* destination = outputBuffer;
		while(remaining > 0) {
			uint32_t span = std::min<uint32_t>(
				remaining,
				(uint32_t)(bgConfig.BgSourceWidthHd - sourceXHd)
			);
			DrawBackgroundSpan<blendMode>(
				bgConfig,
				sourceRow + sourceXHd,
				destination,
				span
			);
			remaining -= span;
			destination += span;
			sourceXHd = 0;
		}

		outputBuffer += screenWidth;
		sourceYHd++;
		if(bgConfig.BgRepeatY && sourceYHd >= bgConfig.BgSourceHeightHd) {
			sourceYHd = 0;
		}
	}
}

template<uint32_t scale>
void HdNesPack<scale>::DrawBulkBehindBgSpritesLine(
	uint32_t y,
	uint32_t* outputBuffer,
	uint32_t screenWidth,
	uint32_t xStart,
	uint32_t xEnd
)
{
	if(xEnd <= xStart) {
		return;
	}

	// Build the native PPU backdrop once for the first HD row, coalescing runs
	// of identical colors, then duplicate that row to the remaining scale rows.
	uint32_t outputXHd = 0;
	uint32_t x = xStart;
	while(x < xEnd) {
		uint32_t color = _palette[
			_hdScreenInfo->ScreenTiles[(y << 8) | x].Tile.PpuBackgroundColor
		];
		uint32_t runEnd = x + 1;
		while(runEnd < xEnd
			&& _palette[
				_hdScreenInfo->ScreenTiles[(y << 8) | runEnd].Tile.PpuBackgroundColor
			] == color) {
			runEnd++;
		}

		uint32_t runWidthHd = (runEnd - x) * scale;
		std::fill(
			outputBuffer + outputXHd,
			outputBuffer + outputXHd + runWidthHd,
			color
		);
		outputXHd += runWidthHd;
		x = runEnd;
	}

	uint32_t visibleWidthHd = (xEnd - xStart) * scale;
	for(uint32_t row = 1; row < scale; row++) {
		memcpy(
			outputBuffer + row * screenWidth,
			outputBuffer,
			visibleWidthHd * sizeof(uint32_t)
		);
	}

	// Draw each lowest-priority background as long sequential source spans.
	for(int i = 0; i < _activeBgCount[0]; i++) {
		const HdBgConfig& cfg = _bgConfig[HdNesPack::BehindBgSpritesPriority + i];
		switch(cfg.Background->BlendMode) {
			case HdPackBlendMode::Alpha:
				DrawBulkBackgroundLayer<HdPackBlendMode::Alpha>(
					cfg, outputBuffer, screenWidth, xStart, xEnd
				);
				break;
			case HdPackBlendMode::Add:
				DrawBulkBackgroundLayer<HdPackBlendMode::Add>(
					cfg, outputBuffer, screenWidth, xStart, xEnd
				);
				break;
			case HdPackBlendMode::Subtract:
				DrawBulkBackgroundLayer<HdPackBlendMode::Subtract>(
					cfg, outputBuffer, screenWidth, xStart, xEnd
				);
				break;
		}
	}
}
template<uint32_t scale>
void HdNesPack<scale>::DrawTile(HdPpuTileInfo& tileInfo, HdPackTileInfo& hdPackTileInfo, uint32_t* outputBuffer, uint32_t screenWidth)
{
	if(hdPackTileInfo.IsFullyTransparent) {
		return;
	}

	uint32_t* bitmapData = hdPackTileInfo.HdTileData.data();
	uint32_t tileWidth = 8 * scale;
	uint8_t tileOffsetX = tileInfo.HorizontalMirroring ? 7 - tileInfo.OffsetX : tileInfo.OffsetX;
	uint32_t bitmapOffset = (tileInfo.OffsetY * scale) * tileWidth + tileOffsetX * scale;
	int32_t bitmapSmallInc = 1;
	int32_t bitmapLargeInc = tileWidth - scale;
	if(tileInfo.HorizontalMirroring) {
		bitmapOffset += scale - 1;
		bitmapSmallInc = -1;
		bitmapLargeInc = tileWidth + scale;
	}
	if(tileInfo.VerticalMirroring) {
		bitmapOffset += tileWidth * (scale - 1);
		bitmapLargeInc = (tileInfo.HorizontalMirroring ? (int32_t)scale : -(int32_t)scale) - (int32_t)tileWidth;
	}

	uint32_t rgbValue;
	if(hdPackTileInfo.HasTransparentPixels || hdPackTileInfo.Brightness != 255) {
		for(uint32_t y = 0; y < scale; y++) {
			for(uint32_t x = 0; x < scale; x++) {
				if(hdPackTileInfo.Brightness == 255) {
					rgbValue = *(bitmapData + bitmapOffset);
				} else {
					rgbValue = AdjustBrightness((uint8_t*)(bitmapData + bitmapOffset), hdPackTileInfo.Brightness);
				}

				if(!hdPackTileInfo.HasTransparentPixels || (bitmapData[bitmapOffset] & 0xFF000000) == 0xFF000000) {
					*outputBuffer = rgbValue;
				} else {
					if(bitmapData[bitmapOffset] & 0xFF000000) {
						BlendColors<HdPackBlendMode::Alpha>((uint8_t*)outputBuffer, (uint8_t*)&rgbValue);
					}
				}
				outputBuffer++;
				bitmapOffset += bitmapSmallInc;
			}
			bitmapOffset += bitmapLargeInc;
			outputBuffer += screenWidth - scale;
		}
	} else {
		for(uint32_t y = 0; y < scale; y++) {
			for(uint32_t x = 0; x < scale; x++) {
				*outputBuffer = *(bitmapData + bitmapOffset);
				outputBuffer++;
				bitmapOffset += bitmapSmallInc;
			}
			bitmapOffset += bitmapLargeInc;
			outputBuffer += screenWidth - scale;
		}
	}
}

template<uint32_t scale>
void HdNesPack<scale>::OnLineStart(HdPpuPixelInfo& lineFirstPixel, uint8_t y)
{
	int32_t rawScrollX = ((lineFirstPixel.TmpVideoRamAddr & 0x1F) << 3) | lineFirstPixel.XScroll | ((lineFirstPixel.TmpVideoRamAddr & 0x400) ? 0x100 : 0);
	_scrollX = rawScrollX;
	_useCachedTile = false;

	int32_t continuousScrollX = rawScrollX;
	if(_scrollXInitialized[y]) {
		int32_t delta = rawScrollX - (int32_t)_lastRawScrollX[y];
		if(delta > 256) {
			delta -= 512;
		} else if(delta < -256) {
			delta += 512;
		}

		// A real camera moves only a few pixels per frame. A larger discontinuity
		// is a level load, warp, rewind, or state load and should start fresh.
		if(delta > 64 || delta < -64) {
			_continuousScrollX[y] = rawScrollX;
		} else {
			_continuousScrollX[y] += delta;
		}
		continuousScrollX = _continuousScrollX[y];
	} else {
		_continuousScrollX[y] = rawScrollX;
		_scrollXInitialized[y] = true;
	}
	_lastRawScrollX[y] = (uint16_t)rawScrollX;

	int32_t scrollY = (((lineFirstPixel.TmpVideoRamAddr & 0x3E0) >> 2) | ((lineFirstPixel.TmpVideoRamAddr & 0x7000) >> 12)) + ((lineFirstPixel.TmpVideoRamAddr & 0x800) ? 240 : 0);
	for(int layer = 0; layer < 4; layer++) {
		for(int i = 0; i < _activeBgCount[layer]; i++) {
			HdBgConfig& cfg = _bgConfig[layer * HdNesPack::PriorityLevelsPerLayer + i];
			if(cfg.BackgroundIndex < 0) {
				continue;
			}

			HdBackgroundInfo& bgInfo = _hdData->BackgroundsByPriority[cfg.BgPriority][cfg.BackgroundIndex];
			bgInfo.Data->Init();
			bool repeatX = bgInfo.WrapMode == HdBackgroundWrapMode::RepeatX || bgInfo.WrapMode == HdBackgroundWrapMode::RepeatXY;
			bool repeatY = bgInfo.WrapMode == HdBackgroundWrapMode::RepeatY || bgInfo.WrapMode == HdBackgroundWrapMode::RepeatXY;
			bool gameplayScroll = bgInfo.ScrollMode == HdBackgroundScrollMode::Gameplay;
			int32_t selectedRawScrollX = gameplayScroll ? _gameplayRawScrollX : rawScrollX;
			int32_t selectedContinuousScrollX = gameplayScroll ? _gameplayContinuousScrollX : continuousScrollX;
			int32_t selectedScrollY = gameplayScroll ? _gameplayScrollY : scrollY;
			int32_t horizontalScroll = repeatX ? selectedContinuousScrollX : selectedRawScrollX;
			cfg.BgScrollX = (int32_t)(horizontalScroll * bgInfo.HorizontalScrollRatio);
			cfg.BgScrollY = (int32_t)(selectedScrollY * bgInfo.VerticalScrollRatio);

			// HD GAMEPLAY SPAN FAST PATH V111S3
			// Preserve sub-NES-pixel motion, but prepare all repeating coordinates
			// once per scanline rather than wrapping every rendered pixel.
			cfg.BgScrollXHd = gameplayScroll && repeatX
				? (int32_t)std::lround((double)horizontalScroll * bgInfo.HorizontalScrollRatio * (double)scale)
				: cfg.BgScrollX * (int32_t)scale;
			cfg.BgScrollYHd = gameplayScroll && repeatY
				? (int32_t)std::lround((double)selectedScrollY * bgInfo.VerticalScrollRatio * (double)scale)
				: cfg.BgScrollY * (int32_t)scale;

			cfg.Background = &bgInfo;
			cfg.BgPixels = bgInfo.data();
			cfg.BgPngWidth = (int32_t)bgInfo.Data->Width;
			cfg.BgSourceLeftHd = (int32_t)bgInfo.Left * (int32_t)scale;
			cfg.BgSourceTopHd = (int32_t)bgInfo.Top * (int32_t)scale;
			cfg.BgSourceWidthHd = cfg.BgPngWidth - cfg.BgSourceLeftHd;
			cfg.BgSourceHeightHd = (int32_t)bgInfo.Data->Height - cfg.BgSourceTopHd;
			cfg.BgRepeatX = repeatX;
			cfg.BgRepeatY = repeatY;
			cfg.BgFastOpaqueCopy = bgInfo.BlendMode == HdPackBlendMode::Alpha
				&& bgInfo.Brightness == 255
				&& bgInfo.Data->IsFullyOpaque;

			auto normalizeHdCoordinate = [](int32_t value, int32_t size) {
				if(size <= 0) {
					return 0;
				}
				if((uint32_t)value < (uint32_t)size) {
					return value;
				}
				if(value >= size) {
					value -= size;
					if(value < size) {
						return value;
					}
				} else {
					value += size;
					if(value >= 0) {
						return value;
					}
				}
				value %= size;
				return value < 0 ? value + size : value;
			};

			if(repeatX && cfg.BgSourceWidthHd > 0) {
				cfg.BgScrollXHd = normalizeHdCoordinate(cfg.BgScrollXHd, cfg.BgSourceWidthHd);
			}
			if(repeatY && cfg.BgSourceHeightHd > 0) {
				cfg.BgScrollYHd = normalizeHdCoordinate(cfg.BgScrollYHd, cfg.BgSourceHeightHd);
			}

			int32_t preparedXHd = cfg.BgScrollXHd;
			for(int32_t screenX = 0; screenX < NesConstants::ScreenWidth; screenX++) {
				cfg.BgSourceXHd[screenX] = preparedXHd;
				preparedXHd += (int32_t)scale;
				if(repeatX && cfg.BgSourceWidthHd > 0 && preparedXHd >= cfg.BgSourceWidthHd) {
					preparedXHd -= cfg.BgSourceWidthHd;
					if(preparedXHd >= cfg.BgSourceWidthHd) {
						preparedXHd %= cfg.BgSourceWidthHd;
					}
				}
			}

			cfg.BgSourceYHd = ((int32_t)y * (int32_t)scale) + cfg.BgScrollYHd;
			if(repeatY && cfg.BgSourceHeightHd > 0) {
				cfg.BgSourceYHd = normalizeHdCoordinate(cfg.BgSourceYHd, cfg.BgSourceHeightHd);
			}

			int32_t sourceWidth = (int32_t)(bgInfo.Data->Width / scale) - (int32_t)bgInfo.Left;
			int32_t sourceHeight = (int32_t)(bgInfo.Data->Height / scale) - (int32_t)bgInfo.Top;
			bool verticalVisible = repeatY || (y >= -cfg.BgScrollY && (y + bgInfo.Top + cfg.BgScrollY + 1) * scale <= bgInfo.Data->Height);
			if(verticalVisible && sourceWidth > 0 && sourceHeight > 0) {
				if(repeatX) {
					cfg.BgMinX = 0;
					cfg.BgMaxX = NesConstants::ScreenWidth - 1;
				} else {
					cfg.BgMinX = -cfg.BgScrollX;
					cfg.BgMaxX = sourceWidth - cfg.BgScrollX - 1;
				}
			} else {
				cfg.BgMinX = -1;
				cfg.BgMaxX = -1;
			}
		}
	}
}

template<uint32_t scale>
int32_t HdNesPack<scale>::GetLayerIndex(uint8_t priority)
{
	for(size_t i = 0; i < _hdData->BackgroundsByPriority[priority].size(); i++) {
		bool isMatch = true;
		for(HdPackCondition* condition : _hdData->BackgroundsByPriority[priority][i].Conditions) {
			if(!condition->CheckCondition(0, 0, nullptr)) {
				isMatch = false;
				break;
			}
		}

		if(isMatch) {
			return (int32_t)i;
		}
	}
	return -1;
}

template<uint32_t scale>
void HdNesPack<scale>::OnBeforeApplyFilter()
{
	const char* modeRaw = getenv("MESENCE_HD_LIGHTWEIGHT_PRIORITY_MODE");
	string mode = modeRaw ? modeRaw : "lightweight";
	_useLightweightPriorityCompositor = mode != "legacy";
	_lightweightPriorityDiagnostics = mode == "diagnostics";

	// HD GAMEPLAY BULK STRIP COMPOSITOR V111S4
	// Default on; set to 0 for an exact runtime A/B comparison with V111S3.
	const char* bulkRaw = getenv("MESENCE_HD_BULK_PARALLAX");
	_bulkParallaxEnabled = bulkRaw == nullptr || string(bulkRaw) != "0";
	_bulkBehindBgSpritesReady = false;

	_syntheticSpritesByPixel.clear();

	// Synthetic rule matches depend on the current frame state.
	// Never retain a grow/transformation match into a later frame.
	_syntheticMatchCache.clear();
	for(unique_ptr<HdPackCondition>& condition : _hdData->Conditions) {
		condition->Initialize(_hdScreenInfo, this);
	}

	if(_hdData->Palette.size() == 0x40) {
		memcpy(_palette, _hdData->Palette.data(), 0x40 * sizeof(uint32_t));
	} else {
		NesDefaultVideoFilter::GetFullPalette(_palette, _settings->GetNesConfig(), PpuModel::Ppu2C02);
	}
	_cacheEnabled = (_hdData->OptionFlags & (int)HdPackOptions::DisableCache) == 0;

	for(int layer = 0; layer < 4; layer++) {
		uint32_t activeCount = 0;
		for(int i = 0; i < HdNesPack::PriorityLevelsPerLayer; i++) {
			int32_t index = GetLayerIndex(layer * HdNesPack::PriorityLevelsPerLayer + i);
			if(index >= 0) {
				_bgConfig[layer * 10 + activeCount].BgPriority = layer * HdNesPack::PriorityLevelsPerLayer + i;
				_bgConfig[layer * 10 + activeCount].BackgroundIndex = index;
				activeCount++;
			}
		}
		_activeBgCount[layer] = activeCount;
	}

	ProcessAdditionalSprites();
}

template<uint32_t scale>
void HdNesPack<scale>::ProcessAdditionalSprites()
{
	if(_hdData->AdditionalSprites.empty()) {
		return;
	}

	bool checkFallbackTiles = _console->GetMapper()->HasChrRom() && _fallbackTiles.size() > 0;
	HdPpuPixelInfo& lineFirstPixel = _hdScreenInfo->ScreenTiles[0];
	uint32_t yScroll = (((lineFirstPixel.TmpVideoRamAddr & 0x3E0) >> 2) | ((lineFirstPixel.TmpVideoRamAddr & 0x7000) >> 12)) + ((lineFirstPixel.TmpVideoRamAddr & 0x800) ? 240 : 0);
	uint16_t tmpVramAddr = lineFirstPixel.TmpVideoRamAddr;
	bool processBgNextRow = true;

	for(int32_t y = 0; y < NesConstants::ScreenHeight; y++) {
		lineFirstPixel = _hdScreenInfo->ScreenTiles[y << 8];

		//Only process the first scanline for each row of tiles (if no additions are found)
		if(((yScroll + y) & 0x07) == 0 || tmpVramAddr != lineFirstPixel.TmpVideoRamAddr) {
			yScroll = (((lineFirstPixel.TmpVideoRamAddr & 0x3E0) >> 2) | ((lineFirstPixel.TmpVideoRamAddr & 0x7000) >> 12)) + ((lineFirstPixel.TmpVideoRamAddr & 0x800) ? 240 : 0);
			tmpVramAddr = lineFirstPixel.TmpVideoRamAddr;
			processBgNextRow = true;
		}

		if(processBgNextRow) {
			processBgNextRow = false;
			for(int32_t x = 0; x < NesConstants::ScreenWidth; x++) {
				HdPpuPixelInfo& pixelInfo = _hdScreenInfo->ScreenTiles[(y << 8) | x];
				for(uint8_t i = 0; i < pixelInfo.SpriteCount; i++) {
					DrawAdditionalTiles(x, y, pixelInfo.Sprite[i], checkFallbackTiles);
				}

				processBgNextRow |= DrawAdditionalTiles(x, y, pixelInfo.Tile, checkFallbackTiles);
			}
		} else {
			for(int32_t x = 0; x < NesConstants::ScreenWidth; x++) {
				HdPpuPixelInfo& pixelInfo = _hdScreenInfo->ScreenTiles[(y << 8) | x];
				for(uint8_t i = 0; i < pixelInfo.SpriteCount; i++) {
					DrawAdditionalTiles(x, y, pixelInfo.Sprite[i], checkFallbackTiles);
				}
			}
		}
	}
}

template<uint32_t scale>
bool HdNesPack<scale>::DrawAdditionalTiles(int32_t x, int32_t y, HdPpuTileInfo& tile, bool checkFallbackTiles)
{
	auto result = _additionalTilesByKey.find(tile);
	if(result == _additionalTilesByKey.end()) {
		BuildAdditionalTileCache(x, y, tile, checkFallbackTiles);
		result = _additionalTilesByKey.find(tile);
	}

	for(auto& additionalSprite : result->second) {
		InsertAdditionalSprite(x, y, tile, additionalSprite);
	}
	return result->second.size() > 0;
}

template<uint32_t scale>
void HdNesPack<scale>::BuildAdditionalTileCache(int32_t x, int32_t y, HdPpuTileInfo& tile, bool checkFallbackTiles)
{
	vector<HdPackAdditionalSpriteInfo> additions;
	for(HdPackAdditionalSpriteInfo& additionalSprite : _hdData->AdditionalSprites) {
		if(!additionalSprite.IgnorePalette && tile == additionalSprite.OriginalTile) {
			additions.push_back(additionalSprite);
		} else if(additionalSprite.IgnorePalette && tile.GetKey(true) == additionalSprite.OriginalTile.GetKey(true)) {
			additions.push_back(additionalSprite);
		} else if(checkFallbackTiles) {
			if(additionalSprite.IgnorePalette || tile.PaletteColors == additionalSprite.OriginalTile.PaletteColors) {
				if(GetFallbackTile(tile.TileIndex) == additionalSprite.OriginalTile.TileIndex) {
					additions.push_back(additionalSprite);
				}
			}
		}
	}

	//Cache list of additional tiles linked to this sprite
	_additionalTilesByKey.emplace(tile, additions);
}

template<uint32_t scale>
uint32_t HdNesPack<scale>::BuildProvenanceId(HdPpuTileInfo& sprite, HdPackAdditionalSpriteInfo& additionalSprite)
{
	uint32_t hash = 2166136261u;
	auto mix = [&hash](uint32_t value) {
		hash ^= value;
		hash *= 16777619u;
	};

	mix(sprite.GetHashCode());
	mix(additionalSprite.OriginalTile.GetHashCode());
	mix(additionalSprite.AdditionalTile.GetHashCode());
	mix((uint32_t)sprite.RootSpriteIndex);
	mix((uint32_t)sprite.SpritePriorityRank);
	mix((uint32_t)(uint16_t)additionalSprite.OffsetX | ((uint32_t)(uint16_t)additionalSprite.OffsetY << 16));
	return hash;
}

template<uint32_t scale>
void HdNesPack<scale>::InsertAdditionalSprite(int32_t sourceX, int32_t sourceY, HdPpuTileInfo& sprite, HdPackAdditionalSpriteInfo& additionalSprite)
{
	int32_t targetX = sourceX + (sprite.HorizontalMirroring ? -additionalSprite.OffsetX : additionalSprite.OffsetX);
	int32_t targetY = sourceY + (sprite.VerticalMirroring ? -additionalSprite.OffsetY : additionalSprite.OffsetY);

	if(targetX < 0 || targetX >= NesConstants::ScreenWidth || targetY < 0 || targetY >= NesConstants::ScreenHeight) {
		return;
	}

	uint32_t pixelIndex = (targetY << 8) | targetX;
	uint32_t provenanceId = BuildProvenanceId(sprite, additionalSprite);

	if(_useLightweightPriorityCompositor) {
		HdSyntheticSpriteInfo synthetic;
		memcpy(&synthetic.Tile, &sprite, sizeof(HdPpuTileInfo));
		synthetic.Tile.SpriteColorIndex = 0;
		synthetic.Tile.TileIndex = additionalSprite.AdditionalTile.TileIndex;
		synthetic.Tile.PaletteColors = additionalSprite.AdditionalTile.PaletteColors;
		memcpy(synthetic.Tile.TileData, additionalSprite.AdditionalTile.TileData, sizeof(synthetic.Tile.TileData));
		synthetic.Tile.IsSyntheticAddition = true;
		synthetic.Tile.ProvenanceId = provenanceId;
		synthetic.Tile.RootSpriteIndex = sprite.RootSpriteIndex;
		synthetic.Tile.SpritePriorityRank = sprite.SpritePriorityRank;

		auto& bucket = _syntheticSpritesByPixel[pixelIndex];
		bool exists = false;
		for(auto& item : bucket) {
			if(item.Tile.ProvenanceId == provenanceId) {
				exists = true;
				break;
			}
		}
		if(!exists) {
			bucket.push_back(synthetic);
		}
		return;
	}

	HdPpuPixelInfo& pixelInfo = _hdScreenInfo->ScreenTiles[pixelIndex];
	if(pixelInfo.SpriteCount < 4) {
		HdPpuTileInfo& newSprite = pixelInfo.Sprite[pixelInfo.SpriteCount];
		memcpy(&newSprite, &sprite, sizeof(HdPpuTileInfo));
		newSprite.SpriteColorIndex = 0;
		newSprite.TileIndex = additionalSprite.AdditionalTile.TileIndex;
		newSprite.PaletteColors = additionalSprite.AdditionalTile.PaletteColors;
		memcpy(newSprite.TileData, additionalSprite.AdditionalTile.TileData, sizeof(newSprite.TileData));
		newSprite.IsSyntheticAddition = true;
		newSprite.ProvenanceId = provenanceId;
		newSprite.RootSpriteIndex = sprite.RootSpriteIndex;
		newSprite.SpritePriorityRank = sprite.SpritePriorityRank;
		pixelInfo.SpriteCount++;
	}
}

template<uint32_t scale>
HdPackTileInfo* HdNesPack<scale>::GetCachedSyntheticMatchingTile(uint32_t x, uint32_t y, HdSyntheticSpriteInfo& sprite)
{
	if(sprite.MatchResolved) {
		return sprite.ResolvedTile;
	}

	sprite.MatchResolved = true;
	auto cacheIt = _syntheticMatchCache.find(sprite.Tile.ProvenanceId);
	if(cacheIt != _syntheticMatchCache.end()) {
		sprite.ResolvedTile = cacheIt->second;
		return sprite.ResolvedTile;
	}

	sprite.ResolvedTile = GetMatchingTile(x, y, &sprite.Tile);
	_syntheticMatchCache[sprite.Tile.ProvenanceId] = sprite.ResolvedTile;
	return sprite.ResolvedTile;
}

template<uint32_t scale>
HdPackTileInfo* HdNesPack<scale>::GetCachedMatchingTile(uint32_t x, uint32_t y, HdPpuTileInfo* tile)
{
	if(((_scrollX + x) & 0x07) == 0) {
		_useCachedTile = false;
	}

	bool disableCache = false;
	HdPackTileInfo* hdPackTileInfo;
	if(_useCachedTile) {
		hdPackTileInfo = _cachedTile;
	} else {
		hdPackTileInfo = GetMatchingTile(x, y, tile, &disableCache);

		if(!disableCache && _cacheEnabled) {
			//Use this tile for the next 8 horizontal pixels
			//Disable cache if a sprite condition is used, because sprites are not on a 8x8 grid
			_cachedTile = hdPackTileInfo;
			_useCachedTile = true;
		}
	}
	return hdPackTileInfo;
}

template<uint32_t scale>
HdPackTileInfo* HdNesPack<scale>::GetMatchingTile(uint32_t x, uint32_t y, HdPpuTileInfo* tile, bool* disableCache)
{
	auto hdTile = _hdData->TileByKey.find(*tile);
	if(hdTile == _hdData->TileByKey.end()) {
		int32_t fallbackTileIndex = GetFallbackTile(tile->TileIndex);
		if(fallbackTileIndex >= 0) {
			int32_t orgIndex = tile->TileIndex;
			tile->TileIndex = fallbackTileIndex;
			hdTile = _hdData->TileByKey.find(*tile);
			if(hdTile == _hdData->TileByKey.end()) {
				hdTile = _hdData->TileByKey.find(tile->GetKey(true));
				if(hdTile == _hdData->TileByKey.end()) {
					tile->TileIndex = orgIndex;
				}
			}
		}

		if(hdTile == _hdData->TileByKey.end()) {
			hdTile = _hdData->TileByKey.find(tile->GetKey(true));
		}
	}

	if(hdTile != _hdData->TileByKey.end()) {
		for(HdPackTileInfo* hdPackTile : hdTile->second) {
			if(disableCache != nullptr && hdPackTile->ForceDisableCache) {
				*disableCache = true;
			}

			if(hdPackTile->MatchesCondition(x, y, tile)) {
				if(hdPackTile->NeedInit()) {
					hdPackTile->Init();
				}
				return hdPackTile;
			}
		}
	}

	return nullptr;
}

template<uint32_t scale>
void HdNesPack<scale>::DrawBackgroundLayer(uint8_t priority, uint32_t x, uint32_t y, uint32_t* outputBuffer, uint32_t screenWidth)
{
	const HdBgConfig& bgConfig = _bgConfig[(int)priority];
	if((int32_t)x >= bgConfig.BgMinX && (int32_t)x <= bgConfig.BgMaxX && bgConfig.Background != nullptr) {
		HdBackgroundInfo& bgInfo = *bgConfig.Background;
		int32_t sourceXHd = bgConfig.BgSourceXHd[x];
		int32_t sourceYHd = bgConfig.BgSourceYHd;
		switch(bgInfo.BlendMode) {
			case HdPackBlendMode::Alpha: DrawCustomBackground<HdPackBlendMode::Alpha>(bgConfig, outputBuffer, sourceXHd, sourceYHd, screenWidth); break;
			case HdPackBlendMode::Add: DrawCustomBackground<HdPackBlendMode::Add>(bgConfig, outputBuffer, sourceXHd, sourceYHd, screenWidth); break;
			case HdPackBlendMode::Subtract: DrawCustomBackground<HdPackBlendMode::Subtract>(bgConfig, outputBuffer, sourceXHd, sourceYHd, screenWidth); break;
		}
	}
}
template<uint32_t scale>
void HdNesPack<scale>::GetPixels(uint32_t x, uint32_t y, HdPpuPixelInfo& pixelInfo, uint32_t* outputBuffer, uint32_t screenWidth)
{
	if(!_useLightweightPriorityCompositor) {
		HdPackTileInfo* hdPackTileInfo = nullptr;
		HdPackTileInfo* hdPackSpriteInfo = nullptr;
		bool hasSprite = pixelInfo.SpriteCount > 0;
		bool renderOriginalTiles = ((_hdData->OptionFlags & (int)HdPackOptions::DontRenderOriginalTiles) == 0);

		if(pixelInfo.Tile.TileIndex != HdPpuTileInfo::NoTile) {
			hdPackTileInfo = GetCachedMatchingTile(x, y, &pixelInfo.Tile);
		}

		int lowestBgSprite = 999;
		DrawColor(_palette[pixelInfo.Tile.PpuBackgroundColor], outputBuffer, screenWidth);

		for(int i = 0; i < _activeBgCount[0]; i++) {
			DrawBackgroundLayer(HdNesPack::BehindBgSpritesPriority + i, x, y, outputBuffer, screenWidth);
		}

		if(hasSprite) {
			for(int k = pixelInfo.SpriteCount - 1; k >= 0; k--) {
				if(pixelInfo.Sprite[k].BackgroundPriority) {
					if(pixelInfo.Sprite[k].SpriteColorIndex != 0) {
						lowestBgSprite = k;
					}
					hdPackSpriteInfo = GetMatchingTile(x, y, &pixelInfo.Sprite[k]);
					if(hdPackSpriteInfo) {
						DrawTile(pixelInfo.Sprite[k], *hdPackSpriteInfo, outputBuffer, screenWidth);
					} else if(pixelInfo.Sprite[k].SpriteColorIndex != 0) {
						DrawColor(_palette[pixelInfo.Sprite[k].SpriteColor], outputBuffer, screenWidth);
					}
				}
			}
		}

		for(int i = 0; i < _activeBgCount[1]; i++) {
			DrawBackgroundLayer(HdNesPack::BehindBgPriority + i, x, y, outputBuffer, screenWidth);
		}

		if(hdPackTileInfo) {
			DrawTile(pixelInfo.Tile, *hdPackTileInfo, outputBuffer, screenWidth);
		} else if(renderOriginalTiles && pixelInfo.Tile.BgColorIndex != 0) {
			DrawColor(_palette[pixelInfo.Tile.BgColor], outputBuffer, screenWidth);
		}

		for(int i = 0; i < _activeBgCount[2]; i++) {
			DrawBackgroundLayer(HdNesPack::BehindFgSpritesPriority + i, x, y, outputBuffer, screenWidth);
		}

		if(hasSprite) {
			for(int k = pixelInfo.SpriteCount - 1; k >= 0; k--) {
				if(!pixelInfo.Sprite[k].BackgroundPriority && lowestBgSprite > k) {
					hdPackSpriteInfo = GetMatchingTile(x, y, &pixelInfo.Sprite[k]);
					if(hdPackSpriteInfo) {
						DrawTile(pixelInfo.Sprite[k], *hdPackSpriteInfo, outputBuffer, screenWidth);
					} else if(pixelInfo.Sprite[k].SpriteColorIndex != 0) {
						DrawColor(_palette[pixelInfo.Sprite[k].SpriteColor], outputBuffer, screenWidth);
					}
				}
			}
		}

		for(int i = 0; i < _activeBgCount[3]; i++) {
			DrawBackgroundLayer(HdNesPack::ForegroundPriority + i, x, y, outputBuffer, screenWidth);
		}
		return;
	}

	HdPackTileInfo* hdPackTileInfo = nullptr;
	bool renderOriginalTiles = ((_hdData->OptionFlags & (int)HdPackOptions::DontRenderOriginalTiles) == 0);
	if(pixelInfo.Tile.TileIndex != HdPpuTileInfo::NoTile) {
		hdPackTileInfo = GetCachedMatchingTile(x, y, &pixelInfo.Tile);
	}

	struct LocalCandidate {
		HdPpuTileInfo* Tile = nullptr;
		HdPackTileInfo* HdTile = nullptr;
		bool Synthetic = false;
	};

	LocalCandidate bgSprites[16] = {};
	LocalCandidate fgSprites[16] = {};
	int bgCount = 0;
	int fgCount = 0;

	auto pushCandidate = [&](LocalCandidate* bucket, int& count, HdPpuTileInfo* tile, HdPackTileInfo* hdTile, bool synthetic) {
		if(count >= 16) {
			return;
		}
		bucket[count].Tile = tile;
		bucket[count].HdTile = hdTile;
		bucket[count].Synthetic = synthetic;
		count++;
	};

	for(uint8_t k = 0; k < pixelInfo.SpriteCount; k++) {
		HdPackTileInfo* hd = GetMatchingTile(x, y, &pixelInfo.Sprite[k]);
		if(pixelInfo.Sprite[k].BackgroundPriority) {
			pushCandidate(bgSprites, bgCount, &pixelInfo.Sprite[k], hd, false);
		} else {
			pushCandidate(fgSprites, fgCount, &pixelInfo.Sprite[k], hd, false);
		}
	}

	auto syntheticIt = _syntheticSpritesByPixel.find((y << 8) | x);
	if(syntheticIt != _syntheticSpritesByPixel.end()) {
		for(auto& synthetic : syntheticIt->second) {
			HdPackTileInfo* hd = GetCachedSyntheticMatchingTile(x, y, synthetic);
			if(synthetic.Tile.BackgroundPriority) {
				pushCandidate(bgSprites, bgCount, &synthetic.Tile, hd, true);
			} else {
				pushCandidate(fgSprites, fgCount, &synthetic.Tile, hd, true);
			}
		}
	}

	auto sortCandidates = [](LocalCandidate* bucket, int count) {
		std::stable_sort(bucket, bucket + count, [](const LocalCandidate& a, const LocalCandidate& b) {
			if(a.Tile->SpritePriorityRank != b.Tile->SpritePriorityRank) {
				return a.Tile->SpritePriorityRank > b.Tile->SpritePriorityRank;
			}
			if(a.Tile->RootSpriteIndex != b.Tile->RootSpriteIndex) {
				return a.Tile->RootSpriteIndex > b.Tile->RootSpriteIndex;
			}
			if(a.Synthetic != b.Synthetic) {
				return a.Synthetic && !b.Synthetic;
			}
			return a.Tile->ProvenanceId < b.Tile->ProvenanceId;
		});
	};

	sortCandidates(bgSprites, bgCount);
	sortCandidates(fgSprites, fgCount);

	auto drawCandidateList = [&](LocalCandidate* bucket, int count) {
		for(int i = 0; i < count; i++) {
			HdPpuTileInfo& tile = *bucket[i].Tile;
			HdPackTileInfo* hd = bucket[i].HdTile;
			if(hd) {
				DrawTile(tile, *hd, outputBuffer, screenWidth);
			} else if(tile.SpriteColorIndex != 0) {
				DrawColor(_palette[tile.SpriteColor], outputBuffer, screenWidth);
			}
		}
	};

	if(!_bulkBehindBgSpritesReady) {
		DrawColor(_palette[pixelInfo.Tile.PpuBackgroundColor], outputBuffer, screenWidth);
		for(int i = 0; i < _activeBgCount[0]; i++) {
			DrawBackgroundLayer(HdNesPack::BehindBgSpritesPriority + i, x, y, outputBuffer, screenWidth);
		}
	}

	drawCandidateList(bgSprites, bgCount);

	for(int i = 0; i < _activeBgCount[1]; i++) {
		DrawBackgroundLayer(HdNesPack::BehindBgPriority + i, x, y, outputBuffer, screenWidth);
	}

	if(hdPackTileInfo) {
		DrawTile(pixelInfo.Tile, *hdPackTileInfo, outputBuffer, screenWidth);
	} else if(renderOriginalTiles && pixelInfo.Tile.BgColorIndex != 0) {
		DrawColor(_palette[pixelInfo.Tile.BgColor], outputBuffer, screenWidth);
	}

	for(int i = 0; i < _activeBgCount[2]; i++) {
		DrawBackgroundLayer(HdNesPack::BehindFgSpritesPriority + i, x, y, outputBuffer, screenWidth);
	}

	drawCandidateList(fgSprites, fgCount);

	for(int i = 0; i < _activeBgCount[3]; i++) {
		DrawBackgroundLayer(HdNesPack::ForegroundPriority + i, x, y, outputBuffer, screenWidth);
	}
}

template<uint32_t scale>
void HdNesPack<scale>::Process(HdScreenInfo* hdScreenInfo, uint32_t* outputBuffer, OverscanDimensions& overscan)
{
	_hdScreenInfo = hdScreenInfo;

	// HD GAMEPLAY FAST-FORWARD CONTINUITY FIX V111F1
	// RetroArch fast-forward may skip presentation frames, so a forward gap in
	// FrameNumber is normal and must not discard the accumulated world scroll.
	// Only a backward-moving frame counter indicates rewind/state-load history
	// where the HD-only camera accumulator should restart.
	bool frameCounterMovedBackwards = _frameNumberInitialized && hdScreenInfo->FrameNumber < _lastFrameNumber;
	if(!_frameNumberInitialized || frameCounterMovedBackwards) {
		for(uint32_t y = 0; y < NesConstants::ScreenHeight; y++) {
			_scrollXInitialized[y] = false;
		}
		_gameplayScrollInitialized = false;
		_frameNumberInitialized = true;
	}
	_lastFrameNumber = hdScreenInfo->FrameNumber;

	// Select the dominant horizontal PPU scroll below the usual HUD region.
	// This generalizes to games with a fixed top status bar without touching
	// emulated RAM or requiring game-specific addresses.
	uint16_t gameplayScrollCounts[512] = {};
	uint16_t bestGameplayCount = 0;
	uint32_t gameplaySampleY = 32;
	int32_t gameplayRawScrollX = 0;
	for(uint32_t sampleY = 32; sampleY < NesConstants::ScreenHeight; sampleY++) {
		HdPpuPixelInfo& samplePixel = hdScreenInfo->ScreenTiles[sampleY << 8];
		int32_t sampleScrollX = ((samplePixel.TmpVideoRamAddr & 0x1F) << 3) | samplePixel.XScroll | ((samplePixel.TmpVideoRamAddr & 0x400) ? 0x100 : 0);
		uint16_t count = ++gameplayScrollCounts[sampleScrollX & 0x1FF];
		if(count > bestGameplayCount) {
			bestGameplayCount = count;
			gameplayRawScrollX = sampleScrollX;
			gameplaySampleY = sampleY;
		}
	}

	HdPpuPixelInfo& gameplayPixel = hdScreenInfo->ScreenTiles[gameplaySampleY << 8];
	_gameplayScrollY = (((gameplayPixel.TmpVideoRamAddr & 0x3E0) >> 2) | ((gameplayPixel.TmpVideoRamAddr & 0x7000) >> 12)) + ((gameplayPixel.TmpVideoRamAddr & 0x800) ? 240 : 0);
	_gameplayRawScrollX = gameplayRawScrollX;
	if(_gameplayScrollInitialized) {
		int32_t delta = gameplayRawScrollX - (_gameplayContinuousScrollX & 0x1FF);
		if(delta > 256) {
			delta -= 512;
		} else if(delta < -256) {
			delta += 512;
		}
		if(delta > 64 || delta < -64) {
			_gameplayContinuousScrollX = gameplayRawScrollX;
		} else {
			_gameplayContinuousScrollX += delta;
		}
	} else {
		_gameplayContinuousScrollX = gameplayRawScrollX;
		_gameplayScrollInitialized = true;
	}

	uint32_t hdScale = GetScale();
	uint32_t screenWidth = (NesConstants::ScreenWidth - overscan.Left - overscan.Right) * hdScale;

	OnBeforeApplyFilter();
	for(uint32_t i = overscan.Top, iMax = 240 - overscan.Bottom; i < iMax; i++) {
		OnLineStart(hdScreenInfo->ScreenTiles[i << 8], i);
		uint32_t bufferIndex = (i - overscan.Top) * screenWidth * hdScale;
		uint32_t lineStartIndex = bufferIndex;

		// HD GAMEPLAY BULK STRIP COMPOSITOR V111S4
		// Precompose the full lowest-priority stack once, then GetPixels handles
		// only background sprites, tiles, foreground sprites, and higher layers.
		_bulkBehindBgSpritesReady = CanBulkRenderBehindBgSprites();
		if(_bulkBehindBgSpritesReady) {
			DrawBulkBehindBgSpritesLine(
				i,
				outputBuffer + lineStartIndex,
				screenWidth,
				overscan.Left,
				256 - overscan.Right
			);
		}

		for(uint32_t j = overscan.Left, jMax = 256 - overscan.Right; j < jMax; j++) {
			GetPixels(j, i, hdScreenInfo->ScreenTiles[i * 256 + j], outputBuffer + bufferIndex, screenWidth);
			bufferIndex += hdScale;
		}

		ProcessGrayscaleAndEmphasis(hdScreenInfo->ScreenTiles[i * 256], outputBuffer + lineStartIndex, screenWidth);
	}
}

template<uint32_t scale>
void HdNesPack<scale>::ProcessGrayscaleAndEmphasis(HdPpuPixelInfo& pixelInfo, uint32_t* outputBuffer, uint32_t hdScreenWidth)
{
	//Apply grayscale/emphasis bits on a scanline level (less accurate, but shouldn't cause issues and simpler to implement)
	if(pixelInfo.Grayscale) {
		uint32_t* out = outputBuffer;
		for(uint32_t y = 0; y < scale; y++) {
			for(uint32_t x = 0; x < hdScreenWidth; x++) {
				uint32_t& rgbValue = out[x];
				uint8_t average = (((rgbValue >> 16) & 0xFF) + ((rgbValue >> 8) & 0xFF) + (rgbValue & 0xFF)) / 3;
				rgbValue = (rgbValue & 0xFF000000) | (average << 16) | (average << 8) | average;
			}
			out += hdScreenWidth;
		}
	}

	if(pixelInfo.EmphasisBits) {
		uint8_t emphasisBits = pixelInfo.EmphasisBits;
		double red = 1.0, green = 1.0, blue = 1.0;
		if(emphasisBits & 0x01) {
			//Intensify red
			red *= 1.1;
			green *= 0.9;
			blue *= 0.9;
		}
		if(emphasisBits & 0x02) {
			//Intensify green
			green *= 1.1;
			red *= 0.9;
			blue *= 0.9;
		}
		if(emphasisBits & 0x04) {
			//Intensify blue
			blue *= 1.1;
			red *= 0.9;
			green *= 0.9;
		}

		uint32_t* out = outputBuffer;
		for(uint32_t y = 0; y < scale; y++) {
			for(uint32_t x = 0; x < hdScreenWidth; x++) {
				uint32_t& rgbValue = out[x];

				rgbValue = 0xFF000000 |
					(std::min<uint16_t>((uint16_t)(((rgbValue >> 16) & 0xFF) * red), 255) << 16) |
					(std::min<uint16_t>((uint16_t)(((rgbValue >> 8) & 0xFF) * green), 255) << 8) |
					std::min<uint16_t>((uint16_t)((rgbValue & 0xFF) * blue), 255);
			}
			out += hdScreenWidth;
		}
	}
}

template HdNesPack<1>::HdNesPack(NesConsole* console, EmuSettings* settings, HdPackData* hdData);
template HdNesPack<2>::HdNesPack(NesConsole* console, EmuSettings* settings, HdPackData* hdData);
template HdNesPack<3>::HdNesPack(NesConsole* console, EmuSettings* settings, HdPackData* hdData);
template HdNesPack<4>::HdNesPack(NesConsole* console, EmuSettings* settings, HdPackData* hdData);
template HdNesPack<5>::HdNesPack(NesConsole* console, EmuSettings* settings, HdPackData* hdData);
template HdNesPack<6>::HdNesPack(NesConsole* console, EmuSettings* settings, HdPackData* hdData);
template HdNesPack<7>::HdNesPack(NesConsole* console, EmuSettings* settings, HdPackData* hdData);
template HdNesPack<8>::HdNesPack(NesConsole* console, EmuSettings* settings, HdPackData* hdData);
template HdNesPack<9>::HdNesPack(NesConsole* console, EmuSettings* settings, HdPackData* hdData);
template HdNesPack<10>::HdNesPack(NesConsole* console, EmuSettings* settings, HdPackData* hdData);
