#pragma warning(disable:4458) // declaration hides class member
#pragma warning(disable:4201) // nonatandard extension user: nameless struct/union
#pragma warning(disable:4100) // unreferenced formal parameter

#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES

#define NOMINMAX
#define WINVER 0x0502
#define _WIN32_WINNT 0x0502

#include <windows.h>
#include <cassert>

#define RwEngineInstance (*rwengine)

#include <rwcore.h>
#include <rphanim.h>
#include <rtpng.h>

#include "resource.h"

#include "MemoryMgr.h"
#include "Maths.h"
#include "rwpred.hpp"

// SA operator delete
extern void	(*GTAdelete)(void* data);
extern const char* (*GetFrameNodeName)(RwFrame*);
extern RpHAnimHierarchy* (*GetAnimHierarchyFromSkinClump)(RpClump*);
int32_t Int32Rand();

extern unsigned char& nGameClockDays;
extern unsigned char& nGameClockMonths;

#define DISABLE_FLA_DONATION_WINDOW		0
