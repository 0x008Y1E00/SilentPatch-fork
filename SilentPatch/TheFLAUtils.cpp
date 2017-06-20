#include "TheFLAUtils.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

int32_t (*FLAUtils::GetExtendedIDFunc)(const void* ptr) = nullptr;

void FLAUtils::Init()
{
	HMODULE hFLA = GetModuleHandle("$fastman92limitAdjuster.asi");
	if ( hFLA != nullptr )
	{
		GetExtendedIDFunc = reinterpret_cast<decltype(GetExtendedIDFunc)>(GetProcAddress( hFLA, "GetExtendedIDfrom16bitBefore" ));
	}
}