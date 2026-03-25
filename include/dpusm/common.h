#ifndef _FILE_SERVICES_MODULE_COMMON_H
#define _FILE_SERVICES_MODULE_COMMON_H

#include <linux/types.h>

#define DPUSM_OK                    0
#define DPUSM_ERROR                 1
#define DPUSM_PROVIDER_NOT_EXISTS   2 /* provider reference does not point to anything */
#define DPUSM_PROVIDER_UNREGISTERED 3 /* provider existed at one point, but reference is no longer valid */
#define DPUSM_PROVIDER_INVALIDATED  4 /* provider is still registered, but has been invalidated */
#define DPUSM_PROVIDER_MISMATCH     5 /* offloaded data is not located on the provider running the function */
#define DPUSM_NOT_IMPLEMENTED       6 /* function is not implemented */
#define DPUSM_NOT_SUPPORTED         7 /* function is implemented, but specific operation is not supported */
#define DPUSM_BAD_RESULT            8 /* function ran and returned an error */

/* 0 should be considered invalid/not available when using these values */

typedef enum {
    DPUSM_OPTIONAL_COPY_FROM_PTR         = 1 << 0,
    DPUSM_OPTIONAL_COPY_TO_PTR           = 1 << 1,
    DPUSM_OPTIONAL_COPY_FROM_SCATTERLIST = 1 << 2,
    DPUSM_OPTIONAL_COPY_TO_SCATTERLIST   = 1 << 3,
    DPUSM_OPTIONAL_ASSOCIATE_HANDLE      = 1 << 4,
    DPUSM_OPTIONAL_MEM_STATS             = 1 << 5,
    DPUSM_OPTIONAL_ZERO_FILL             = 1 << 6,
    DPUSM_OPTIONAL_ALL_ZEROS             = 1 << 7,

    DPUSM_OPTIONAL_MAX                   = 1 << 8,
} dpusm_optional_t;

extern const char *DPUSM_OPTIONAL_STR[];

/* used for both compression and decompression */
typedef enum {
    DPUSM_COMPRESS_GZIP_1  = 1ULL << 0,
    DPUSM_COMPRESS_GZIP_2  = 1ULL << 1,
    DPUSM_COMPRESS_GZIP_3  = 1ULL << 2,
    DPUSM_COMPRESS_GZIP_4  = 1ULL << 3,
    DPUSM_COMPRESS_GZIP_5  = 1ULL << 4,
    DPUSM_COMPRESS_GZIP_6  = 1ULL << 5,
    DPUSM_COMPRESS_GZIP_7  = 1ULL << 6,
    DPUSM_COMPRESS_GZIP_8  = 1ULL << 7,
    DPUSM_COMPRESS_GZIP_9  = 1ULL << 8,
    DPUSM_COMPRESS_LZ4     = 1ULL << 9,
    DPUSM_COMPRESS_ZSTD_1  = 1ULL << 10,
    DPUSM_COMPRESS_ZSTD_2  = 1ULL << 11,
    DPUSM_COMPRESS_ZSTD_3  = 1ULL << 12,
    DPUSM_COMPRESS_ZSTD_4  = 1ULL << 13,
    DPUSM_COMPRESS_ZSTD_5  = 1ULL << 14,
    DPUSM_COMPRESS_ZSTD_6  = 1ULL << 15,
    DPUSM_COMPRESS_ZSTD_7  = 1ULL << 16,
    DPUSM_COMPRESS_ZSTD_8  = 1ULL << 17,
    DPUSM_COMPRESS_ZSTD_9  = 1ULL << 18,
    DPUSM_COMPRESS_ZSTD_10 = 1ULL << 19,
    DPUSM_COMPRESS_ZSTD_11 = 1ULL << 20,
    DPUSM_COMPRESS_ZSTD_12 = 1ULL << 21,
    DPUSM_COMPRESS_ZSTD_13 = 1ULL << 22,
    DPUSM_COMPRESS_ZSTD_14 = 1ULL << 23,
    DPUSM_COMPRESS_ZSTD_15 = 1ULL << 24,
    DPUSM_COMPRESS_ZSTD_16 = 1ULL << 25,
    DPUSM_COMPRESS_ZSTD_17 = 1ULL << 26,
    DPUSM_COMPRESS_ZSTD_18 = 1ULL << 27,
    DPUSM_COMPRESS_ZSTD_19 = 1ULL << 28,
    DPUSM_COMPRESS_ZSTD_20 = 1ULL << 29,
    DPUSM_COMPRESS_ZSTD_21 = 1ULL << 30,
    DPUSM_COMPRESS_ZSTD_22 = 1ULL << 31,

    DPUSM_COMPRESS_MAX     = 1ULL << 32,
} dpusm_compress_t;

extern const char *DPUSM_COMPRESS_STR[];

typedef dpusm_compress_t dpusm_decompress_t;

extern const char *DPUSM_DECOMPRESS_STR[];

typedef enum {
    DPUSM_CHECKSUM_FLETCHER_2 = 1 << 0,
    DPUSM_CHECKSUM_FLETCHER_4 = 1 << 1,
    DPUSM_CHECKSUM_SHA224     = 1 << 2,
    DPUSM_CHECKSUM_SHA256     = 1 << 3,
    DPUSM_CHECKSUM_SHA384     = 1 << 4,
    DPUSM_CHECKSUM_SHA512     = 1 << 5,
    DPUSM_CHECKSUM_MAX        = 1 << 6,
} dpusm_checksum_t;

extern const char *DPUSM_CHECKSUM_STR[];

typedef enum {
    DPUSM_BYTEORDER_NATIVE   = 1 << 0,
    DPUSM_BYTEORDER_BYTESWAP = 1 << 1,

    DPUSM_BYTEORDER_MAX      = 1 << 2,
} dpusm_checksum_byteorder_t;

extern const char *DPUSM_CHECKSUM_BYTEORDER_STR[];

typedef enum {
    DPUSM_RAID_1_GEN = 1 << 1,
    DPUSM_RAID_2_GEN = 1 << 2,
    DPUSM_RAID_3_GEN = 1 << 3,
    DPUSM_RAID_1_REC = 1 << 4,
    DPUSM_RAID_2_REC = 1 << 5,
    DPUSM_RAID_3_REC = 1 << 6,

    DPUSM_RAID_MAX   = 1 << 7,

    /* don't pass into enum2index */
    DPUSM_RAID_GEN   = DPUSM_RAID_1_GEN | DPUSM_RAID_2_GEN | DPUSM_RAID_3_GEN,
    DPUSM_RAID_REC   = DPUSM_RAID_1_REC | DPUSM_RAID_2_REC | DPUSM_RAID_3_REC,
} dpusm_raid_t;

extern const char *DPUSM_RAID_STR[];

typedef enum {
    DPUSM_IO_FILE = 1 << 0,
    DPUSM_IO_DISK = 1 << 1,

    DPUSM_IO_MAX  = 1 << 2,
} dpusm_io_t;

extern const char *DPUSM_IO_STR[];

/* each member is a bitmask of capabilities */
typedef struct dpusm_provider_capabilities {
    int optional;             // dpusm_optional_t
    int compress;             // dpusm_compress_t
    int decompress;           // dpusm_decompress_t
    int checksum;             // dpusm_checksum_t
    int checksum_byteorder;   // dpusm_byteorder_t
    int raid;                 // dpusm_raid_t
    int io;                   // dpusm_io_t
} dpusm_pc_t;

/* expects only one bit will be set, so only returns first set bit */
int enum2index(int mask);
const char *enum2str(const char **strs, int mask);

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
