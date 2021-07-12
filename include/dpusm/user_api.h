#ifndef _DATA_PROCESSING_UNIT_SERVICES_MODULE_USER_API_H
#define _DATA_PROCESSING_UNIT_SERVICES_MODULE_USER_API_H

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
    int (*capabilities)(void *provider, dpusm_pc_t *caps);

    /* get a new handle */
    void *(*alloc)(void *provider, size_t size);

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
     * amount memory allocated
     * definitions of count/size vs actual_count/actual_size
     * will depend on the provider
     */
    int (*active)(void *provider, size_t *count, size_t *size,
        size_t *actual_count, size_t *actual_size);

    /* fill in a buffer with zeros */
    int (*zero_fill)(void *handle, size_t offset, size_t size);

    /* whether or not a buffer is all zeros */
    int (*all_zeros)(void *handle, size_t offset, size_t size);

    /* effectively an array of allocations */
    void *(*create_gang)(void **handles, size_t count);

    /*
     * Data is passed in with the bufs array for transfer between the
     * provider and memory.
     *
     * The arrays have a short lifetime, but the data inside being
     * pointed to have the same lifetime as the structure they came
     * from.
     */
    int (*bulk_from_mem)(void *handle, void **bufs,
        size_t *sizes, size_t count);
    int (*bulk_to_mem)  (void *handle, void **bufs,
        size_t *sizes, size_t count);

    int (*compress)(dpusm_compress_t alg,
        void *src, void *dst,
        size_t s_len, int level,
        size_t *c_len);

    int (*decompress)(dpusm_compress_t alg,
        void *src, void *dst, int level);

    int (*checksum)(dpusm_checksum_t alg, dpusm_byteorder_t order,
        void *data, size_t size, void *cksum);

    struct {
        /* parity_col_dpusm_handles are created by the caller */
        void *(*alloc)(uint64_t raidn, uint64_t acols,
            void *src_handle, uint64_t *sizes,
            void **parity_col_dpusm_handles);

        void (*free)(void *raid);

        /* Erasure Code Generation */
        int (*gen)(void *raid);

        /*
         * create new allocations for the parity columns
         *
         * columns whose new_parity_sizes[c] == 0 will not be allocated
         */
        int (*new_parity)(void *raid, uint64_t raidn,
            void ***new_dpusm_parity_cols, size_t *new_parity_sizes);

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
            loff_t offset, ssize_t *resid, int *err);
        int (*close)(void *fp_handle);
    } file;

#ifdef _KERNEL
    struct {
        /* open should return NULL on error */
        void *(*open)(void *provider, const char *path,
            fmode_t mode, void *holder);
        int (*invalidate)(void *bdev_handle);
        int (*write)(void *bdev_handle, void *data,
            size_t io_size, uint64_t io_offset, int rw,
            int failfast, int flags, void *ptr,
            dpusm_disk_write_completion_t write_completion);
        int (*flush)(void *bdev_handle, void *ptr,
            dpusm_disk_flush_completion_t flush_completion);
        int (*close)(void *bdev_handle, fmode_t mode);
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
