#ifndef _DATA_PROCESSING_UNIT_SERVICES_MODULE_ALLOC_H
#define _DATA_PROCESSING_UNIT_SERVICES_MODULE_ALLOC_H

#include <linux/types.h>

void dpusm_mem_init(void);
void *dpusm_mem_alloc(size_t size);
void dpusm_mem_free(void *ptr, size_t size);
void dpusm_mem_stats(size_t *total, size_t *count, size_t *size);

#endif
