#include <linux/slab.h>

#include <dpusm/provider.h>
#include <dpusm/user_api.h>

#define FUNCS(dpusmph) (((dpusm_ph_t *) dpusmph)->funcs)

/* defined in dpusm.c */
extern void *dpusm_get(const char *name);
extern int dpusm_put(void *handle);

/* struct returned by alloc functions */
typedef struct dpusm_handle {
    void *provider; /* where this allocation came from */
    void *handle;   /* opaque handle to the allocation */
} dpusm_handle_t;

static dpusm_handle_t *
dpusm_handle_construct(void *provider, void *handle) {
    dpusm_handle_t *dpusmh = NULL;
    if (provider && handle) {
        dpusmh = kmalloc(sizeof(dpusm_handle_t), GFP_KERNEL);
        dpusmh->provider = provider;
        dpusmh->handle = handle;
    }
    return dpusmh;
}

static void
dpusm_handle_free(dpusm_handle_t *dpusmh) {
    kfree(dpusmh);
}

/*
 * check provider sanity at run time
 *
 * The individual required functions do not need to be checked, as
 * they were checked at load time. This is just to make sure the
 * offloader has not gone down. This relies on the provider updating
 * the offloader's state.
 */
static int
dpusm_provider_sane(dpusm_ph_t *provider) {
    return (provider && FUNCS(provider))?DPUSM_OK:DPUSM_ERROR;
}

#define CHECK_PROVIDER(provider, ret)                           \
    do {                                                        \
        if (dpusm_provider_sane(provider) != DPUSM_OK) {        \
            return (ret);                                       \
        }                                                       \
    } while (0)

#define CHECK_HANDLE(handle, new_name, ret)                     \
    dpusm_handle_t *(new_name) = NULL;                          \
    do {                                                        \
        if (!(handle)) {                                        \
            return (ret);                                       \
        }                                                       \
                                                                \
        (new_name) = (dpusm_handle_t *) (handle);               \
        CHECK_PROVIDER((new_name)->provider, (ret));            \
    } while (0)

#define SAME_PROVIDERS(lhs, new_lhs, rhs, new_rhs, ret)         \
    dpusm_handle_t *(new_lhs) = NULL;                           \
    dpusm_handle_t *(new_rhs) = NULL;                           \
    do {                                                        \
        if (!(lhs) || !(rhs)) {                                 \
            return (ret);                                       \
        }                                                       \
                                                                \
        (new_lhs) = (dpusm_handle_t *) (lhs);                   \
        (new_rhs) = (dpusm_handle_t *) (rhs);                   \
                                                                \
        if ((new_lhs)->provider != (new_rhs)->provider) {       \
            return DPUSM_PROVIDER_MISMATCH;                     \
        }                                                       \
                                                                \
        CHECK_PROVIDER((new_lhs)->provider, ret);               \
    } while (0)

static void *
dpusm_get_provider(const char *name) {
    dpusm_ph_t *provider = (dpusm_ph_t *) dpusm_get(name);
    if (!provider) {
        printk("Provider with name \"%s\" not found.\n", name);
        return NULL;
    }

    printk("Provider with name \"%s\" found. %p\n", name, provider);
    return provider;
}

static const char *
dpusm_get_provider_name(void *provider) {
    dpusm_ph_t *dpusmph = (dpusm_ph_t *) provider;
    return dpusmph?dpusmph->name:NULL;
}

static int
dpusm_put_provider(void *provider) {
    return dpusm_put(provider);
}

static void *
dpusm_extract_provider(void *handle) {
    dpusm_handle_t *dpusmh = (dpusm_handle_t *) handle;
    return dpusmh?dpusmh->provider:NULL;
}

static int
dpusm_get_capabilities(void *provider, dpusm_pc_t *caps) {
    CHECK_PROVIDER(provider, DPUSM_ERROR);
    if (!caps) {
        return DPUSM_ERROR;
    }
    return FUNCS(provider)->capabilities(caps);
}

static void *
dpusm_alloc(void *provider, size_t size) {
    CHECK_PROVIDER(provider, NULL);
    return dpusm_handle_construct(provider, FUNCS(provider)->alloc(size));
}

static void *
dpusm_alloc_ref(void *src, size_t offset, size_t size) {
    if (!src) {
        return NULL;
    }

    CHECK_HANDLE(src, dpusmh, NULL);
    return dpusm_handle_construct(dpusmh->provider,
        FUNCS(dpusmh->provider)->alloc_ref(dpusmh->handle,
        offset, size));
}

static void
dpusm_free(void *handle) {
    if (!handle) {
        return;
    }

    dpusm_handle_t *dpusmh = (dpusm_handle_t *) handle;
    if (dpusm_provider_sane(dpusmh->provider) == DPUSM_OK) {
        FUNCS(dpusmh->provider)->free(dpusmh->handle);
    }
    dpusm_handle_free(dpusmh);
}

static int
dpusm_copy_from_mem(dpusm_mv_t *mv, const void *buf, size_t size) {
    if (!mv) {
        return DPUSM_ERROR;
    }

    CHECK_HANDLE(mv->handle, dpusmh, DPUSM_ERROR);

    dpusm_mv_t actual_mv = { .handle = dpusmh->handle,
        .offset = mv->offset };

    return FUNCS(dpusmh->provider)->copy_from_mem(&actual_mv,
        buf, size);
}

static int
dpusm_copy_to_mem(dpusm_mv_t *mv, void *buf, size_t size) {
    if (!mv) {
        return DPUSM_ERROR;
    }

    CHECK_HANDLE(mv->handle, dpusmh, DPUSM_ERROR);

    dpusm_mv_t actual_mv = { .handle = dpusmh->handle,
        .offset = mv->offset };

    return FUNCS(dpusmh->provider)->copy_to_mem(&actual_mv,
        buf, size);
}

static int
dpusm_active(void *provider, size_t *count, size_t *size,
             size_t *actual_count, size_t *actual_size) {
    CHECK_PROVIDER(provider, DPUSM_ERROR);
    return FUNCS(provider)->active(count, size,
        actual_count, actual_size);
}

static int
dpusm_zero_fill(void *handle, size_t offset, size_t size) {
    CHECK_HANDLE(handle, dpusmh, DPUSM_ERROR);
    if (!FUNCS(dpusmh->provider)->zero_fill) {
        return DPUSM_NOT_IMPLEMENTED;
    }
    return FUNCS(dpusmh->provider)->zero_fill(dpusmh->handle, offset, size);
}

static int
dpusm_all_zeros(void *handle, size_t offset, size_t size) {
    CHECK_HANDLE(handle, dpusmh, DPUSM_ERROR);
    if (!FUNCS(dpusmh->provider)->all_zeros) {
        return DPUSM_NOT_IMPLEMENTED;
    }
    return FUNCS(dpusmh->provider)->all_zeros(dpusmh->handle, offset, size);
}

static void *
dpusm_create_gang(void **handles, size_t count) {
    /*
     * there must be at least one handle to have
     * a target provider to create the gang on
     */
    if (!count) {
        return NULL;
    }

    void **provider_handles = kzalloc(sizeof(void *) * count, GFP_KERNEL);
    if (!provider_handles) {
        return NULL;
    }

    void *provider = NULL;
    size_t i;
    for(i = 0; i < count; i++) {
        dpusm_handle_t *dpusmh = (dpusm_handle_t *) handles[i];

        /* all handles must exist */
        if (!dpusmh) {
            break;
        }

        void *dpusmh_provider = dpusmh->provider;
        if (!provider) {
            provider = dpusmh->provider;
            if (dpusm_provider_sane(provider) != DPUSM_OK) {
                break;
            }
        }
        /* all handles must be on the same provider */
        else if (provider != dpusmh_provider) {
            break;
        }

        /* extract provider handle */
        provider_handles[i] = dpusmh->handle;
    }


    void *gang = NULL;
    if (i == count) {
        if (FUNCS(provider)->create_gang) {
            gang = FUNCS(provider)->create_gang(provider_handles, count);
        }
    }

    kfree(provider_handles);
    return dpusm_handle_construct(provider, gang);
}

static int
dpusm_realign(void *src, void **dst, size_t aligned_size, size_t alignment) {
    CHECK_HANDLE(src, src_dpusmh, DPUSM_ERROR);

    /* realign is optional */
    if (!FUNCS(src_dpusmh->provider)->realign) {
        return DPUSM_NOT_IMPLEMENTED;
    }

    if (!dst) {
        return DPUSM_ERROR;
    }

    void *dst_provider_handle = NULL;
    const int rc = FUNCS(src_dpusmh->provider)->realign(src_dpusmh->handle,
        &dst_provider_handle, aligned_size, alignment);
    if (rc != DPUSM_OK) {
        return rc;
    }

    /* nothing done */
    if (src_dpusmh->handle == dst_provider_handle) {
        *dst = src;
    }
    /* data was copied into a new handle */
    else {
        *dst = dpusm_handle_construct(src_dpusmh->provider, dst_provider_handle);
    }
    return rc;
}

static int
dpusm_bulk_from_mem(void *handle, void **bufs, size_t *sizes, size_t count) {
    CHECK_HANDLE(handle, dpusmh, DPUSM_ERROR);

    /* bulk from mem is optional */
    if (!FUNCS(dpusmh->provider)->bulk_from_mem) {
        return DPUSM_NOT_IMPLEMENTED;
    }

    return FUNCS(dpusmh->provider)->bulk_from_mem(dpusmh->handle,
        bufs, sizes, count);
}

static int
dpusm_bulk_to_mem(void *handle, void **bufs, size_t *sizes, size_t count) {
    CHECK_HANDLE(handle, dpusmh, DPUSM_ERROR);

    /* bulk to mem is optional */
    if (!FUNCS(dpusmh->provider)->bulk_to_mem) {
        return DPUSM_NOT_IMPLEMENTED;
    }

    return FUNCS(dpusmh->provider)->bulk_to_mem(dpusmh->handle,
        bufs, sizes, count);
}

static int
dpusm_compress(dpusm_compress_t alg, void *src, void *dst, size_t s_len,
    int level, size_t *c_len) {
    SAME_PROVIDERS(dst, dst_dpusmh, src, src_dpusmh, DPUSM_ERROR);

    /* compression is optional */
    if (!FUNCS(src_dpusmh->provider)->compress) {
        return DPUSM_NOT_IMPLEMENTED;
    }

    return FUNCS(src_dpusmh->provider)->compress(alg, src_dpusmh->handle,
        dst_dpusmh->handle, s_len, level, c_len);
}

static int
dpusm_decompress(dpusm_compress_t alg,
    void *src, void *dst, int level) {
    SAME_PROVIDERS(dst, dst_dpusmh, src, src_dpusmh, DPUSM_ERROR);

    /* decompression is optional */
    if (!FUNCS(src_dpusmh->provider)->decompress) {
        return DPUSM_NOT_IMPLEMENTED;
    }

    return FUNCS(src_dpusmh->provider)->decompress(alg,
        src_dpusmh->handle, dst_dpusmh->handle, level);
}

static int
dpusm_checksum(dpusm_checksum_t alg, dpusm_byteorder_t order,
    void *data, size_t size, void *cksum) {
    SAME_PROVIDERS(data, data_dpusmh, cksum, cksum_dpusmh, DPUSM_ERROR);

    /* checksum is optional */
    if (!FUNCS(data_dpusmh->provider)->checksum) {
        return DPUSM_NOT_IMPLEMENTED;
    }

    return FUNCS(data_dpusmh->provider)->checksum(alg,
        order, data_dpusmh->handle, size, cksum_dpusmh->handle);
}

static void
dpusm_raid_free(void *raid) {
    if (!raid) {
        return;
    }

    dpusm_handle_t *raid_dpusmh = (dpusm_handle_t *) raid;
    if (dpusm_provider_sane(raid_dpusmh->provider) != DPUSM_OK) {
        return;
    }

    /* raid is optional */
    if (!FUNCS(raid_dpusmh->provider)->raid.free) {
        return;
    }

    FUNCS(raid_dpusmh->provider)->raid.free(raid_dpusmh->handle);
    dpusm_handle_free(raid_dpusmh);
}

static void *
dpusm_raid_alloc(uint64_t raidn, uint64_t cols, void *src_handle,
    void **col_dpusm_handles) {
    CHECK_HANDLE(src_handle, src_dpusmh, NULL);
    void *provider = src_dpusmh->provider;

    /* raid is optional */
    if (!FUNCS(provider)->raid.alloc) {
        return NULL;
    }

    int good = 1;

    /* extract provider handles out of the dpusm handles */
    void **col_provider_handles = kmalloc(sizeof(void *) * cols, GFP_KERNEL);
    for(uint64_t c = 0; c < cols; c++) {
        dpusm_handle_t *col_dpusm_handle = (dpusm_handle_t *) col_dpusm_handles[c];
        if (!col_dpusm_handle) {
            good = 0;
            break;
        }

        /* make sure the columns are on the same provider as src */
        if (provider != col_dpusm_handle->provider) {
            good = 0;
            break;
        }

        col_provider_handles[c] = col_dpusm_handle->handle;
    }

    void *rr_handle = NULL;
    if (good == 1) {
        /* src_handle is not passed in since it has already been checked */
        rr_handle = FUNCS(provider)->raid.alloc(raidn, cols,
            col_provider_handles);

        good = !!rr_handle;
    }

    dpusm_handle_t *dpusmh = NULL;
    if (good == 1) {
        dpusmh = dpusm_handle_construct(provider, rr_handle);
    }
    else {
        if (rr_handle) {
            dpusm_raid_free(rr_handle);
        }
    }

    kfree(col_provider_handles);
    return dpusmh;
}

static int
dpusm_raid_gen(void *raid) {
    CHECK_HANDLE(raid, dpusmh, DPUSM_ERROR);

    /* raid is optional */
    if (!FUNCS(dpusmh->provider)->raid.gen) {
        return DPUSM_NOT_IMPLEMENTED;
    }

    return FUNCS(dpusmh->provider)->raid.gen(dpusmh->handle);
}

static int
dpusm_raid_new_parity(void *raid, uint64_t raidn,
    void ***new_dpusm_parity_cols, size_t *new_parity_sizes) {
    CHECK_HANDLE(raid, dpusmh, DPUSM_ERROR);
    void *provider = dpusmh->provider;

    /* raid is optional */
    if (!FUNCS(provider)->raid.new_parity) {
        return DPUSM_NOT_IMPLEMENTED;
    }

    /* array of pointers for provider to fill in */
    void **new_provider_parity_cols = kzalloc(sizeof(void *) * raidn, GFP_KERNEL);
    const int rc = FUNCS(provider)->raid.new_parity(dpusmh->handle, raidn,
        new_provider_parity_cols, new_parity_sizes);

    if (rc == DPUSM_OK) {
        /* wrap provider handles in DPUSM handles and send to user */
        for(uint64_t c = 0; c < raidn; c++) {
            if (new_dpusm_parity_cols[c]) {
                *(new_dpusm_parity_cols[c]) =
                    dpusm_handle_construct(provider, new_provider_parity_cols[c]);
            }
        }
    }
    else {
        for(uint64_t c = 0; c < raidn; c++) {
            FUNCS(provider)->free(new_provider_parity_cols[c]);
            if (new_dpusm_parity_cols[c]) {
                *(new_dpusm_parity_cols[c]) = NULL;
            }
        }
    }

    kfree(new_provider_parity_cols);
    return rc;

}

static int
dpusm_raid_cmp(void *lhs_handle, void *rhs_handle, int *diff) {
    if (!diff) {
        return DPUSM_ERROR;
    }

    SAME_PROVIDERS(lhs_handle, lhs_dpusmh, rhs_handle, rhs_dpusmh, DPUSM_ERROR);

    if (lhs_dpusmh == rhs_dpusmh) {
        *diff = 0;
        return DPUSM_OK;
    }

    return FUNCS(lhs_dpusmh->provider)->raid.cmp(lhs_dpusmh->handle, rhs_dpusmh->handle, diff);
}

static int
dpusm_raid_rec(void *raid, int *tgts, int ntgts) {
    CHECK_HANDLE(raid, dpusmh, DPUSM_ERROR);

    /* raid is optional */
    if (!FUNCS(dpusmh->provider)->raid.rec) {
        return DPUSM_NOT_IMPLEMENTED;
    }

    return FUNCS(dpusmh->provider)->raid.rec(dpusmh->handle, tgts, ntgts);
}

static void *
dpusm_file_open(void *provider, const char *path, int flags, int mode) {
    CHECK_PROVIDER(provider, NULL);

    /* file operations are optional */
    if (!FUNCS(provider)->file.open) {
        return NULL;
    }

    return dpusm_handle_construct(provider,
        FUNCS(provider)->file.open(path, flags, mode));
}

static int
dpusm_file_write(void *fp_handle, void *data, size_t count,
    loff_t offset, ssize_t *resid, int *err) {
    SAME_PROVIDERS(fp_handle, fp_dpusmh, data, dpusmh, DPUSM_ERROR);

    /* file operations are optional */
    if (!FUNCS(fp_dpusmh->provider)->file.write) {
        return DPUSM_NOT_IMPLEMENTED;
    }

    return FUNCS(fp_dpusmh->provider)->file.write(fp_dpusmh->handle,
        dpusmh->handle, count, offset, resid, err);
}

static int
dpusm_file_close(void *fp_handle) {
    CHECK_HANDLE(fp_handle, fp_dpusmh, DPUSM_ERROR);

    /* file operations are optional */
    if (!FUNCS(fp_dpusmh->provider)->file.close) {
        return DPUSM_NOT_IMPLEMENTED;
    }

    FUNCS(fp_dpusmh->provider)->file.close(fp_dpusmh->handle);
    dpusm_handle_free(fp_dpusmh);
    return DPUSM_OK;
}

#ifdef _KERNEL
static void *
dpusm_disk_open(void *provider, const char *path, fmode_t mode, void *holder) {
    CHECK_PROVIDER(provider, NULL);

    /* disk operations are optional */
    if (!FUNCS(provider)->disk.open) {
        return NULL;
    }

    return dpusm_handle_construct(provider,
        FUNCS(provider)->disk.open(path, mode, holder));
}

static int
dpusm_disk_invalidate(void *bdev_handle) {
    CHECK_HANDLE(bdev_handle, bdev_dpusmh, DPUSM_ERROR);

    /* disk operations are optional */
    if (!FUNCS(bdev_dpusmh->provider)->disk.invalidate) {
        return DPUSM_NOT_IMPLEMENTED;
    }

    return FUNCS(bdev_dpusmh->provider)->disk.invalidate(bdev_dpusmh->handle);
}

static int
dpusm_disk_write(void *bdev_handle, void *data,
    size_t io_size, uint64_t io_offset, int rw,
    int failfast, int flags, void *ptr,
    dpusm_disk_write_completion_t write_completion) {
    if (!write_completion) {
        return DPUSM_ERROR;
    }

    SAME_PROVIDERS(bdev_handle, bdev_dpusmh, data, dpusmh, DPUSM_ERROR);

    /* disk operations are optional */
    if (!FUNCS(bdev_dpusmh->provider)->disk.write) {
        return DPUSM_NOT_IMPLEMENTED;
    }

    return FUNCS(bdev_dpusmh->provider)->disk.write(bdev_dpusmh->handle,
        dpusmh->handle, io_size, io_offset, rw, failfast, flags,
        ptr, write_completion);
}

static int
dpusm_disk_flush(void *bdev_handle, void *ptr,
    dpusm_disk_flush_completion_t flush_completion) {
    if (!flush_completion) {
        return DPUSM_ERROR;
    }

    CHECK_HANDLE(bdev_handle, bdev_dpusmh, DPUSM_ERROR);

    /* disk operations are optional */
    if (!FUNCS(bdev_dpusmh->provider)->disk.flush) {
        return DPUSM_NOT_IMPLEMENTED;
    }

    return FUNCS(bdev_dpusmh->provider)->disk.flush(bdev_dpusmh->handle,
        ptr, flush_completion);
}

static int
dpusm_disk_close(void *bdev_handle, fmode_t mode) {
    CHECK_HANDLE(bdev_handle, bdev_dpusmh, DPUSM_ERROR);

    /* disk operations are optional */
    if (!FUNCS(bdev_dpusmh->provider)->disk.close) {
        return DPUSM_NOT_IMPLEMENTED;
    }

    FUNCS(bdev_dpusmh->provider)->disk.close(bdev_dpusmh->handle, mode);
    dpusm_handle_free(bdev_dpusmh);
    return DPUSM_OK;
}
#endif

static const dpusm_uf_t user_functions = {
    .get                = dpusm_get_provider,
    .get_name           = dpusm_get_provider_name,
    .put                = dpusm_put_provider,
    .extract            = dpusm_extract_provider,
    .capabilities       = dpusm_get_capabilities,
    .alloc              = dpusm_alloc,
    .alloc_ref          = dpusm_alloc_ref,
    .free               = dpusm_free,
    .copy_from_mem      = dpusm_copy_from_mem,
    .copy_to_mem        = dpusm_copy_to_mem,
    .active             = dpusm_active,
    .zero_fill          = dpusm_zero_fill,
    .all_zeros          = dpusm_all_zeros,
    .create_gang        = dpusm_create_gang,
    .realign            = dpusm_realign,
    .bulk_from_mem      = dpusm_bulk_from_mem,
    .bulk_to_mem        = dpusm_bulk_to_mem,
    .compress           = dpusm_compress,
    .decompress         = dpusm_decompress,
    .checksum           = dpusm_checksum,
    .raid               = {
                              .alloc              = dpusm_raid_alloc,
                              .free               = dpusm_raid_free,
                              .gen                = dpusm_raid_gen,
                              .new_parity         = dpusm_raid_new_parity,
                              .cmp                = dpusm_raid_cmp,
                              .rec                = dpusm_raid_rec,
                          },
    .file               = {
                              .open               = dpusm_file_open,
                              .write              = dpusm_file_write,
                              .close              = dpusm_file_close,
                          },
#ifdef _KERNEL
    .disk               = {
                              .open               = dpusm_disk_open,
                              .invalidate         = dpusm_disk_invalidate,
                              .write              = dpusm_disk_write,
                              .flush              = dpusm_disk_flush,
                              .close              = dpusm_disk_close,
                          },
#endif
};

const dpusm_uf_t *
dpusm_initialize(void) {
    return &user_functions;
}

int
dpusm_finalize(void) {
    return DPUSM_OK;
}

EXPORT_SYMBOL(dpusm_initialize);
EXPORT_SYMBOL(dpusm_finalize);
