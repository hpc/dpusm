#ifndef _FILE_SERVICES_MODULE_COMMON_H
#define _FILE_SERVICES_MODULE_COMMON_H

#include <linux/types.h>

#define DPUSM_OK                  0
#define DPUSM_ERROR               1
#define DPUSM_PROVIDER_EXISTS     2
#define DPUSM_PROVIDER_NOT_EXISTS 3
#define DPUSM_PROVIDER_MISMATCH   5
#define DPUSM_NOT_IMPLEMENTED     6
#define DPUSM_BAD_RESULT          7

/* 0 should be considered invalid/not available when using these values */

typedef enum {
    DPUSM_OPTIONAL_ZERO_FILL     = 1 << 0,
    DPUSM_OPTIONAL_ALL_ZEROS     = 1 << 1,
    DPUSM_OPTIONAL_REALIGN       = 1 << 2,
} dpusm_optional_t;

/* used for both compression and decompression */
typedef enum {
    DPUSM_COMPRESS_GZIP_1  = 1 << 0,
    DPUSM_COMPRESS_GZIP_2  = 1 << 1,
    DPUSM_COMPRESS_GZIP_3  = 1 << 2,
    DPUSM_COMPRESS_GZIP_4  = 1 << 3,
    DPUSM_COMPRESS_GZIP_5  = 1 << 4,
    DPUSM_COMPRESS_GZIP_6  = 1 << 5,
    DPUSM_COMPRESS_GZIP_7  = 1 << 6,
    DPUSM_COMPRESS_GZIP_8  = 1 << 7,
    DPUSM_COMPRESS_GZIP_9  = 1 << 8,
    DPUSM_COMPRESS_LZ4     = 1 << 9,
} dpusm_compress_t;

typedef enum {
    DPUSM_CHECKSUM_FLETCHER_4 = 1 << 0,
} dpusm_checksum_t;

typedef enum {
    DPUSM_BYTEORDER_NATIVE   = 1 << 0,
    DPUSM_BYTEORDER_BYTESWAP = 1 << 1,
} dpusm_byteorder_t;

typedef enum {
    DPUSM_RAID_1_GEN = 1 << 0,
    DPUSM_RAID_2_GEN = 1 << 1,
    DPUSM_RAID_3_GEN = 1 << 2,
    DPUSM_RAID_1_REC = 1 << 3,
    DPUSM_RAID_2_REC = 1 << 4,
    DPUSM_RAID_3_REC = 1 << 5,
} dpusm_raid_t;

typedef enum {
    DPUSM_IO_FILE = 1 << 0,
    DPUSM_IO_DISK = 1 << 1,
} dpusm_io_t;

/* each member is a bitmask of capabilities */
typedef struct dpusm_provider_capabilities {
    int optional;
    int compress;
    int decompress;
    int checksum;
    int checksum_byteorder;
    int raid;
    int io;
} dpusm_pc_t;

/* expects only one bit will be set, so only returns first set bit */
static inline int enum2index(int mask) {
	if (mask == 0) return -1;

    int index = 0;
    while (!(mask & 1)) {
        mask >>= 1;
        index++;
    }
    return index;
}

/*
 * use this struct to copy data to and from offloader memory
 *
 * passing explicit offset because handle could be anything
 * so ((char *) handle) + offset probably won't make sense
 */
typedef struct dpusm_move {
    void *handle;
    size_t offset;
} dpusm_mv_t;

/* callback to run after completing writes */
typedef void (*dpusm_disk_write_completion_t)(void *ptr, int error);
typedef dpusm_disk_write_completion_t dpusm_dwc_t;

/* callback to run after completing flushes */
typedef void (*dpusm_disk_flush_completion_t)(void *ptr, int error);
typedef dpusm_disk_flush_completion_t dpusm_dfc_t;

#endif
