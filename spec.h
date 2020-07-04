#pragma once

#include <stdint.h>

static inline void
flush(void const*const ptr)
{
  asm volatile (
     "mfence         \n"
     "clflush 0(%0)  \n"
     :
     : "r" (ptr)
     :
  );
}

static inline void
prefetch(void const*const ptr, int const level)
{
    switch (level) {
        case 0:
            asm volatile (
                "prefetcht0 0(%0)\n"
                :
                : "r" (ptr)
                : "memory"
                );
            break;
        case 1:
            asm volatile (
                "prefetcht1 0(%0)\n"
                :
                : "r" (ptr)
                : "memory"
                );
            break;
        case 2:
            asm volatile (
                "prefetcht2 0(%0)\n"
                :
                : "r" (ptr)
                : "memory"
                );
            break;
        default:
            asm volatile (
                "prefetchnta 0(%0)\n"
                :
                : "r" (ptr)
                : "memory"
                );
            break;
    }
}

static inline uint64_t
rdtsc(void)
{
    uint32_t low, high;
    asm volatile("rdtsc":"=a"(low),"=d"(high));
    return ((uint64_t)high << 32) | low;
}

static inline void
mfence(void)
{
    asm volatile ("mfence\n" ::: "memory");
}
static inline void
lfence(void)
{
    asm volatile ("lfence\n" ::: "memory");
}

static inline uint64_t
my_probe(void const*const ptr, volatile unsigned char *const p_out)
{
    unsigned char const*const charp = ptr;
    unsigned char tmp;
    mfence();
    lfence();
    uint64_t const start = rdtsc();
    lfence();
    tmp = *charp;
    lfence();
    uint64_t const end = rdtsc();
    *p_out = tmp;

    return end - start;
}

static inline unsigned long
timed_probe(void const*const ptr)
{
  volatile unsigned long time;

  asm volatile (
    "mfence             \n"
    "lfence             \n"
    "rdtsc              \n"
    "lfence             \n"
    "movl %%eax, %%esi  \n"
    "movl (%1), %%eax   \n"
    "lfence             \n"
    "rdtsc              \n"
    "subl %%esi, %%eax  \n"
    "clflush 0(%1)      \n"
    : "=a" (time)
    : "c" (ptr)
    :  "%esi", "%edx");

  return time;
}

#define likely(a) __builtin_expect(a, 1)
#define unlikely(a) __builtin_expect(a, 0)
