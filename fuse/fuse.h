#ifndef FUSE_TAG_H
#define FUSE_TAG_H


#ifndef FUSE_VER
#error "must define FUSE_VER"
#endif

#ifndef _TAG_FUSE
#error "must define _TAG_FUSE"
#endif

#define FUSE_USE_VERSION FUSE_VER

#define INCLUDE_FILE(M) <M>
#include INCLUDE_FILE(_TAG_FUSE/fuse.h)

#endif //FUSE_TAG_H
