#include <linux/module.h>

#include <dpusm/common.h>

const char *DPUSM_OPTIONAL_STR[] = {
    "copy_from_ptr",
    "copy_to_ptr",
    "copy_from_scatterlist",
    "copy_to_scatterlist",
    "mem_stats",
    "zero_fill",
    "all_zeros",
};

const char *DPUSM_COMPRESS_STR[] = {
    "GZIP Level 1 Compress",
    "GZIP Level 2 Compress",
    "GZIP Level 3 Compress",
    "GZIP Level 4 Compress",
    "GZIP Level 5 Compress",
    "GZIP Level 6 Compress",
    "GZIP Level 7 Compress",
    "GZIP Level 8 Compress",
    "GZIP Level 9 Compress",
    "LZ4 Compress",
    "ZSTD Compress",
};

const char *DPUSM_DECOMPRESS_STR[] = {
    "GZIP Level 1 Decompress",
    "GZIP Level 2 Decompress",
    "GZIP Level 3 Decompress",
    "GZIP Level 4 Decompress",
    "GZIP Level 5 Decompress",
    "GZIP Level 6 Decompress",
    "GZIP Level 7 Decompress",
    "GZIP Level 8 Decompress",
    "GZIP Level 9 Decompress",
    "LZ4 Decompress",
    "ZSTD Decompress",
};

const char *DPUSM_CHECKSUM_STR[] = {
    "Fletcher 2",
    "Fletcher 4",
    "SHA224",
    "SHA256",
    "SHA384",
    "SHA512",
};

const char *DPUSM_CHECKSUM_BYTEORDER_STR[] = {
    "Native Byteorder",
    "Byteswap Byteorder",
};

const char *DPUSM_RAID_STR[] = {
    "RAID INVALID",
    "RAID 1 GEN",
    "RAID 2 GEN",
    "RAID 3 GEN",
    "RAID 1 REC",
    "RAID 2 REC",
    "RAID 3 REC",
};

const char *DPUSM_IO_STR[] = {
    "File I/O",
    "Disk I/O",
};

/* expects only one bit will be set, so only returns first set bit */
int enum2index(int mask) {
	if (mask == 0) return -1;

    int index = 0;
    while (!(mask & 1)) {
        mask >>= 1;
        index++;
    }
    return index;
}

EXPORT_SYMBOL(enum2index);

const char *enum2str(const char **strs, int mask) {
    return strs[enum2index(mask)];
}
