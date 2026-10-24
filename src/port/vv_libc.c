/*
 * The few C library functions VVVVVV (and libstdc++) use that the
 * firmware ABI does not export.
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

/* static objects are never destroyed (the homebrew ends by switching
 * back to the launcher) */
int atexit(void (*fn)(void))
{
    (void)fn;
    return 0;
}

int putchar(int c)
{
    printf("%c", c);
    return c;
}

size_t strnlen(const char *s, size_t max)
{
    size_t n = 0;
    while (n < max && s[n])
        n++;
    return n;
}

float fmodf(float x, float y)
{
    if (y == 0.0f || isinf(x) || isnan(x) || isnan(y))
        return NAN;
    if (isinf(y))
        return x;
    float q = truncf(x / y);
    float r = x - q * y;
    /* x / y rounded: bring r back into (-|y|, |y|) with the sign of x */
    if (fabsf(r) >= fabsf(y))
        r = fmodf(r, y);
    if (r != 0.0f && ((r < 0.0f) != (x < 0.0f)))
        r += (x < 0.0f ? -fabsf(y) : fabsf(y));
    return r;
}
