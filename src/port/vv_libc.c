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
    /* (not printf("%c"): GCC turns that into putchar) */
    char s[2] = { (char)c, 0 };
    printf("%s", s);
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

/* sin / cos / ldexp (stb_vorbis builds its tables with them): not in the
 * firmware ABI. Double precision: quadrant reduction, then Taylor series
 * on [-pi/4, pi/4] (error < 1e-16). */
static double sin_poly(double x)
{
    double x2 = x * x;
    return x * (1 - x2 / 6 * (1 - x2 / 20 * (1 - x2 / 42 * (1 - x2 / 72 * (1 - x2 / 110 * (1 - x2 / 156 * (1 - x2 / 210)))))));
}

static double cos_poly(double x)
{
    double x2 = x * x;
    return 1 - x2 / 2 * (1 - x2 / 12 * (1 - x2 / 30 * (1 - x2 / 56 * (1 - x2 / 90 * (1 - x2 / 132 * (1 - x2 / 182 * (1 - x2 / 240)))))));
}

static double trig(double x, int want_cos)
{
    const double half_pi = 1.57079632679489661923;
    if (isnan(x) || isinf(x))
        return NAN;
    double q = floor(x / half_pi + 0.5);
    double r = x - q * half_pi;
    long n = (long)q & 3;
    if (want_cos)
        n = (n + 1) & 3;
    switch (n) {
    case 0: return sin_poly(r);
    case 1: return cos_poly(r);
    case 2: return -sin_poly(r);
    default: return -cos_poly(r);
    }
}

double sin(double x) { return trig(x, 0); }
double cos(double x) { return trig(x, 1); }

double ldexp(double x, int e)
{
    while (e > 0) {
        int k = e > 60 ? 60 : e;
        x *= (double)(1ull << k);
        e -= k;
    }
    while (e < 0) {
        int k = -e > 60 ? 60 : -e;
        x /= (double)(1ull << k);
        e += k;
    }
    return x;
}

/* log / exp (stb_vorbis: lookup1_values) */
double log(double x)
{
    if (isnan(x) || x < 0)
        return NAN;
    if (x == 0)
        return -INFINITY;
    if (isinf(x))
        return x;
    /* x = m * 2^e, m in [sqrt(1/2), sqrt(2)) */
    int e = 0;
    while (x >= 1.41421356237309504880) { x *= 0.5; e++; }
    while (x < 0.70710678118654752440) { x *= 2.0; e--; }
    /* log(m) = 2 atanh((m - 1) / (m + 1)) */
    double t = (x - 1) / (x + 1), t2 = t * t, term = t, sum = 0;
    for (int k = 1; k < 40; k += 2) {
        sum += term / k;
        term *= t2;
    }
    return 2 * sum + e * 0.69314718055994530942;
}

double exp(double x)
{
    if (isnan(x))
        return x;
    if (x > 709)
        return INFINITY;
    if (x < -745)
        return 0;
    /* x = k ln 2 + r, |r| <= ln 2 / 2 */
    double k = floor(x / 0.69314718055994530942 + 0.5);
    double r = x - k * 0.69314718055994530942;
    double term = 1, sum = 1;
    for (int i = 1; i < 25; i++) {
        term *= r / i;
        sum += term;
    }
    return ldexp(sum, (int)k);
}
