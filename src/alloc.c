#include <linux/slab.h>

#include <dpusm/alloc.h>

#if DPUSM_TRACK_ALLOCS
/*
 * keep track of allocations owned by the dpusm
 * these variables are defined in dpusm.c
 */
static atomic_t alloc_count;    /* total allocations (never decreases) */
static atomic_t active_count;   /* currently active allocations */
static atomic_t active_size;    /* currently active allocations */
#endif

void dpusm_mem_init(void) {
#if DPUSM_TRACK_ALLOCS
    atomic_set(&alloc_count,  0);
    atomic_set(&active_count, 0);
    atomic_set(&active_size,  0);
#endif
}

void *dpusm_mem_alloc(size_t size) {
    void *ptr = kmalloc(size, GFP_KERNEL);
    if (ptr) {
#if DPUSM_TRACK_ALLOCS
        atomic_add(1,    &alloc_count);
        atomic_add(1,    &active_count);
        atomic_add(size, &active_size);
#endif
    }
    return ptr;
}

void dpusm_mem_free(void *ptr, size_t size) {
    kfree(ptr);
#if DPUSM_TRACK_ALLOCS
    atomic_sub(1,    &active_count);
    atomic_sub(size, &active_size);
#endif
}

void dpusm_mem_stats(size_t *total, size_t *count, size_t *size) {
#if DPUSM_TRACK_ALLOCS
    if (total) {
        *total = atomic_read(&alloc_count);
    }

    if (count) {
        *count = atomic_read(&active_count);
    }

    if (size) {
        *size = atomic_read(&active_size);
    }
#endif
}
