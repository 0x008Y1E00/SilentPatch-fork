#define WIN32_LEAN_AND_MEAN

#define WINVER 0x0501
#define _WIN32_WINNT 0x0501
#define NOMINMAX

#include <windows.h>
#include <cassert>
#include <cstdio>

#include "Utils/MemoryMgr.h"
#include "Utils/MemoryMgr.GTA.h"
#include "Utils/Patterns.h"

#define DISABLE_FLA_DONATION_WINDOW		0
