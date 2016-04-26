#include <stdio.h>

#include "./util.h"
#include "./log.h"
#include "initializer.h"

#define LOGFILE "tagfs.log"

FILE *mylog;

INITIALIZER(log_init)
{
    /* append logs to previous executions */
    mylog = fopen(LOGFILE, "a");
}
