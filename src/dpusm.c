/* SPDX-License-Identifier: (GPL-2.0 WITH interfaces-note) OR BSD-3-Clause */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <dpusm/alloc.h>
#include <dpusm/provider.h>

/* global list of providers */
static dpusm_t dpusm;

int
dpusm_register_bsd(struct module *module, const dpusm_pf_t *funcs) {
    return dpusm_provider_register(&dpusm, module, funcs);
}

int
dpusm_unregister_bsd(struct module *module) {
    return dpusm_provider_unregister(&dpusm, module);
}

/* provider facing functions */
EXPORT_SYMBOL(dpusm_register_bsd);
EXPORT_SYMBOL(dpusm_unregister_bsd);

int
dpusm_register_gpl(struct module *module, const dpusm_pf_t *funcs) {
    return dpusm_provider_register(&dpusm, module, funcs);
}

int
dpusm_unregister_gpl(struct module *module) {
    return dpusm_provider_unregister(&dpusm, module);
}

/* provider facing functions */
EXPORT_SYMBOL_GPL(dpusm_register_gpl);
EXPORT_SYMBOL_GPL(dpusm_unregister_gpl);

/*
 * Call when backing DPU goes down unexpectedly
 *
 * Provider is not unregistered, so dpusm_unregister still needs to be called.
 * Using name instead of handle because the provider handle is not available to the provider.
 */
void dpusm_invalidate(const char *name) {
    dpusm_provider_invalidate(&dpusm, name);
}
EXPORT_SYMBOL(dpusm_invalidate);

/* called by user.c */
void *
dpusm_get(const char *name) {
    return dpusm_provider_get(&dpusm, name);
}

/* called by user.c */
int
dpusm_put(void *handle) {
    return dpusm_provider_put(&dpusm, handle);
}

/* set up providers list */
static int __init
dpusm_init(void) {
    INIT_LIST_HEAD(&dpusm.providers);
    dpusm.count = 0;
    rwlock_init(&dpusm.lock);

    atomic_set(&dpusm.active, 0);
    dpusm_mem_init();

    printk("DPUSM init\n");
    return 0;
}

static void __exit
dpusm_exit(void) {
    dpusm_provider_write_lock(&dpusm);

    const int active = atomic_read(&dpusm.active);
    if (unlikely(active)) {
        printk("Exiting with %d active references to providers\n", active);
    }

    if (unlikely(dpusm.count)) {
        printk("Somehow exiting with %zu providers still registered.\n",
            dpusm.count);
    }

    /*
     * this should not print unless there are active
     * users or providers still registered
     */
    struct list_head *it = NULL;
    struct list_head *next = NULL;
    list_for_each_safe(it, next, &dpusm.providers) {
        dpusm_ph_t *provider = list_entry(it, dpusm_ph_t, list);
        dpusm_provider_unregister_handle(&dpusm, &provider->self);
    }

    dpusm_provider_write_unlock(&dpusm);

#if DPUSM_TRACK_ALLOCS
    size_t alloc_count = 0;
    size_t active_count = 0;
    size_t active_size = 0;
    dpusm_mem_stats(&alloc_count, &active_count, &active_size);
    printk("DPUSM exit with %zu bytes in %zu/%zu allocations\n",
           active_size, active_count, alloc_count);
#else
    printk("DPUSM exit\n");
#endif
}

module_init(dpusm_init);
module_exit(dpusm_exit);

MODULE_LICENSE("Dual BSD/GPL");
