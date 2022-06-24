#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>

#include <dpusm/user_api.h>     /* the DPUSM User API */
#include <dpusm/provider_api.h> /* the DPUSM Provider API (not normally included by users) */

const char   TEST_BUF[]    = "HELLO WORLD";
const size_t TEST_BUF_LEN  = sizeof(TEST_BUF) - 1;

const dpusm_uf_t *dpusm = NULL;
void *provider = NULL;

static int
use_provider(const char *provider_name, bool invalidate) {
    provider = dpusm->get(provider_name);
    if (!provider) {
        printk("%s error: Could not find \"%s\".\n", module_name(THIS_MODULE), provider_name);
        return -ENODEV;
    }

    if (invalidate) {
        dpusm_invalidate(provider_name);
    }

    /* new offloader allocation */
    void *handle = dpusm->alloc(provider, TEST_BUF_LEN + 1);
    if (!handle) {
        if (invalidate) {
            goto put;
        }
        else {
            BUG_ON(!handle);
        }
    }

    /* copy memory buffer to offloader */
    dpusm_mv_t mv_off = { .handle = handle, .offset = 0 };
    dpusm->copy_from_mem(&mv_off, TEST_BUF, TEST_BUF_LEN);

    /* new memory allocation */
    void *buf = kmalloc(TEST_BUF_LEN, GFP_KERNEL);
    BUG_ON(!buf);

    /* copy offloader data to memory */
    dpusm_mv_t mv_on = { .handle = handle, .offset = 0 };
    dpusm->copy_to_mem(&mv_on, buf, TEST_BUF_LEN);

    /* contents should match */
    BUG_ON(memcmp(buf, TEST_BUF, TEST_BUF_LEN));

    kfree(buf);

    /* free offloader allocation */
    dpusm->free(handle);

  put:
    dpusm->put(provider);

    return 0;
}

static int __init
dpusm_need_provider_init(void) {
    dpusm = dpusm_initialize();
    if (!dpusm) {
        printk("%s error: Could not initialze DPUSM.\n", module_name(THIS_MODULE));
        return -EFAULT;
    }

    int rc = 0;
    rc = (rc == 0)?use_provider("example_bsd_dpusm_provider", 0):rc;
    rc = (rc == 0)?use_provider("example_gpl_dpusm_provider", 0):rc;

    /* down providers while they are in use */
    use_provider("example_bsd_dpusm_provider", 1);
    use_provider("example_gpl_dpusm_provider", 1);

    printk("%s ready to exit.\n", module_name(THIS_MODULE));

    return rc;
}

static void __exit
dpusm_need_provider_exit(void) {
    if (!dpusm) {
        printk("%s error: DPUSM disappeared before exit.\n", module_name(THIS_MODULE));
        return;
    }

    dpusm_finalize();

    printk("%s exited\n", module_name(THIS_MODULE));
}

module_init(dpusm_need_provider_init);
module_exit(dpusm_need_provider_exit);

MODULE_LICENSE("OSS + BSD 3");
