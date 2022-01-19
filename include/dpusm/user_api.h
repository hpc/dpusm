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

    /* reference an existing handle */
    void *(*alloc_ref)(void *src, size_t offset, size_t size);

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
     * amount of memory allocated
     * definitions of count/size vs actual_count/actual_size
     * will depend on the provider
     */
    int (*active)(void *provider, size_t *count, size_t *size,
        size_t *actual_count, size_t *actual_size);

    /* fill in a buffer with zeros */
    int (*zero_fill)(void *handle, size_t offset, size_t size);

    /* whether or not a buffer is all zeros */
    int (*all_zeros)(void *handle, size_t offset, size_t size);

    /* move (if necessary) a buffer to an aligned address */
    int (*realign)(void *src, void **dst, size_t aligned_size, size_t alignment);

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
        /*
         * col_dpusm_handles order:
         *     [0, raidn)    - parity
         *     [raidn, cols) - data
         */
        void *(*alloc)(uint64_t raidn, uint64_t cols,
            void *src_handle, void **col_dpusm_handles);

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
        int (*invalidate)(void *disk_handle);
        int (*write)(void *disk_handle, void *data,
            size_t io_size, uint64_t io_offset,
            int failfast, int flags, void *ptr,
            dpusm_disk_write_completion_t write_completion);
        int (*flush)(void *disk_handle, void *ptr,
            dpusm_disk_flush_completion_t flush_completion);
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
