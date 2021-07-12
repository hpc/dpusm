#ifndef _DATA_PROCESSING_UNIT_SERVICES_MODULE_PROVIDER_API_H
#define _DATA_PROCESSING_UNIT_SERVICES_MODULE_PROVIDER_API_H

#include <dpusm/common.h>

/* signatures of functions that the provider should implement */
typedef struct dpusm_provider_functions {
    /*
     * required
     */

    /* bitmasks of capabilities the provider brings */
    int (*capabilities)(dpusm_pc_t *caps);

    /* get a new offloader handle */
    void *(*alloc)(size_t size);

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
     * amount memory allocated
     * definitions of count/size vs actual_count/actual_size
     * will depend on the provider
     */
    int (*active)(size_t *count, size_t *size,
        size_t *actual_count, size_t *actual_size);

    /* fill in a buffer with zeros */
    int (*zero_fill)(void *handle, size_t offset, size_t size);

    /* whether or not a buffer is all zeros */
    int (*all_zeros)(void *handle, size_t offset, size_t size);

    /* effectively an array of allocations */
    void *(*create_gang)(void **handles, size_t count);

    /*
     * Data is passed in with the bufs array for transfer between the
     * offloader and memory.
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

        /* create new allocations for the parity columns */
        int (*new_parity)(void *raid, uint64_t raidn,
            void **new_dpusm_parity_cols, size_t *new_parity_sizes);

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
            loff_t offset, ssize_t *resid, int *err);
        void (*close)(void *fp_handle);
    } file;

    struct {
        /* open should return NULL on error */
        void *(*open)(const char *path,
            fmode_t mode, void *holder);
        int (*invalidate)(void *bdev_handle);
        int (*write)(void *bdev_handle, void *data,
            size_t io_size, uint64_t io_offset, int rw,
            int failfast, int flags, void *ptr,
            dpusm_disk_write_completion_t write_completion);
        int (*flush)(void *bdev_handle, void *ptr,
            dpusm_disk_flush_completion_t flush_completion);
        void (*close)(void *bdev_handle, fmode_t mode);
    } disk;
} dpusm_pf_t;

/* returns -ERRNO instead of DPUSM_* */
int dpusm_register_bsd(const char *name, const dpusm_pf_t *funcs);
int dpusm_unregister_bsd(const char *name);
int dpusm_register_gpl(const char *name, const dpusm_pf_t *funcs);
int dpusm_unregister_gpl(const char *name);

#endif
