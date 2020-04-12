#include <stdio.h>

#include "r_time.h"

int
main(void)
{
    r_time_init();

    time_t res = r_time_now();
    printf("final: %ld\n", res);
    return 0;
}
