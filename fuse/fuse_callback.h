#ifndef FUSE_CALLBACK_H
#define FUSE_CALLBACK_H

#define FUSE_USE_VERSION 26
#include <fuse.h>
extern struct fuse_operations tag_oper;
int tag_getattr(const char *user_path, struct stat *stbuf);


#endif //FUSE_CALLBACK_H
