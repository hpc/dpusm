#ifndef _DATA_PROCESSING_UNIT_SERVICES_MODULE_PROVIDER_H
#define _DATA_PROCESSING_UNIT_SERVICES_MODULE_PROVIDER_H

#include <linux/atomic.h>
#include <linux/list.h>
#include <linux/spinlock.h>

#include <dpusm/provider_api.h>

/* single provider data */
typedef struct dpusm_provider_handle {
    const char *name;        /* reference to a string */
    dpusm_pc_t capabilities; /* constant set of capabilities */
    const dpusm_pf_t *funcs; /* reference to a struct */
    atomic_t refs;           /* how many users are holding this provider */
    struct list_head list;
} dpusm_ph_t;

typedef struct {
    struct list_head providers;  /* list of providers */
    size_t count;                /* count of registered providers */
    rwlock_t lock;
    atomic_t active;             /* how many providers are active (may be larger than count) */
                                 /* this is not tied to the provider/count */
} dpusm_t;

int dpusm_provider_register(dpusm_t *dpusm, const char *name, const dpusm_pf_t *funcs);

/* can't prevent provider module from unloading */
int dpusm_provider_unregister_handle(dpusm_t *dpusm, dpusm_ph_t *provider);
int dpusm_provider_unregister(dpusm_t *dpusm, const char *name);

dpusm_ph_t *dpusm_provider_get(dpusm_t *dpusm, const char *name);
int dpusm_provider_put(dpusm_t *dpusm, void *handle);

void dpusm_provider_write_lock(dpusm_t *dpusm);
void dpusm_provider_write_unlock(dpusm_t *dpusm);

/*
 * call when backing DPU goes down unexpectedly
 *
 * provider is not unregistered, so dpusm_unregister still needs to be called
 */
void dpusm_provider_invalidate(dpusm_t *dpusm, const char *name);

#endif
