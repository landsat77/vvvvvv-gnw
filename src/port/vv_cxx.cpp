/*
 * C++ runtime of the homebrew (replaces sdk/src/gw_core_cxx_support.cpp,
 * filtered out of the link by the Makefile): operator new / delete use the
 * game heap (vv_alloc.c), like SDL_malloc. VVVVVV allocates all the time
 * (std::string, std::vector), and frees.
 */
#include <cstddef>
#include <cstdlib>
#include <new>
#include "vv_platform.h"

static void *alloc(size_t s)
{
    void *p = vv_malloc(s ? s : 1);
    if (!p)
        vv_plat_fatal("out of memory (operator new)");
    return p;
}

void *operator new(size_t s) { return alloc(s); }
void *operator new[](size_t s) { return alloc(s); }
void *operator new(size_t s, const std::nothrow_t &) noexcept { return vv_malloc(s ? s : 1); }
void *operator new[](size_t s, const std::nothrow_t &) noexcept { return vv_malloc(s ? s : 1); }
void operator delete(void *p) noexcept { vv_free(p); }
void operator delete[](void *p) noexcept { vv_free(p); }
void operator delete(void *p, size_t) noexcept { vv_free(p); }
void operator delete[](void *p, size_t) noexcept { vv_free(p); }

#ifdef VV_DEVICE
extern "C" void abort(void);

extern "C" void __cxa_pure_virtual()
{
    vv_plat_fatal("pure virtual call");
}

extern "C" {
void *__dso_handle = (void *)&__dso_handle;
}

extern "C" int __cxa_atexit(void (*)(void *), void *, void *)
{
    return 0;
}

extern "C" void *__cxa_allocate_exception(size_t) { abort(); return nullptr; }
extern "C" void  __cxa_throw(void *, void *, void (*)(void *)) { abort(); }
extern "C" int __gxx_personality_v0(int, int, unsigned long long, void *, void *)
{
    abort();
    return 0;
}

namespace __gnu_cxx {
void __verbose_terminate_handler() { abort(); }
}

#endif
