#include <dpusm/alloc.h>
#include <dpusm/provider.h>
#include <dpusm/user_api.h>

#define FUNCS(dpusmph) (((dpusm_ph_t *) dpusmph)->funcs)

/* defined in dpusm.c */
extern void *dpusm_get(const char *name);
extern int dpusm_put(void *handle);

/* struct returned by alloc functions */
typedef struct dpusm_handle {
    dpusm_ph_t *provider; /* where this allocation came from */
    void *handle;         /* opaque handle to the allocation */
} dpusm_handle_t;

static dpusm_handle_t *
dpusm_handle_construct(void *provider, void *handle) {
    dpusm_handle_t *dpusmh = NULL;
    if (provider && handle) {
        dpusmh = dpusm_mem_alloc(sizeof(dpusm_handle_t));
        if (dpusmh) {
            dpusmh->provider = provider;
            dpusmh->handle = handle;
        }
    }
    return dpusmh;
}

static void
dpusm_handle_free(dpusm_handle_t *dpusmh) {
    dpusm_mem_free(dpusmh, sizeof(*dpusmh));
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
dpusm_alloc(void *provider, size_t size, size_t actual) {
    if (size > actual) {
        return NULL;
    }

    CHECK_PROVIDER(provider, NULL);
    return dpusm_handle_construct(provider,
        FUNCS(provider)->alloc(size, actual));
}

static void *
dpusm_alloc_ref(void *src, size_t offset, size_t size) {
    CHECK_HANDLE(src, dpusmh, NULL);
    return dpusm_handle_construct(dpusmh->provider,
        FUNCS(dpusmh->provider)->alloc_ref(dpusmh->handle,
        offset, size));
}

static int
dpusm_get_size(void *handle, size_t *size, size_t *actual) {
    CHECK_HANDLE(handle, dpusmh, DPUSM_ERROR);
    return FUNCS(dpusmh->provider)->get_size(dpusmh->handle, size, actual);
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

    dpusm_mv_t actual_mv = {
        .handle = dpusmh->handle,
        .offset = mv->offset,
    };

    return FUNCS(dpusmh->provider)->copy_from_mem(&actual_mv,
        buf, size);
}

static int
dpusm_copy_to_mem(dpusm_mv_t *mv, void *buf, size_t size) {
    if (!mv) {
        return DPUSM_ERROR;
    }

    CHECK_HANDLE(mv->handle, dpusmh, DPUSM_ERROR);

    dpusm_mv_t actual_mv = {
        .handle = dpusmh->handle,
        .offset = mv->offset,
    };

    return FUNCS(dpusmh->provider)->copy_to_mem(&actual_mv,
        buf, size);
}

static int
dpusm_provider_mem_stats(void *provider, size_t *total, size_t *count,
    size_t *size, size_t *actual) {
    CHECK_PROVIDER(provider, DPUSM_ERROR);
    if (!FUNCS(provider)->mem_stats) {
        return DPUSM_NOT_IMPLEMENTED;
    }
    return FUNCS(provider)->mem_stats(total, count, size, actual);
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

    /* raid_dpusmh->handle should not be NULL if this has been reached */
    FUNCS(raid_dpusmh->provider)->raid.free(raid_dpusmh->handle);
    dpusm_handle_free(raid_dpusmh);
}

static void *
dpusm_raid_alloc(size_t nparity, size_t ndata, void *src,
    void **col_handles) {
    CHECK_HANDLE(src, src_dpusmh, NULL);
    void *provider = src_dpusmh->provider;

    /* raid is optional */
    if (!FUNCS(provider)->raid.alloc) {
        return NULL;
    }

    const size_t ncols = nparity + ndata;

    dpusm_handle_t *raid_dpusmh = NULL;

    /* extract provider handles out of the dpusm handles */
    const size_t provider_handles_size = sizeof(void *) * ncols;
    void **provider_handles = dpusm_mem_alloc(provider_handles_size);
    for(uint64_t c = 0; c < ncols; c++) {
        dpusm_handle_t *col_handle = (dpusm_handle_t *) col_handles[c];
        if (!col_handle) {
            goto cleanup;
        }

        /* make sure the columns are on the same provider as src */
        if (provider != col_handle->provider) {
            goto cleanup;
        }

        provider_handles[c] = col_handle->handle;
    }

    /* src is not passed in since it has already been checked */
    void *raid_ctx = FUNCS(provider)->raid.alloc(nparity, ndata,
        provider_handles);

    raid_dpusmh = dpusm_handle_construct(provider, raid_ctx);

    if (!raid_dpusmh) {
        if (raid_ctx) {
            FUNCS(provider)->raid.free(raid_ctx);
        }
    }

  cleanup:
    dpusm_mem_free(provider_handles, provider_handles_size);
    return raid_dpusmh;
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
    void ***new_parity_cols, size_t *new_parity_sizes) {
    CHECK_HANDLE(raid, dpusmh, DPUSM_ERROR);
    void *provider = dpusmh->provider;

    /* raid is optional */
    if (!FUNCS(provider)->raid.new_parity) {
        return DPUSM_NOT_IMPLEMENTED;
    }

    /* array of pointers for provider to fill in */
    const size_t new_provider_parity_col_size = sizeof(void *) * raidn;
    void **new_provider_parity_cols = dpusm_mem_alloc(new_provider_parity_col_size);
    const int rc = FUNCS(provider)->raid.new_parity(dpusmh->handle, raidn,
        new_provider_parity_cols, new_parity_sizes);

    if (rc == DPUSM_OK) {
        /* wrap provider handles in DPUSM handles and send to user */
        for(uint64_t c = 0; c < raidn; c++) {
            if (new_parity_cols[c]) {
                *(new_parity_cols[c]) =
                    dpusm_handle_construct(provider, new_provider_parity_cols[c]);
            }
        }
    }
    else {
        for(uint64_t c = 0; c < raidn; c++) {
            FUNCS(provider)->free(new_provider_parity_cols[c]);
            if (new_parity_cols[c]) {
                *(new_parity_cols[c]) = NULL;
            }
        }
    }

    dpusm_mem_free(new_provider_parity_cols, new_provider_parity_col_size);
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
    size_t trailing_zeros, loff_t offset, ssize_t *resid, int *err) {
    SAME_PROVIDERS(fp_handle, fp_dpusmh, data, dpusmh, DPUSM_ERROR);

    /* file operations are optional */
    if (!FUNCS(fp_dpusmh->provider)->file.write) {
        return DPUSM_NOT_IMPLEMENTED;
    }

    return FUNCS(fp_dpusmh->provider)->file.write(fp_dpusmh->handle,
        dpusmh->handle, count, trailing_zeros, offset, resid, err);
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

    dpusm_dd_t data = {
        .path = path,
        .path_len = strlen(path),
        .mode = mode,
        .holder = holder,
    };

    return dpusm_handle_construct(provider,
        FUNCS(provider)->disk.open(&data));
}

static int
dpusm_disk_invalidate(void *disk) {
    CHECK_HANDLE(disk, disk_dpusmh, DPUSM_ERROR);

    /* disk operations are optional */
    if (!FUNCS(disk_dpusmh->provider)->disk.invalidate) {
        return DPUSM_NOT_IMPLEMENTED;
    }

    return FUNCS(disk_dpusmh->provider)->disk.invalidate(disk_dpusmh->handle);
}

static int
dpusm_disk_write(void *disk, void *data,
    size_t io_size, size_t trailing_zeros,
    uint64_t io_offset, int failfast, int flags,
    void *ptr, dpusm_dwc_t write_completion) {
    if (!write_completion) {
        return DPUSM_ERROR;
    }

    SAME_PROVIDERS(disk, disk_dpusmh, data, dpusmh, DPUSM_ERROR);

    /* disk operations are optional */
    if (!FUNCS(disk_dpusmh->provider)->disk.write) {
        return DPUSM_NOT_IMPLEMENTED;
    }

    return FUNCS(disk_dpusmh->provider)->disk.write(disk_dpusmh->handle,
        dpusmh->handle, io_size, trailing_zeros, io_offset, failfast, flags,
        ptr, write_completion);
}

static int
dpusm_disk_flush(void *disk, void *ptr,
    dpusm_dfc_t flush_completion) {
    if (!flush_completion) {
        return DPUSM_ERROR;
    }

    CHECK_HANDLE(disk, disk_dpusmh, DPUSM_ERROR);

    /* disk operations are optional */
    if (!FUNCS(disk_dpusmh->provider)->disk.flush) {
        return DPUSM_NOT_IMPLEMENTED;
    }

    return FUNCS(disk_dpusmh->provider)->disk.flush(disk_dpusmh->handle,
        ptr, flush_completion);
}

static int
dpusm_disk_close(void *disk) {
    CHECK_HANDLE(disk, disk_dpusmh, DPUSM_ERROR);

    /* disk operations are optional */
    if (!FUNCS(disk_dpusmh->provider)->disk.close) {
        return DPUSM_NOT_IMPLEMENTED;
    }

    FUNCS(disk_dpusmh->provider)->disk.close(disk_dpusmh->handle);
    dpusm_handle_free(disk_dpusmh);
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
    .get_size           = dpusm_get_size,
    .free               = dpusm_free,
    .copy_from_mem      = dpusm_copy_from_mem,
    .copy_to_mem        = dpusm_copy_to_mem,
    .mem_stats          = dpusm_provider_mem_stats,
    .zero_fill          = dpusm_zero_fill,
    .all_zeros          = dpusm_all_zeros,
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
