#pragma once

#include <cinttypes>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
#   define OS_WINDOWS
#elif defined(MACOS)
#   define OS_MACOS
#else
#   define OS_LINUX
#endif

// Assert macro
void do_assert(const char* check, const char* file, s32 line, const char* fmt, ...);
[[noreturn]] void crash();

//#if _DEBUG
#   define debug_assert(E) do { if (!(E)) { do_assert(#E, __FILE__, __LINE__, ""); }} while(0)
#   define debug_assertf(E, F, ...) do { if (!(E)) { do_assert(#E, __FILE__, __LINE__, F, __VA_ARGS__); } } while(0)
//#else
//#   define debug_assert(E)
//#   define debug_assertf(E, F, ...)
//#endif

#if _DEBUG
#define SHIP 0
#else
#define SHIP 1
#endif