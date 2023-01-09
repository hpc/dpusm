#ifndef _DATA_PROCESSING_UNIT_SERVICES_MODULE_USER_API_H
#define _DATA_PROCESSING_UNIT_SERVICES_MODULE_USER_API_H

#ifdef _KERNEL
#include <linux/blkdev.h>
#include <linux/scatterlist.h>
#else
/*
 * forward declare structs to remove compiler errors due to undeclared
 * types - functions will fail if these structs are not defined, so
 * generally don't use outside of kernel
 */
struct block_device;
struct scatterlist;
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
    int (*free)(void *handle);

    struct {
        /* memory -> offloader */
        struct {
            /* required */
            /* pass in an address that should not be directly accessed */
            int (*generic)(dpusm_mv_t *mv, const void *buf, size_t size);

            /* optional */
            /* pass in an address that can be directly accessed (i.e. DMA) */
            int (*ptr)(dpusm_mv_t *mv, const void *buf, size_t size);

            /* optional */
            /* only call in kernel */
            int (*scatterlist)(dpusm_mv_t *mv,
                struct scatterlist *sgl,
                unsigned int nents,
                size_t size);
        } from;

        /* offloader -> memory */
        struct {
            /* required */
            /* pass in an address that should not be directly accessed */
            int (*generic)(dpusm_mv_t *mv, void *buf, size_t size);

            /* optional */
            /* pass in an address that can be directly accessed (i.e. DMA) */
            int (*ptr)(dpusm_mv_t *mv, void *buf, size_t size);

            /* optional */
            /* only call in kernel */
            int (*scatterlist)(dpusm_mv_t *mv,
                struct scatterlist *sgl,
                unsigned int nents,
                size_t size);
        } to;
    } copy;

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

        /* set up an empty RAID structure */
        void *(*alloc)(void *provider, size_t nparity, size_t ndata);

        /* fill in single column of RAID structure */
        int (*set_column)(void *raid, uint64_t c, void *col, size_t size);

        int (*free)(void *raid);

        /* Erasure Code Generation */
        int (*gen)(void *raid);

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
        /* returns E errors */
        int (*write)(void *fp_handle, void *data, size_t size,
             size_t trailing_zeros, loff_t offset, ssize_t *resid, int *err);
        int (*close)(void *fp_handle);
    } file;

    struct {
        /* open should return NULL on error */
        void *(*open)(void *provider, const char *path, struct block_device *bdev);
        int (*invalidate)(void *disk_handle);
        /* returns E errors */
        int (*write)(void *disk_handle, void *data, size_t data_size,
            size_t trailing_zeros, uint64_t io_offset, int flags,
            dpusm_dwc_t write_completion, void *wc_args);
        /* returns E errors */
        int (*flush)(void *disk_handle, dpusm_dfc_t flush_completion,
            void *fc_args);
        int (*close)(void *disk_handle);
    } disk;
} dpusm_uf_t;

/*
 * Caller should check to see if these functions are defined.
 * If not, the DPUSM is not loaded.
 */
const dpusm_uf_t *dpusm_initialize(void) __attribute__((weak));
int dpusm_finalize(void) __attribute__((weak));

#endif
