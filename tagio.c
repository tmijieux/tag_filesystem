#include "tagio.h"


int tag_ioctl_unique_id(void)
{
    static int i = 0;
    return i++;
}
