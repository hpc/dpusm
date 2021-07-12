#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>

#include <dpusm/user_api.h> /* the DPUSM API */

const dpusm_uf_t *dpusm = NULL;

static int __init
dpusm_no_provider_init(void) {
    dpusm = dpusm_initialize();
    if (!dpusm) {
        printk("%s error: Could not initialze DPUSM.\n", module_name(THIS_MODULE));
        return -EFAULT;
    }

    printk("%s ready to exit.\n", module_name(THIS_MODULE));

    return 0;
}

static void __exit
dpusm_no_provider_exit(void) {
    if (!dpusm) {
        printk("%s error: DPUSM disappeared before exit.\n", module_name(THIS_MODULE));
        return;
    }

    printk("%s exited\n", module_name(THIS_MODULE));
}

module_init(dpusm_no_provider_init);
module_exit(dpusm_no_provider_exit);

MODULE_LICENSE("OSS + BSD 3");
