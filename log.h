#ifndef LOG_H
#define LOG_H

#include <stdio.h>
extern FILE *mylog;
#define LOG(args...) do { fprintf(mylog, args); fflush(mylog); } while (0)


#endif //LOG_H
