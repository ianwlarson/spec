#include "spec.h"

/*
 * xfer slots are from 0xc000_0000 to 0xcfff_ffff
 *
 */

volatile int *valid_ptr;

int
victim_function(void const*const arr)
{
    volatile static int i = 1;
    int const*const i_arr = arr;
    if (likely(i_arr == valid_ptr)) {
        i *= *i_arr;
    }

    return i;
}
