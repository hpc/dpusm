#include <linux/module.h>

#include <dpusm/common.h>

const char *DPUSM_OPTIONAL_STR[] = {
    "mem_stats",
    "zero_fill",
    "all_zeros",
};

const char *DPUSM_COMPRESS_STR[] = {
    "GZIP Level 1",
    "GZIP Level 2",
    "GZIP Level 3",
    "GZIP Level 4",
    "GZIP Level 5",
    "GZIP Level 6",
    "GZIP Level 7",
    "GZIP Level 8",
    "GZIP Level 9",
    "LZ4",
};

const char **DPUSM_DECOMPRESS_STR = DPUSM_COMPRESS_STR;

const char *DPUSM_CHECKSUM_STR[] = {
    "Fletcher 4",
};

const char *DPUSM_CHECKSUM_BYTEORDER_STR[] = {
    "Native Byteorder",
    "Byteswap Byteorder",
};

const char *DPUSM_RAID_STR[] = {
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
