#include <linux/slab.h>

#include <dpusm/provider_api.h>

typedef enum alloc_type {
    ALLOC_REAL,
    ALLOC_REF,
    ALLOC_INVALID,
} alloc_type_t;

typedef struct alloc {
    alloc_type_t type;
    void *ptr;
    size_t size;
} alloc_t;

static void *
ptr_offset(void *ptr, size_t offset) {
    if (!ptr) {
        return NULL;
    }

    return ((char *) ptr) + offset;
}

static int
dpusm_provider_algorithms(int *compress, int *decompress,
                          int *checksum, int *checksum_byteorder,
                          int *raid) {
    *compress           = 0;
    *decompress         = 0;
    *checksum           = 0;
    *checksum_byteorder = 0;
    *raid               = 0;
    return DPUSM_OK;
}

static void *
dpusm_provider_alloc(size_t size, size_t actual) {
    alloc_t *alloc = kmalloc(sizeof(alloc_t), GFP_KERNEL);
    if (alloc) {
        alloc->type = ALLOC_REAL;
        alloc->ptr = kmalloc(actual, GFP_KERNEL);
        alloc->size = size;

        if (!alloc->ptr) {
            kfree(alloc);
            alloc = NULL;
        }
    }

    return alloc;
}

static void *
dpusm_provider_alloc_ref(void *src, size_t offset, size_t size) {
    if (!src) {
        return NULL;
    }

    alloc_t *src_handle = (alloc_t *) src;
    alloc_t *ref = kmalloc(sizeof(alloc_t), GFP_KERNEL);
    if (ref) {
        ref->type = ALLOC_REF;
        ref->ptr = ptr_offset(src_handle->ptr, offset);
        ref->size = size;
    }

    return ref;
}

static int
dpusm_provider_get_size(void *handle, size_t *size, size_t *actual) {
    return DPUSM_ERROR;
}

static void
dpusm_provider_free(void *handle) {
    alloc_t *alloc = (alloc_t *) handle;
    if (alloc) {
        switch (alloc->type) {
            case ALLOC_REAL:
                kfree(alloc->ptr);
                break;
            case ALLOC_REF:
            case ALLOC_INVALID:
            default:
                break;
        }

        kfree(alloc);
    }
}

static int
dpusm_provider_copy_from_mem(dpusm_mv_t *mv, const void *buf, size_t size) {
    alloc_t *dst_handle = (alloc_t *) mv->handle;
    if (!dst_handle) {
        return DPUSM_ERROR;
    }

    void *dst = ptr_offset(dst_handle->ptr, mv->offset);
    memcpy(dst, buf, size);

    return DPUSM_OK;
}

static int
dpusm_provider_copy_to_mem(dpusm_mv_t *mv, void *buf, size_t size) {
    alloc_t *src_handle = (alloc_t *) mv->handle;
    if (!src_handle) {
        return DPUSM_ERROR;
    }

    void *src = ptr_offset(src_handle->ptr, mv->offset);
    memcpy(buf, src, size);

    return DPUSM_OK;
}

const dpusm_pf_t example_dpusm_provider_functions = {
    .algorithms         = dpusm_provider_algorithms,
    .alloc              = dpusm_provider_alloc,
    .alloc_ref          = dpusm_provider_alloc_ref,
    .get_size           = dpusm_provider_get_size,
    .free               = dpusm_provider_free,
    .copy_from_mem      = dpusm_provider_copy_from_mem,
    .copy_to_mem        = dpusm_provider_copy_to_mem,
    .mem_stats          = NULL,
    .zero_fill          = NULL,
    .all_zeros          = NULL,
    .compress           = NULL,
    .decompress         = NULL,
    .checksum           = NULL,
    .raid               = {
                              .alloc       = NULL,
                              .free        = NULL,
                              .gen         = NULL,
                              .new_parity  = NULL,
                              .cmp         = NULL,
                              .rec         = NULL,
                          },
    .file               = {
                              .open        = NULL,
                              .write       = NULL,
                              .close       = NULL,

                          },
    .disk               = {
                              .open        = NULL,
                              .invalidate  = NULL,
                              .write       = NULL,
                              .flush       = NULL,
                              .close       = NULL,
                          },
};
