/* SPDX-License-Identifier: (GPL-2.0 WITH interfaces-note) OR BSD-3-Clause */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <dpusm/provider.h>
#include <dpusm/user_api.h>

/* global list of providers */
static dpusm_t dpusm;

int
dpusm_register_bsd(const char *name, const dpusm_pf_t *funcs) {
    return dpusm_provider_register(&dpusm, name, funcs);
}

int
dpusm_unregister_bsd(const char *name) {
    return dpusm_provider_unregister(&dpusm, name);
}

/* provider facing functions */
EXPORT_SYMBOL(dpusm_register_bsd);
EXPORT_SYMBOL(dpusm_unregister_bsd);

int
dpusm_register_gpl(const char *name, const dpusm_pf_t *funcs) {
    return dpusm_provider_register(&dpusm, name, funcs);
}

int
dpusm_unregister_gpl(const char *name) {
    return dpusm_provider_unregister(&dpusm, name);
}

/* provider facing functions */
EXPORT_SYMBOL_GPL(dpusm_register_gpl);
EXPORT_SYMBOL_GPL(dpusm_unregister_gpl);

void *
dpusm_get(const char *name) {
    return dpusm_provider_get(&dpusm, name);
}

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

    printk("DPUSM init\n");
    return 0;
}

static void __exit
dpusm_exit(void)
{
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
        dpusm_provider_unregister_handle(&dpusm, provider);
    }

    dpusm_provider_write_unlock(&dpusm);

    printk("DPUSM exit\n");
}

module_init(dpusm_init);
module_exit(dpusm_exit);

MODULE_LICENSE("Dual BSD/GPL");
