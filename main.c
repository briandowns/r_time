#include "r_time.h"

int
main(void)
{
    r_time_init();
    
    char buff[20];
    time_t* current_time = r_time_now();
    strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(current_time));
    return 0;
}
