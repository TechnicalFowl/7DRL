
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void do_assert(const char* check, const char* file, s32 line, const char* fmt, ...) {
    printf("Assertion failed (%s) ", check);
    char* msg = nullptr;
    if (fmt[0]) {
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
        size_t len = strlen(fmt);
        if (len > 0 && fmt[len - 1] != '\n')
        {
            printf("\n");
        }
    }
    else
    {
        printf("\n");
    }
#if defined(OS_WINDOWS) && defined(_DEBUG)
    __debugbreak();
#endif
}

[[noreturn]] void crash() {
#if defined(OS_WINDOWS) && defined(_DEBUG)
    __debugbreak();
#endif
    abort();
}