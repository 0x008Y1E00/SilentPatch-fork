#pragma once

#include <cstdint>

namespace ExtraCompSpecularity
{
	void ReadExtraCompSpecularityExceptions(const wchar_t* pPath);
	bool SpecularityExcluded(int32_t modelID);
}
namespace Common
{
	namespace Patches
	{
		void III_VC_DelayedCommon( bool hasDebugMenu, const wchar_t* iniPath );
		void III_VC_Common();
		void III_VC_SetDelayedPatchesFunc( void(*func)() );
	}
};