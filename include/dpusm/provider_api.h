#ifndef _DATA_PROCESSING_UNIT_SERVICES_MODULE_PROVIDER_API_H
#define _DATA_PROCESSING_UNIT_SERVICES_MODULE_PROVIDER_API_H

#include <dpusm/common.h>

/* provider should copy whatever data it needs out of here */
typedef struct dpusm_provider_disk_data {
    const char *path;
    size_t path_len;
    fmode_t mode;
    void *holder;
} dpusm_dd_t;

/* signatures of functions that the provider should implement */
typedef struct dpusm_provider_functions {
    /*
     * required
     */

    /*
     * set algorithm bitmasks
     *
     * When the provider is registered, the DPUSM will call
     * this function once while gathering capabilities.
     */
    int (*algorithms)(int *compress, int *decompress,
                      int *checksum, int *checksum_byteorder,
                      int *raid);

    /* get a new offloader handle */
    void *(*alloc)(size_t size);

    /* reference an existing handle */
    void *(*alloc_ref)(void *src, size_t offset, size_t size);

    /* Get size data of a handle */
    int (*get_size)(void *handle, size_t *size, size_t *actual);

    /* free an offloader handle */
    void (*free)(void *handle);

    /* memory -> offloader */
    int (*copy_from_mem)(dpusm_mv_t *mv, const void *buf, size_t size);

    /* offloader -> memory */
    int (*copy_to_mem)(dpusm_mv_t *mv, void *buf, size_t size);

    /*
     * optional
     */

    /*
     * memory statistics
     * definition will depend on the provider, but in general:
     *
     *     total:  count of all allocations made (never decreases)
     *     count:  count of active allocations
     *     size:   total requested size of active allocations
     *     actual: total actual size of active allocations
     */
    int (*mem_stats)(size_t *total, size_t *count,
        size_t *size, size_t *actual);

    /* fill in a buffer with zeros */
    int (*zero_fill)(void *handle, size_t offset, size_t size);

    /* whether or not a buffer is all zeros */
    int (*all_zeros)(void *handle, size_t offset, size_t size);

    int (*compress)(dpusm_compress_t alg,
        void *src, void *dst,
        size_t s_len, int level,
        size_t *c_len);

    int (*decompress)(dpusm_decompress_t alg,
        void *src, void *dst, int level);

    int (*checksum)(dpusm_checksum_t alg,
        dpusm_checksum_byteorder_t order,
        void *data, size_t size, void *cksum);

    struct {
        /*
         * col_handles should contain both parity columns as well as
         * data columns. The order will depend on the user. The data
         * columns should normally be references to a src row.
         */
        void *(*alloc)(size_t nparity, size_t ndata,
            void **col_handles);

        void (*free)(void *raid);

        /* Erasure Code Generation */
        int (*gen)(void *raid);

        /* create new allocations for the parity columns */
        int (*new_parity)(void *raid, uint64_t raidn,
            void **new_parity_cols, size_t *new_parity_sizes);

        /*
         * compare the contents of 2 handles
         *
         * This really should not be tied to raid, but is
         * only used during parity verification of resilver
         */
        int (*cmp)(void *lhs_handle, void *rhs_handle, int *diff);

        /* Erasure Code Reconstruction */
        int (*rec)(void *raid, int *tgts, int ntgts);
    } raid;

    struct {
        /* open should return NULL on error */
        void *(*open)(const char *path, int flags, int mode);
        int (*write)(void *fp_handle, void *data, size_t size,
            size_t trailing_zeros, loff_t offset, ssize_t *resid,
            int *err);
        void (*close)(void *fp_handle);
    } file;

    struct {
        /* open should return NULL on error */
        void *(*open)(dpusm_dd_t *disk_data);
        int (*invalidate)(void *disk_handle);
        int (*write)(void *disk_handle, void *data,
            size_t io_size, size_t trailing_zeros,
            uint64_t io_offset, int failfast, int flags,
            void *ptr, dpusm_dwc_t write_completion);
        int (*flush)(void *disk_handle, void *ptr,
            dpusm_dfc_t flush_completion);
        void (*close)(void *private);
    } disk;
} dpusm_pf_t;

/* returns -ERRNO instead of DPUSM_* */
int dpusm_register_bsd(const char *name, const dpusm_pf_t *funcs);
int dpusm_unregister_bsd(const char *name);
int dpusm_register_gpl(const char *name, const dpusm_pf_t *funcs);
int dpusm_unregister_gpl(const char *name);

#endif
