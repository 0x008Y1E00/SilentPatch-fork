#include "StdAfx.h"

#include "Timer.h"

struct RsGlobalType
{
	const char*		AppName;
	unsigned int	unkWidth, unkHeight;
	signed int		MaximumWidth;
	signed int		MaximumHeight;
	unsigned int	frameLimit;
	BOOL			quit;
	void*			ps;
	void*			keyboard;
	void*			mouse;
	void*			pad;
};

bool*					bSnapShotActive;
static const void*		RosieAudioFix_JumpBack;


void (__stdcall *AudioResetTimers)(unsigned int);
static void (*PrintString)(float,float,const wchar_t*);

static RsGlobalType*	RsGlobal;
static const float*		ResolutionWidthMult;
static const float*		ResolutionHeightMult;
static const void*		SubtitlesShadowFix_JumpBack;


void __declspec(naked) RosiesAudioFix()
{
	_asm
	{
		mov     byte ptr [ebx+0CCh], 0
		mov     byte ptr [ebx+148h], 0
		jmp		[RosieAudioFix_JumpBack]
	}
}

void __stdcall Recalculate(float& fX, float& fY, signed int nShadow)
{
	fX = nShadow * *ResolutionWidthMult * RsGlobal->MaximumWidth;
	fY = nShadow * *ResolutionHeightMult * RsGlobal->MaximumHeight;
}

template<int pFltX, int pFltY>
void AlteredPrintString(float fX, float fY, const wchar_t* pText)
{
	float	fMarginX = **reinterpret_cast<float**>(pFltX);
	float	fMarginY = **reinterpret_cast<float**>(pFltY);
	PrintString(fX - fMarginX + (fMarginX * *ResolutionWidthMult * RsGlobal->MaximumWidth), fY - fMarginY + (fMarginY * *ResolutionHeightMult * RsGlobal->MaximumHeight), pText);
}

template<int pFltX, int pFltY>
void AlteredPrintStringMinus(float fX, float fY, const wchar_t* pText)
{
	float	fMarginX = **reinterpret_cast<float**>(pFltX);
	float	fMarginY = **reinterpret_cast<float**>(pFltY);
	PrintString(fX + fMarginX - (fMarginX * *ResolutionWidthMult * RsGlobal->MaximumWidth), fY + fMarginY - (fMarginY * *ResolutionHeightMult * RsGlobal->MaximumHeight), pText);
}

template<int pFltX>
void AlteredPrintStringXOnly(float fX, float fY, const wchar_t* pText)
{
	float	fMarginX = **reinterpret_cast<float**>(pFltX);
	PrintString(fX - fMarginX + (fMarginX * *ResolutionWidthMult * RsGlobal->MaximumWidth), fY, pText);
}

template<int pFltY>
void AlteredPrintStringYOnly(float fX, float fY, const wchar_t* pText)
{
	float	fMarginY = **reinterpret_cast<float**>(pFltY);
	PrintString(fX, fY - fMarginY + (fMarginY * *ResolutionHeightMult * RsGlobal->MaximumHeight), pText);
}

float FixedRefValue()
{
	return 1.0f;
}

void __declspec(naked) SubtitlesShadowFix()
{
	_asm
	{
		mov		[esp], eax
		fild	[esp]
		push	eax
		lea		eax, [esp+20h-18h]
		push	eax
		lea		eax, [esp+24h-14h]
		push	eax
		call	Recalculate
		jmp		SubtitlesShadowFix_JumpBack
	}
}


void Patch_VC_10()
{
	using namespace MemoryVP;

	AudioResetTimers = (void(__stdcall*)(unsigned int))0x5F98D0;
	PrintString = (void(*)(float,float,const wchar_t*))0x551040;

	bSnapShotActive = *(bool**)0x4D1239;
	RsGlobal = *(RsGlobalType**)0x602D32;
	ResolutionWidthMult = *(float**)0x5FA15E;
	ResolutionHeightMult = *(float**)0x5FA148;
	RosieAudioFix_JumpBack = (void*)0x42BFFE;
	SubtitlesShadowFix_JumpBack = (void*)0x551701;

	CTimer::ms_fTimeScale = *(float**)0x453D38;
	CTimer::ms_fTimeStep = *(float**)0x41A318;
	CTimer::ms_fTimeStepNotClipped = *(float**)0x40605B;
	CTimer::m_UserPause = *(bool**)0x4D0F91;
	CTimer::m_CodePause = *(bool**)0x4D0FAE;
	CTimer::m_snTimeInMilliseconds = *(int**)0x418CFC;
	CTimer::m_snPreviousTimeInMilliseconds = *(int**)0x41BB3A;
	CTimer::m_snTimeInMillisecondsNonClipped = *(int**)0x4D1081;
	CTimer::m_snTimeInMillisecondsPauseMode = *(int**)0x4D0FE2;
	CTimer::m_FrameCounter = *(unsigned int**)0x4D12CF;

	Patch<BYTE>(0x43E983, 16);
	Patch<BYTE>(0x43EC03, 16);
	Patch<BYTE>(0x43EECB, 16);
	Patch<BYTE>(0x43F52B, 16);
	Patch<BYTE>(0x43F842, 16);
	Patch<BYTE>(0x48EB27, 16);
	Patch<BYTE>(0x541E7E, 16);

	InjectHook(0x4D1300, CTimer::Initialise, PATCH_JUMP);
	InjectHook(0x4D0ED0, CTimer::Suspend, PATCH_JUMP);
	InjectHook(0x4D0E50, CTimer::Resume, PATCH_JUMP);
	InjectHook(0x4D0DF0, CTimer::GetCyclesPerFrame, PATCH_JUMP);
	InjectHook(0x4D0E30, CTimer::GetCyclesPerMillisecond, PATCH_JUMP);
	InjectHook(0x4D0F30, CTimer::Update, PATCH_JUMP);
	InjectHook(0x61AA7D, CTimer::RecoverFromSave);

	InjectHook(0x5433BD, FixedRefValue);

	InjectHook(0x42BFF7, RosiesAudioFix, PATCH_JUMP);

	InjectHook(0x5516FC, SubtitlesShadowFix, PATCH_JUMP);
	Patch<BYTE>(0x5517C4, 0xD9);
	Patch<BYTE>(0x5517DF, 0xD9);
	Patch<BYTE>(0x551832, 0xD9);
	Patch<BYTE>(0x551848, 0xD9);
	Patch<BYTE>(0x5517E2, 0x34-0x14);
	Patch<BYTE>(0x55184B, 0x34-0x14);
	Patch<BYTE>(0x5517C7, 0x28-0x18);
	Patch<BYTE>(0x551835, 0x24-0x18);
	Patch<BYTE>(0x5516FB, 0x90);

	InjectHook(0x5FA1FD, AlteredPrintString<0x5FA1F6,0x5FA1D5>);
	InjectHook(0x54474D, AlteredPrintStringMinus<0x544727,0x544727>);
}

void Patch_VC_11()
{
	using namespace MemoryVP;

	AudioResetTimers = (void(__stdcall*)(unsigned int))0x5F98F0;
	PrintString = (void(*)(float,float,const wchar_t*))0x551060;

	bSnapShotActive = *(bool**)0x4D1259;
	RsGlobal = *(RsGlobalType**)0x602D12;
	ResolutionWidthMult = *(float**)0x5FA17E;
	ResolutionHeightMult = *(float**)0x5FA168;
	RosieAudioFix_JumpBack = (void*)0x42BFFE;
	SubtitlesShadowFix_JumpBack = (void*)0x551721;

	CTimer::ms_fTimeScale = *(float**)0x453D38;
	CTimer::ms_fTimeStep = *(float**)0x41A318;
	CTimer::ms_fTimeStepNotClipped = *(float**)0x40605B;
	CTimer::m_UserPause = *(bool**)0x4D0FB1;
	CTimer::m_CodePause = *(bool**)0x4D0FCE;
	CTimer::m_snTimeInMilliseconds = *(int**)0x418CFC;
	CTimer::m_snPreviousTimeInMilliseconds = *(int**)0x41BB3A;
	CTimer::m_snTimeInMillisecondsNonClipped = *(int**)0x4D10A1;
	CTimer::m_snTimeInMillisecondsPauseMode = *(int**)0x4D1002;
	CTimer::m_FrameCounter = *(unsigned int**)0x4D12EF;

	Patch<BYTE>(0x43E983, 16);
	Patch<BYTE>(0x43EC03, 16);
	Patch<BYTE>(0x43EECB, 16);
	Patch<BYTE>(0x43F52B, 16);
	Patch<BYTE>(0x43F842, 16);
	Patch<BYTE>(0x48EB37, 16);
	Patch<BYTE>(0x541E9E, 16);

	InjectHook(0x4D1320, CTimer::Initialise, PATCH_JUMP);
	InjectHook(0x4D0EF0, CTimer::Suspend, PATCH_JUMP);
	InjectHook(0x4D0E70, CTimer::Resume, PATCH_JUMP);
	InjectHook(0x4D0E10, CTimer::GetCyclesPerFrame, PATCH_JUMP);
	InjectHook(0x4D0E50, CTimer::GetCyclesPerMillisecond, PATCH_JUMP);
	InjectHook(0x4D0F50, CTimer::Update, PATCH_JUMP);
	InjectHook(0x61AA5D, CTimer::RecoverFromSave);

	InjectHook(0x5433DD, FixedRefValue);

	InjectHook(0x42BFF7, RosiesAudioFix, PATCH_JUMP);

	InjectHook(0x55171C, SubtitlesShadowFix, PATCH_JUMP);
	Patch<BYTE>(0x5517E4, 0xD9);
	Patch<BYTE>(0x5517FF, 0xD9);
	Patch<BYTE>(0x551852, 0xD9);
	Patch<BYTE>(0x551868, 0xD9);
	Patch<BYTE>(0x551802, 0x34-0x14);
	Patch<BYTE>(0x55186B, 0x34-0x14);
	Patch<BYTE>(0x5517E7, 0x28-0x18);
	Patch<BYTE>(0x551855, 0x24-0x18);
	Patch<BYTE>(0x55171B, 0x90);

	InjectHook(0x5FA21D, AlteredPrintString<0x5FA216,0x5FA1F5>);
	InjectHook(0x54476D, AlteredPrintStringMinus<0x544747,0x544747>);
}

void Patch_VC_Steam()
{
	using namespace MemoryVP;

	AudioResetTimers = (void(__stdcall*)(unsigned int))0x5F9530;
	PrintString = (void(*)(float,float,const wchar_t*))0x550F30;

	bSnapShotActive = *(bool**)0x4D10F9;
	RsGlobal = *(RsGlobalType**)0x602952;
	ResolutionWidthMult = *(float**)0x5F9DBE;
	ResolutionHeightMult = *(float**)0x5F9DA8;
	RosieAudioFix_JumpBack = (void*)0x42BFCE;
	SubtitlesShadowFix_JumpBack = (void*)0x5515F1;

	CTimer::ms_fTimeScale = *(float**)0x453C18;
	CTimer::ms_fTimeStep = *(float**)0x41A318;
	CTimer::ms_fTimeStepNotClipped = *(float**)0x40605B;
	CTimer::m_UserPause = *(bool**)0x4D0E51;
	CTimer::m_CodePause = *(bool**)0x4D0E6E;
	CTimer::m_snTimeInMilliseconds = *(int**)0x418CFC;
	CTimer::m_snPreviousTimeInMilliseconds = *(int**)0x41BB3A;
	CTimer::m_snTimeInMillisecondsNonClipped = *(int**)0x4D0F41;
	CTimer::m_snTimeInMillisecondsPauseMode = *(int**)0x4D0EA2;
	CTimer::m_FrameCounter = *(unsigned int**)0x4D118F;

	Patch<BYTE>(0x43E8F3, 16);
	Patch<BYTE>(0x43EB73, 16);
	Patch<BYTE>(0x43EE3B, 16);
	Patch<BYTE>(0x43F49B, 16);
	Patch<BYTE>(0x43F7B2, 16);
	Patch<BYTE>(0x48EA37, 16);
	Patch<BYTE>(0x541D6E, 16);

	InjectHook(0x4D11C0, CTimer::Initialise, PATCH_JUMP);
	InjectHook(0x4D0D90, CTimer::Suspend, PATCH_JUMP);
	InjectHook(0x4D0D10, CTimer::Resume, PATCH_JUMP);
	InjectHook(0x4D0CB0, CTimer::GetCyclesPerFrame, PATCH_JUMP);
	InjectHook(0x4D0CF0, CTimer::GetCyclesPerMillisecond, PATCH_JUMP);
	InjectHook(0x4D0DF0, CTimer::Update, PATCH_JUMP);
	InjectHook(0x61A6A6, CTimer::RecoverFromSave);

	InjectHook(0x5432AD, FixedRefValue);

	InjectHook(0x42BFC7, RosiesAudioFix, PATCH_JUMP);

	InjectHook(0x5515EC, SubtitlesShadowFix, PATCH_JUMP);
	Patch<BYTE>(0x5516B4, 0xD9);
	Patch<BYTE>(0x5516CF, 0xD9);
	Patch<BYTE>(0x551722, 0xD9);
	Patch<BYTE>(0x551738, 0xD9);
	Patch<BYTE>(0x5516D2, 0x34-0x14);
	Patch<BYTE>(0x55173B, 0x34-0x14);
	Patch<BYTE>(0x5516B7, 0x28-0x18);
	Patch<BYTE>(0x551725, 0x24-0x18);
	Patch<BYTE>(0x5515EB, 0x90);

	InjectHook(0x5F9E5D, AlteredPrintString<0x5F9E56,0x5F9E35>);
	InjectHook(0x54463D, AlteredPrintStringMinus<0x544617,0x544617>);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	UNREFERENCED_PARAMETER(hinstDLL);
	UNREFERENCED_PARAMETER(lpvReserved);

	if ( fdwReason == DLL_PROCESS_ATTACH )
	{
		if(*(DWORD*)0x667BF0 == 0x53E58955) Patch_VC_10();
		else if(*(DWORD*)0x667C40 == 0x53E58955) Patch_VC_11();
		else if (*(DWORD*)0x666BA0 == 0x53E58955) Patch_VC_Steam();
		else return FALSE;

		CTimer::Initialise();
	}
	return TRUE;
}