#include <cstdarg>
#include <ntddk.h>
#include <ntstrsafe.h>

EXTERN_C_START

// for nlohmann::json
// wrapper for _vsnprintf
int snprintf(char *s, size_t n, const char *format, ...) {
    int ret;
    va_list arg;
    va_start(arg, format);
    ret = _vsnprintf(s, n, format, arg);
    va_end(arg);
    return ret;
}

unsigned long long int strtoull(const char *ptr, char **end, int base) {
    unsigned long long ret = 0;
    if (base > 36)
        goto out;
    while (*ptr) {
        int digit;
        if (*ptr >= '0' && *ptr <= '9' && *ptr < '0' + base)
            digit = *ptr - '0';
        else if (*ptr >= 'A' && *ptr < 'A' + base - 10)
            digit = *ptr - 'A' + 10;
        else if (*ptr >= 'a' && *ptr < 'a' + base - 10)
            digit = *ptr - 'a' + 10;
        else
            break;
        ret *= base;
        ret += digit;
        ptr++;
    }
    out:
    if (end)
        *end = (char *) ptr;
    return ret;
}

long long int strtoll(const char *ptr, char **end, int base) {
    long long ret = 0;
    bool neg = false;
    if (base > 36)
        goto out;
    if (*ptr == '-') {
        neg = true;
        ptr++;
    }
    while (*ptr) {
        int digit;
        if (*ptr >= '0' && *ptr <= '9' && *ptr < '0' + base)
            digit = *ptr - '0';
        else if (*ptr >= 'A' && *ptr < 'A' + base - 10)
            digit = *ptr - 'A' + 10;
        else if (*ptr >= 'a' && *ptr < 'a' + base - 10)
            digit = *ptr - 'a' + 10;
        else
            break;
        ret *= base;
        ret += digit;
        ptr++;
    }
    out:
    if (end)
        *end = (char *) ptr;
    return neg ? -ret : ret;
}

// we don't use floating point in kernel
// so these functions are dummy
// just for linking

// Due to a problem with the msvc toolchain, the linker cannot find some of the
// following functions in kernel mode, but the compiler won't let us redefine
// them. We used global variables to trick the linker that we seem to have
// implemented these functions.

char _dtest, _dsign, _dclass, _fltin;

EXTERN_C_END
