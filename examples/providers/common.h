#ifndef _EXAMPLE_PROVIDER_COMMON_H
#define _EXAMPLE_PROVIDER_COMMON_H

#include <dpusm/provider_api.h>

/* required functions */
int dpusm_provider_algorithms(int *compress, int *decompress,
                              int *checksum, int *checksum_byteorder,
                              int *raid);
void *dpusm_provider_alloc(size_t size);
void *dpusm_provider_ref(void *src, size_t offset, size_t size);
void dpusm_provider_free(void *handle);
int dpusm_provider_copy_from_mem(dpusm_mv_t *mv, const void *buf, size_t size);
int dpusm_provider_copy_to_mem(dpusm_mv_t *mv, void *buf, size_t size);

/* filled callback struct */
extern const dpusm_pf_t example_dpusm_provider_functions;

#endif
