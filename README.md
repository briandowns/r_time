# r_time

r_time is a library to retrieve time from remote servers. This is a port of [tidwall's](http://github.com/tidwall) Go [package](https://github.com/tidwall/rtime).

The way the library works is that it iterates an array of public servers and queries them, extracts the "Date|date" header field, converts the string value to an `time_t` value and returns it to the caller.

## Build

```sh
make 
```

## Example

```c
#include <stdio.h>

#include "r_time.h"

int
main(void)
{
    r_time_init();

    time_t res = r_time_now();
    if (res == 0) {
        fprintf(stderr, "unable to get current remote time");
        return 1;
    }
    printf("current remote time: %ld\n", res);
    
    return 0;
}
```

## Contact

[@bdowns328](http://twitter.com/bdowns328)
