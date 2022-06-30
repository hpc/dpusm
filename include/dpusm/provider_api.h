#ifndef _DATA_PROCESSING_UNIT_SERVICES_MODULE_PROVIDER_API_H
#define _DATA_PROCESSING_UNIT_SERVICES_MODULE_PROVIDER_API_H

#ifdef _KERNEL
#include <linux/blkdev.h>
#endif

#include <dpusm/common.h>

/* provider should copy whatever data it needs out of here */
typedef struct dpusm_provider_disk_data {
    const char *path;
    size_t path_len;
    struct block_device *bdev;
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
     *  count:  how many allocations there are
     *  size:   how much space was requested
     *  actual: how much space was actually provided
     *
     *  t_*:    total - never decreases
     *  a_*:    active allocations
     */
    int (*mem_stats)(size_t *t_count, size_t *t_size, size_t *t_actual,
                     size_t *a_count, size_t *a_size, size_t *a_actual);

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
        void *data, size_t size,
        void *cksum, size_t cksum_size);

    struct {
        int (*can_compute)(size_t nparity, size_t ndata,
            size_t *col_sizes, int rec);

        /*
         * col_handles should contain both parity columns as well as
         * data columns. The order will depend on the user. col_sizes
         * should be the amount of data from each column that is used
         * to calculate parity bits, which may be smaller than the
         * allocated size.
         */
        void *(*alloc)(size_t nparity, size_t ndata,
            void **col_handles, size_t *col_sizes);

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
        /* returns E errors */
        int (*write)(void *fp_handle, void *data, size_t size,
            size_t trailing_zeros, loff_t offset, ssize_t *resid,
            int *err);
        void (*close)(void *fp_handle);
    } file;

    struct {
        /* open should return NULL on error */
        void *(*open)(dpusm_dd_t *disk_data);
        int (*invalidate)(void *disk_handle);
        /* returns E errors */
        int (*write)(void *disk_handle, void *data, size_t data_size,
            size_t trailing_zeros, uint64_t io_offset, int flags,
            dpusm_dwc_t write_completion, void *wc_args);
        /* returns E errors */
        int (*flush)(void *disk_handle, dpusm_dfc_t flush_completion,
            void *fc_args);
        void (*close)(void *private);
    } disk;
} dpusm_pf_t;

/* returns -ERRNO instead of DPUSM_* */
int dpusm_register_bsd(const char *name, const dpusm_pf_t *funcs);
int dpusm_unregister_bsd(const char *name);
int dpusm_register_gpl(const char *name, const dpusm_pf_t *funcs);
int dpusm_unregister_gpl(const char *name);

/*
 * call when backing DPU goes down unexpectedly
 *
 * provider is not unregistered, so dpusm_unregister still needs to be called
 */
void dpusm_invalidate(const char *name);

#endif
