#include "log.h"
#include <stdio.h>

#define LOGFILE "tagfs.log"
FILE *mylog;

__attribute__((constructor))
static void log_init(void)
{
    /* append logs to previous executions */
    mylog = fopen(LOGFILE, "a");
}
