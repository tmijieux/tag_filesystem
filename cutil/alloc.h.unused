#ifndef ALLOC_H
#define ALLOC_H

#ifndef UUC_WHAT_MALLOC
#define UUC_WHAT_MALLOC "malloc"
//#define UUC_WHAT_MALLOC "GC_malloc"
#endif


void *gcmalloc(size_t size);
void gcfree(void *ptr);
void *gccalloc(size_t nmemb, size_t size);
void *gcrealloc(void *ptr, size_t size);

#endif	//ALLOC_H
