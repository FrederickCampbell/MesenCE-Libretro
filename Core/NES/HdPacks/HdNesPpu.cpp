#include "pch.h"
#include "NES/HdPacks/HdNesPpu.h"
#include "NES/NesConsole.h"
#include "NES/HdPacks/HdPackConditions.h"
#include "NES/NesMemoryManager.h"
#include "NES/BaseMapper.h"
#include "NES/HdPacks/HdData.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace
{
	constexpr uint64_t StableFnvOffset = 1469598103934665603ull;
	constexpr uint64_t StableFnvPrime = 1099511628211ull;

	inline void StableMix(uint64_t& hash, uint64_t value)
	{
		for(int i = 0; i < 8; i++) {
			hash ^= (uint8_t)value;
			hash *= StableFnvPrime;
			value >>= 8;
		}
	}

	inline int StableClampVelocity(int value)
	{
		return std::max(-16, std::min(16, value));
	}
}

void HdNesPpu::ResetStableObjectTracker()
{
	for(StableObjectTrack& track : _stableObjectTracks) {
		track = {};
	}
	_nextStableObjectTrackId = 1;
	_stableObjectLastFrame = 0;
	_stableObjectTrackerInitialized = false;
}

void HdNesPpu::FinalizeStableObjectPriorities(HdScreenInfo* info)
{
	std::fill_n(
		info->StableObjectPriorityByOam,
		64,
		0xFFFFFFFFFFFFFFFFull
	);
	std::fill_n(
		info->StableObjectSortKeyByOam,
		64,
		(uint16_t)0xFFFF
	);

	if(
		!_stableObjectTrackerInitialized
		|| _frameCount <= _stableObjectLastFrame
		|| _frameCount > _stableObjectLastFrame + 1
	) {
		ResetStableObjectTracker();
		_stableObjectTrackerInitialized = true;
	}
	_stableObjectLastFrame = _frameCount;

	struct OamSample {
		bool Visible = false;
		uint32_t PaletteColors = 0;
		uint32_t TileHash = 0;
		int32_t TileIndex = 0;
		int16_t OriginX = 0;
		int16_t OriginY = 0;
		bool HorizontalMirror = false;
		bool VerticalMirror = false;
	};

	// One native-resolution pass collects one representative source tile and
	// its actual rendered origin per original primary-OAM entry. This is robust
	// against late OAM writes and does not touch the 8x output buffer.
	OamSample samples[64] = {};
	for(int pixelIndex = 0;
		pixelIndex < NesConstants::ScreenPixelCount;
		pixelIndex++
	) {
		HdPpuPixelInfo& pixel = info->ScreenTiles[pixelIndex];
		int16_t pixelX = (int16_t)(pixelIndex & 0xFF);
		int16_t pixelY = (int16_t)(pixelIndex >> 8);
		for(uint8_t spriteIndex = 0;
			spriteIndex < pixel.SpriteCount;
			spriteIndex++
		) {
			HdPpuTileInfo& sprite = pixel.Sprite[spriteIndex];
			uint8_t root = sprite.RootSpriteIndex;
			if(root >= 64 || samples[root].Visible) {
				continue;
			}

			OamSample& sample = samples[root];
			sample.Visible = true;
			sample.PaletteColors = sprite.PaletteColors;
			sample.TileHash = sprite.GetHashCode();
			sample.TileIndex = sprite.TileIndex;
			sample.OriginX = pixelX - sprite.OffsetX;
			sample.OriginY = pixelY - sprite.OffsetY;
			sample.HorizontalMirror = sprite.HorizontalMirroring;
			sample.VerticalMirror = sprite.VerticalMirroring;
		}
	}

	struct GroupCandidate {
		uint8_t Start = 0;
		uint8_t Size = 0;
		uint8_t ActiveCount = 0;
		uint8_t VisibleCount = 0;
		int16_t AnchorX = 0;
		int16_t AnchorY = 0;
	};

	GroupCandidate groupCandidates[128] = {};
	int groupCandidateCount = 0;

	struct GroupLayout {
		uint8_t Size;
		uint8_t Width;
		uint8_t Height;
		uint8_t MinimumActive;
	};
	const GroupLayout groupLayouts[] = {
		{ 6, 2, 3, 4 },
		{ 4, 2, 2, 4 }
	};

	// Validate each small cyclic OAM window by its actual screen-space grid,
	// not by the order of members inside that window. SMB-family games rotate
	// object OAM placement, and individual metasprite drawing routines are free
	// to emit TL/TR/BL/BR, column-major, reversed, or another permutation.
	for(const GroupLayout& layout : groupLayouts) {
		for(int start = 0; start < 64; start++) {
			int16_t memberX[8] = {};
			int16_t memberY[8] = {};
			uint8_t activeCount = 0;
			uint8_t visibleCount = 0;
			int16_t minX = 32767;
			int16_t minY = 32767;
			int16_t maxX = -32768;
			int16_t maxY = -32768;

			for(uint8_t slot = 0; slot < layout.Size; slot++) {
				uint8_t member = (uint8_t)((start + slot) & 0x3F);
				int16_t rawX;
				int16_t rawY;
				if(samples[member].Visible) {
					rawX = samples[member].OriginX;
					rawY = samples[member].OriginY;
					visibleCount++;
				} else {
					uint8_t oamY = _spriteRam[member << 2];
					if(oamY >= 0xEF) {
						continue;
					}
					rawX = _spriteRam[(member << 2) + 3];
					rawY = (int16_t)oamY + 1;
				}

				memberX[activeCount] = rawX;
				memberY[activeCount] = rawY;
				activeCount++;
				minX = std::min(minX, rawX);
				minY = std::min(minY, rawY);
				maxX = std::max(maxX, rawX);
				maxY = std::max(maxY, rawY);
			}

			if(
				activeCount < layout.MinimumActive
				|| visibleCount == 0
				|| maxX - minX != (layout.Width - 1) * 8
				|| maxY - minY != (layout.Height - 1) * 8
			) {
				continue;
			}

			uint8_t occupiedCells = 0;
			bool valid = true;
			for(uint8_t i = 0; i < activeCount; i++) {
				int dx = memberX[i] - minX;
				int dy = memberY[i] - minY;
				if(
					dx < 0 || dy < 0
					|| (dx & 7) != 0 || (dy & 7) != 0
					|| dx / 8 >= layout.Width
					|| dy / 8 >= layout.Height
				) {
					valid = false;
					break;
				}

				uint8_t cell = (uint8_t)(
					(dy / 8) * layout.Width + (dx / 8)
				);
				uint8_t bit = (uint8_t)(1u << cell);
				if((occupiedCells & bit) != 0) {
					valid = false;
					break;
				}
				occupiedCells |= bit;
			}

			uint8_t completeMask = (uint8_t)((1u << layout.Size) - 1u);
			if(
				!valid
				|| (layout.Size == 4 && occupiedCells != completeMask)
				|| groupCandidateCount >= 128
			) {
				continue;
			}

			GroupCandidate& candidate =
				groupCandidates[groupCandidateCount++];
			candidate.Start = (uint8_t)start;
			candidate.Size = layout.Size;
			candidate.ActiveCount = activeCount;
			candidate.VisibleCount = visibleCount;
			candidate.AnchorX = minX;
			candidate.AnchorY = minY;
		}
	}

	std::sort(
		groupCandidates,
		groupCandidates + groupCandidateCount,
		[](const GroupCandidate& a, const GroupCandidate& b) {
			if(a.ActiveCount != b.ActiveCount) {
				return a.ActiveCount > b.ActiveCount;
			}
			if(a.VisibleCount != b.VisibleCount) {
				return a.VisibleCount > b.VisibleCount;
			}
			if(a.Size != b.Size) {
				return a.Size > b.Size;
			}
			return a.Start < b.Start;
		}
	);

	struct CurrentObject {
		uint8_t Members[8] = {};
		uint8_t MemberCount = 0;
		uint8_t GroupSize = 0;
		uint8_t MinMember = 0xFF;
		int16_t AnchorX = 0;
		int16_t AnchorY = 0;
		uint16_t SpatialKey = 0;
		uint64_t CoarseSignature = 0;
		uint32_t DetailSignature = 0;
		uint32_t TrackId = 0;
		uint64_t PriorityKey = 0xFFFFFFFFFFFFFFFFull;
	};

	CurrentObject currentObjects[64] = {};
	int currentObjectCount = 0;
	bool assignedOam[64] = {};

	auto makeSpatialKey = [](int16_t x, int16_t y) {
		uint8_t keyX = (uint8_t)std::max(0, std::min(255, (int)x));
		uint8_t keyY = (uint8_t)std::max(0, std::min(255, (int)y));
		return (uint16_t)(((uint16_t)keyX << 8) | keyY);
	};

	auto addCurrentObject = [&](const uint8_t* members,
		uint8_t memberCount, uint8_t groupSize,
		int16_t anchorX, int16_t anchorY) {
		if(memberCount == 0 || currentObjectCount >= 64) {
			return;
		}

		uint8_t orderedMembers[8] = {};
		for(uint8_t i = 0; i < memberCount; i++) {
			orderedMembers[i] = members[i];
		}
		std::sort(
			orderedMembers,
			orderedMembers + memberCount,
			[&](uint8_t a, uint8_t b) {
				if(samples[a].OriginY != samples[b].OriginY) {
					return samples[a].OriginY < samples[b].OriginY;
				}
				if(samples[a].OriginX != samples[b].OriginX) {
					return samples[a].OriginX < samples[b].OriginX;
				}
				return a < b;
			}
		);

		CurrentObject& object = currentObjects[currentObjectCount++];
		object.MemberCount = memberCount;
		object.GroupSize = groupSize;
		object.AnchorX = anchorX;
		object.AnchorY = anchorY;
		object.SpatialKey = makeSpatialKey(anchorX, anchorY);
		object.CoarseSignature = StableFnvOffset;
		uint64_t detailHash = StableFnvOffset;

		StableMix(object.CoarseSignature, groupSize);
		StableMix(object.CoarseSignature, memberCount);
		for(uint8_t i = 0; i < memberCount; i++) {
			uint8_t member = orderedMembers[i];
			object.Members[i] = member;
			object.MinMember = std::min(object.MinMember, member);

			// Relative cell position makes the signature independent of OAM
			// permutation while preserving the visual arrangement.
			StableMix(
				object.CoarseSignature,
				(uint16_t)(samples[member].OriginX - anchorX)
			);
			StableMix(
				object.CoarseSignature,
				(uint16_t)(samples[member].OriginY - anchorY)
			);
			StableMix(object.CoarseSignature, samples[member].PaletteColors);
			StableMix(
				object.CoarseSignature,
				(uint32_t)samples[member].TileIndex
			);
			StableMix(detailHash, samples[member].TileHash);
			StableMix(
				detailHash,
				samples[member].HorizontalMirror ? 1 : 0
			);
			StableMix(
				detailHash,
				samples[member].VerticalMirror ? 1 : 0
			);
		}
		object.DetailSignature =
			(uint32_t)(detailHash ^ (detailHash >> 32));
	};

	for(int candidateIndex = 0;
		candidateIndex < groupCandidateCount;
		candidateIndex++
	) {
		const GroupCandidate& candidate = groupCandidates[candidateIndex];
		uint8_t members[8] = {};
		uint8_t memberCount = 0;
		bool conflicts = false;

		for(int slot = 0; slot < candidate.Size; slot++) {
			uint8_t member =
				(uint8_t)((candidate.Start + slot) & 0x3F);
			if(!samples[member].Visible) {
				continue;
			}
			if(assignedOam[member]) {
				conflicts = true;
				break;
			}
			members[memberCount++] = member;
		}

		if(conflicts || memberCount == 0) {
			continue;
		}
		for(uint8_t i = 0; i < memberCount; i++) {
			assignedOam[members[i]] = true;
		}
		addCurrentObject(
			members,
			memberCount,
			candidate.Size,
			candidate.AnchorX,
			candidate.AnchorY
		);
	}

	for(uint8_t oamIndex = 0; oamIndex < 64; oamIndex++) {
		if(!samples[oamIndex].Visible || assignedOam[oamIndex]) {
			continue;
		}
		uint8_t member = oamIndex;
		addCurrentObject(
			&member,
			1,
			1,
			samples[oamIndex].OriginX,
			samples[oamIndex].OriginY
		);
	}

	// Canonicalize current-object order before temporal matching. OAM remains
	// only the final tie-break for visually and spatially indistinguishable data.
	std::sort(
		currentObjects,
		currentObjects + currentObjectCount,
		[](const CurrentObject& a, const CurrentObject& b) {
			if(a.SpatialKey != b.SpatialKey) {
				return a.SpatialKey < b.SpatialKey;
			}
			if(a.GroupSize != b.GroupSize) {
				return a.GroupSize > b.GroupSize;
			}
			if(a.CoarseSignature != b.CoarseSignature) {
				return a.CoarseSignature < b.CoarseSignature;
			}
			if(a.DetailSignature != b.DetailSignature) {
				return a.DetailSignature < b.DetailSignature;
			}
			return a.MinMember < b.MinMember;
		}
	);

	struct MatchCandidate {
		uint32_t Cost = 0;
		uint32_t TrackId = 0;
		uint8_t TrackIndex = 0;
		uint8_t CurrentIndex = 0;
		uint16_t CurrentSpatialKey = 0;
		uint64_t CurrentSignature = 0;
	};

	MatchCandidate matchCandidates[64 * 64] = {};
	int matchCandidateCount = 0;
	for(uint8_t trackIndex = 0; trackIndex < 64; trackIndex++) {
		StableObjectTrack& track = _stableObjectTracks[trackIndex];
		if(!track.Active || track.LastFrame + 1 != _frameCount) {
			continue;
		}

		int predictedX = track.X + track.VelocityX;
		int predictedY = track.Y + track.VelocityY;
		for(uint8_t currentIndex = 0;
			currentIndex < currentObjectCount;
			currentIndex++
		) {
			CurrentObject& current = currentObjects[currentIndex];
			if(track.GroupSize != current.GroupSize) {
				continue;
			}

			int distance =
				std::abs((int)current.AnchorX - predictedX)
				+ std::abs((int)current.AnchorY - predictedY);
			if(distance > 48) {
				continue;
			}

			uint32_t cost = (uint32_t)distance * 64u;
			cost += (uint32_t)std::abs(
				(int)track.MemberCount - (int)current.MemberCount
			) * 128u;
			if(track.CoarseSignature != current.CoarseSignature) {
				cost += 1024u;
			}
			if(track.DetailSignature != current.DetailSignature) {
				cost += 128u;
			}

			MatchCandidate& candidate =
				matchCandidates[matchCandidateCount++];
			candidate.Cost = cost;
			candidate.TrackId = track.Id;
			candidate.TrackIndex = trackIndex;
			candidate.CurrentIndex = currentIndex;
			candidate.CurrentSpatialKey = current.SpatialKey;
			candidate.CurrentSignature = current.CoarseSignature;
		}
	}

	std::sort(
		matchCandidates,
		matchCandidates + matchCandidateCount,
		[](const MatchCandidate& a, const MatchCandidate& b) {
			if(a.Cost != b.Cost) {
				return a.Cost < b.Cost;
			}
			if(a.TrackId != b.TrackId) {
				return a.TrackId < b.TrackId;
			}
			if(a.CurrentSpatialKey != b.CurrentSpatialKey) {
				return a.CurrentSpatialKey < b.CurrentSpatialKey;
			}
			if(a.CurrentSignature != b.CurrentSignature) {
				return a.CurrentSignature < b.CurrentSignature;
			}
			return a.CurrentIndex < b.CurrentIndex;
		}
	);

	bool trackMatched[64] = {};
	bool currentMatched[64] = {};
	for(int candidateIndex = 0;
		candidateIndex < matchCandidateCount;
		candidateIndex++
	) {
		MatchCandidate& candidate = matchCandidates[candidateIndex];
		if(
			trackMatched[candidate.TrackIndex]
			|| currentMatched[candidate.CurrentIndex]
		) {
			continue;
		}

		StableObjectTrack& track =
			_stableObjectTracks[candidate.TrackIndex];
		CurrentObject& current =
			currentObjects[candidate.CurrentIndex];
		int deltaX = (int)current.AnchorX - track.X;
		int deltaY = (int)current.AnchorY - track.Y;
		track.VelocityX = (int8_t)StableClampVelocity(deltaX);
		track.VelocityY = (int8_t)StableClampVelocity(deltaY);
		track.X = current.AnchorX;
		track.Y = current.AnchorY;
		track.GroupSize = current.GroupSize;
		track.MemberCount = current.MemberCount;
		track.CoarseSignature = current.CoarseSignature;
		track.DetailSignature = current.DetailSignature;
		track.LastFrame = _frameCount;

		current.TrackId = track.Id;
		current.PriorityKey = track.PriorityKey;
		trackMatched[candidate.TrackIndex] = true;
		currentMatched[candidate.CurrentIndex] = true;
	}

	for(uint8_t currentIndex = 0;
		currentIndex < currentObjectCount;
		currentIndex++
	) {
		if(currentMatched[currentIndex]) {
			continue;
		}

		int slot = -1;
		uint32_t oldestFrame = 0xFFFFFFFFu;
		uint32_t oldestId = 0xFFFFFFFFu;
		for(int trackIndex = 0; trackIndex < 64; trackIndex++) {
			StableObjectTrack& track = _stableObjectTracks[trackIndex];
			if(!track.Active) {
				slot = trackIndex;
				break;
			}
			if(
				!trackMatched[trackIndex]
				&& (
					track.LastFrame < oldestFrame
					|| (
						track.LastFrame == oldestFrame
						&& track.Id < oldestId
					)
				)
			) {
				oldestFrame = track.LastFrame;
				oldestId = track.Id;
				slot = trackIndex;
			}
		}
		if(slot < 0) {
			continue;
		}

		CurrentObject& current = currentObjects[currentIndex];
		StableObjectTrack& track = _stableObjectTracks[slot];
		track = {};
		track.Active = true;
		track.Id = _nextStableObjectTrackId++;
		if(_nextStableObjectTrackId == 0) {
			_nextStableObjectTrackId = 1;
		}
		track.PriorityKey =
			((uint64_t)current.SpatialKey << 32) | track.Id;
		track.CoarseSignature = current.CoarseSignature;
		track.DetailSignature = current.DetailSignature;
		track.X = current.AnchorX;
		track.Y = current.AnchorY;
		track.GroupSize = current.GroupSize;
		track.MemberCount = current.MemberCount;
		track.LastFrame = _frameCount;

		current.TrackId = track.Id;
		current.PriorityKey = track.PriorityKey;
		trackMatched[slot] = true;
		currentMatched[currentIndex] = true;
	}

	for(int trackIndex = 0; trackIndex < 64; trackIndex++) {
		StableObjectTrack& track = _stableObjectTracks[trackIndex];
		if(track.Active && track.LastFrame != _frameCount) {
			track.Active = false;
		}
	}

	for(int objectIndex = 0;
		objectIndex < currentObjectCount;
		objectIndex++
	) {
		CurrentObject& object = currentObjects[objectIndex];
		for(uint8_t memberIndex = 0;
			memberIndex < object.MemberCount;
			memberIndex++
		) {
			uint8_t member = object.Members[memberIndex];
			info->StableObjectPriorityByOam[member] =
				object.PriorityKey;
			info->StableObjectSortKeyByOam[member] =
				object.SpatialKey;
		}
	}
}

HdNesPpu::HdNesPpu(NesConsole* console, HdPackData* hdData) : NesPpu(console)
{
	_hdData = hdData;
	_version = _hdData->Version;
	_isChrRam = !_console->GetMapper()->HasChrRom();
	_screenInfo[0] = new HdScreenInfo(_isChrRam);
	_screenInfo[1] = new HdScreenInfo(_isChrRam);
	_info = _screenInfo[0];
	_forceRemoveSpriteLimit = (_hdData->OptionFlags & (int)HdPackOptions::NoSpriteLimit) != 0;
}

HdNesPpu::~HdNesPpu()
{
	delete _screenInfo[0];
	delete _screenInfo[1];
}

void* HdNesPpu::OnBeforeSendFrame()
{
	HdScreenInfo* info = _info;
	FinalizeStableObjectPriorities(info);	info->FrameNumber = _frameCount;
	info->WatchedAddressValues.clear();
	for(uint32_t address : _hdData->WatchedMemoryAddresses) {
		if(address & HdPackBaseMemoryCondition::PpuMemoryMarker) {
			if((address & 0x3FFF) >= 0x3F00) {
				info->WatchedAddressValues[address] = ReadPaletteRam(address);
			} else {
				info->WatchedAddressValues[address] = _console->GetMapper()->DebugReadVram(address & 0x3FFF, true);
			}
		} else {
			info->WatchedAddressValues[address] = _console->GetMemoryManager()->DebugRead(address);
		}
	}

	_info = (_info == _screenInfo[0]) ? _screenInfo[1] : _screenInfo[0];

	return info;
}
