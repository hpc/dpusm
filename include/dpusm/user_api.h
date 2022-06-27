#ifndef _DATA_PROCESSING_UNIT_SERVICES_MODULE_USER_API_H
#define _DATA_PROCESSING_UNIT_SERVICES_MODULE_USER_API_H

#ifdef _KERNEL
#include <linux/blkdev.h>
#endif

#include <dpusm/common.h>

/*
 * Signatures of functions that the user should call
 *
 * The DPUSM will define all of these
 * functions, but providers might not.
 */
typedef struct dpusm_user_functions {
    /*
     * required
     */

    /* get from the registry */
    void *(*get)(const char *name);

    /* get name of provider */
    const char *(*get_name)(void *provider);

    /* return to the registry */
    int (*put)(void *provider);

    /* extract the provider from a handle */
    void *(*extract)(void *handle);

    /* capabilities provided by the provider */
    int (*capabilities)(void *provider, dpusm_pc_t **caps);

    /* get a new handle */
    void *(*alloc)(void *provider, size_t size);

    /* reference an existing handle */
    void *(*alloc_ref)(void *src, size_t offset, size_t size);

    /* Get size data of a handle */
    int (*get_size)(void *handle, size_t *size, size_t *actual);

    /* free a handle */
    void (*free)(void *handle);

    /* memory -> provider */
    int (*copy_from_mem)(dpusm_mv_t *mv, const void *buf, size_t size);

    /* provider -> memory */
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
    int (*mem_stats)(void *provider,
                     size_t *t_count, size_t *t_size, size_t *t_actual,
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
        int (*can_compute)(void *provider, size_t nparity, size_t ndata,
            size_t *col_sizes, int rec);

        /*
         * col_handles should contain both parity columns as well as
         * data columns. The order will depend on the user. The data
         * columns should normally be references to src. src is used
         * to make sure all of the columns are located on the same
         * provider. col_sizes should be the amount of data from each
         * column that is used to calculate parity bits, which may be
         * smaller than the allocated size.
         */
        void *(*alloc)(size_t nparity, size_t ndata,
            void *src, void **col_handles, size_t *col_sizes);

        void (*free)(void *raid);

        /* Erasure Code Generation */
        int (*gen)(void *raid);

        /*
         * create new allocations for the parity columns
         *
         * columns whose new_parity_sizes[c] == 0 will not be allocated
         */
        int (*new_parity)(void *raid, uint64_t raidn,
            void ***new_parity_cols, size_t *new_parity_sizes);

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
        void *(*open)(void *provider, const char *path, int flags, int mode);
        int (*write)(void *fp_handle, void *data, size_t size,
             size_t trailing_zeros, loff_t offset, ssize_t *resid, int *err);
        int (*close)(void *fp_handle);
    } file;

#ifdef _KERNEL
    struct {
        /* open should return NULL on error */
        void *(*open)(void *provider, const char *path, struct block_device *bdev);
        int (*invalidate)(void *disk_handle);
        int (*write)(void *disk_handle, void *data,
            size_t io_size, size_t trailing_zeros,
            uint64_t io_offset, int failfast, int flags,
            void *ptr, dpusm_dwc_t write_completion);
        int (*flush)(void *disk_handle, void *ptr,
            dpusm_dfc_t flush_completion);
        int (*close)(void *disk_handle);
    } disk;
#endif
} dpusm_uf_t;

/*
 * Caller should check to see if these functions are defined.
 * If not, the DPUSM is not loaded.
 */
const dpusm_uf_t *dpusm_initialize(void) __attribute__((weak));
int dpusm_finalize(void) __attribute__((weak));

#endif
