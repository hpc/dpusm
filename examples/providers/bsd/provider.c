#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <common.h>

static int __init
dpusm_bsd_provider_init(void) {
    const int rc = dpusm_register_bsd(THIS_MODULE,
        &example_dpusm_provider_functions);
    printk("%s init: %d\n", module_name(THIS_MODULE), rc);
    return rc;
}

static void __exit
dpusm_bsd_provider_exit(void) {
    dpusm_unregister_bsd(THIS_MODULE);

    printk("%s exit\n", module_name(THIS_MODULE));
}

module_init(dpusm_bsd_provider_init);
module_exit(dpusm_bsd_provider_exit);

MODULE_LICENSE("BSD-3");
