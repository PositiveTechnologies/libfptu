#pragma once

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)

#define WIN32_LEAN_AND_MEAN
#define NOCOMM
#define NOMINMAX

#if !defined(WINVER) || !defined(_WIN32_WINNT)
#define WINVER 0x0502
#define _WIN32_WINNT 0x0502
#endif // !(WINVER) || !defined(_WIN32_WINNT)

#ifdef DEFINE_ENUM_FLAG_OPERATORS
#undef DEFINE_ENUM_FLAG_OPERATORS
#endif

#include "warnings_push_system.h"

#include <windows.h>

#include "warnings_pop.h"
#endif /* Windows */
