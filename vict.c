
/*
 * xfer slots are from 0xc000_0000 to 0xcfff_ffff
 *
 */

int *valid_ptr;

int
victim_function(void const*const arr)
{
    int const*const i_arr = arr;
    if (i_arr == valid_ptr) {
        return *i_arr;
    }

    return 0;
}
