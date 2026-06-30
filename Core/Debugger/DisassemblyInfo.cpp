#include "pch.h"
#include "Debugger.h"
#include <algorithm>
#include "Debugger/DisassemblyInfo.h"
#include "Debugger/MemoryDumper.h"
#include "Debugger/DebugUtilities.h"
#include "Utilities/HexUtilities.h"
#include "Utilities/FastString.h"
#include "NES/Debugger/NesDisUtils.h"
#include "Shared/EmuSettings.h"

DisassemblyInfo::DisassemblyInfo()
{
}

DisassemblyInfo::DisassemblyInfo(uint32_t cpuAddress, uint8_t cpuFlags, CpuType cpuType, MemoryType memType, MemoryDumper* memoryDumper)
{
	Initialize(cpuAddress, cpuFlags, cpuType, memType, memoryDumper);
}

void DisassemblyInfo::Initialize(uint32_t cpuAddress, uint8_t cpuFlags, CpuType cpuType, MemoryType memType, MemoryDumper* memoryDumper)
{
	_cpuType = cpuType;
	_flags = cpuFlags;

	_byteCode[0] = memoryDumper->GetMemoryValue(memType, cpuAddress);

	_opSize = GetOpSize(_byteCode[0], _flags, _cpuType, cpuAddress, memType, memoryDumper);

	for(int i = 1; i < _opSize && i < 8; i++) {
		_byteCode[i] = memoryDumper->GetMemoryValue(memType, cpuAddress + i);
	}

	_initialized = true;
}

bool DisassemblyInfo::IsInitialized()
{
	return _initialized;
}

bool DisassemblyInfo::IsValid(uint8_t cpuFlags)
{
	return _flags == cpuFlags;
}

void DisassemblyInfo::Reset()
{
	_initialized = false;
}

void DisassemblyInfo::GetDisassembly(string& out, uint32_t memoryAddr, LabelManager* labelManager, EmuSettings* settings)
{
	if(_cpuType != CpuType::Nes) {
		throw std::runtime_error("Unsupported CPU type in NES-only Libretro core");
	}
	NesDisUtils::GetDisassembly(*this, out, memoryAddr, labelManager, settings);
}

EffectiveAddressInfo DisassemblyInfo::GetEffectiveAddress(Debugger* debugger, void* cpuState, CpuType cpuType)
{
	if(_cpuType != CpuType::Nes) {
		return {};
	}
	return NesDisUtils::GetEffectiveAddress(
		*this, *(NesCpuState*)cpuState, debugger->GetMemoryDumper()
	);
}

CpuType DisassemblyInfo::GetCpuType()
{
	return _cpuType;
}

uint8_t DisassemblyInfo::GetOpCode()
{
	return _byteCode[0];
}

template<CpuType type>
uint32_t DisassemblyInfo::GetFullOpCode()
{
	return _byteCode[0];
}

uint8_t DisassemblyInfo::GetOpSize()
{
	return _opSize;
}

uint8_t DisassemblyInfo::GetFlags()
{
	return _flags;
}

uint8_t* DisassemblyInfo::GetByteCode()
{
	return _byteCode;
}

void DisassemblyInfo::GetByteCode(uint8_t copyBuffer[8])
{
	memcpy(copyBuffer, _byteCode, _opSize);
}

void DisassemblyInfo::GetByteCode(string& out)
{
	FastString str;
	for(int i = 0; i < _opSize; i++) {
		str.WriteAll('$', HexUtilities::ToHex(_byteCode[i]));
		if(i < _opSize - 1) {
			str.Write(' ');
		}
	}
	out += str.ToString();
}

uint8_t DisassemblyInfo::GetOpSize(uint32_t opCode, uint8_t flags, CpuType type, uint32_t cpuAddress, MemoryType memType, MemoryDumper* memoryDumper)
{
	if(type != CpuType::Nes) {
		throw std::runtime_error("Unsupported CPU type in NES-only Libretro core");
	}
	return NesDisUtils::GetOpSize(opCode);
}

bool DisassemblyInfo::IsJumpToSub()
{
	return _cpuType == CpuType::Nes
		? NesDisUtils::IsJumpToSub(GetOpCode())
		: false;
}

bool DisassemblyInfo::IsReturnInstruction()
{
	return _cpuType == CpuType::Nes
		? NesDisUtils::IsReturnInstruction(GetOpCode())
		: false;
}

bool DisassemblyInfo::CanDisassembleNextOp()
{
	return !IsUnconditionalJump();
}

bool DisassemblyInfo::IsUnconditionalJump()
{
	return _cpuType == CpuType::Nes
		? NesDisUtils::IsUnconditionalJump(GetOpCode())
		: false;
}

bool DisassemblyInfo::IsJump()
{
	if(_cpuType != CpuType::Nes) {
		return false;
	}
	return NesDisUtils::IsUnconditionalJump(GetOpCode())
		|| NesDisUtils::IsConditionalJump(GetOpCode());
}

void DisassemblyInfo::UpdateCpuFlags(uint8_t& cpuFlags)
{
	(void)cpuFlags;
}

uint32_t DisassemblyInfo::GetMemoryValue(EffectiveAddressInfo effectiveAddress, MemoryDumper* memoryDumper, MemoryType memType)
{
	MemoryType effectiveMemType = effectiveAddress.Type == MemoryType::None ? memType : effectiveAddress.Type;
	switch(effectiveAddress.ValueSize) {
		default:
		case 1: return memoryDumper->GetMemoryValue(effectiveMemType, effectiveAddress.Address);
		case 2: return memoryDumper->GetMemoryValue16(effectiveMemType, effectiveAddress.Address);
		case 4: return memoryDumper->GetMemoryValue32(effectiveMemType, effectiveAddress.Address);
	}
}
