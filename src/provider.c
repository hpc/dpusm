#include <linux/slab.h>

#include <dpusm/provider.h>

dpusm_ph_t *
dpusmph_init(const char *name, const dpusm_pf_t *funcs)
{
    dpusm_ph_t *dpusmph = kmalloc(sizeof(dpusm_ph_t), GFP_KERNEL);
    if (dpusmph) {
        dpusmph->name = name;
        dpusmph->funcs = funcs;
        atomic_set(&dpusmph->refs, 0);
    }

    return dpusmph;
}

void
dpusmph_destroy(dpusm_ph_t *dpusmph)
{
    kfree(dpusmph);
}

/* masks for bad function groups */
static const int DPUSM_PROVIDER_BAD_STRUCT   = (1 << 0);
static const int DPUSM_PROVIDER_BAD_REQUIRED = (1 << 1);
static const int DPUSM_PROVIDER_BAD_BULK     = (1 << 2);
static const int DPUSM_PROVIDER_BAD_RAID_GEN = (1 << 3);
static const int DPUSM_PROVIDER_BAD_RAID_REC = (1 << 4);
static const int DPUSM_PROVIDER_BAD_FILE     = (1 << 5);
static const int DPUSM_PROVIDER_BAD_DISK     = (1 << 6);

/* check provider sanity when loading */
static int
dpusm_provider_sane_at_load(const dpusm_pf_t *funcs)
{
    if (!funcs) {
        return DPUSM_PROVIDER_BAD_STRUCT;
    }

    /*
      active, zero_fill, all_zeros, realign, compress, decompress, and
      checksum may be defined as needed. They are not grouped together
      like the other functions.
    */

    const int required = (
        !!funcs->capabilities +
        !!funcs->alloc +
        !!funcs->alloc_ref +
        !!funcs->free +
        !!funcs->copy_from_mem +
        !!funcs->copy_to_mem);

    const int bulk = (
        !!funcs->bulk_from_mem +
        !!funcs->bulk_to_mem);

    const int raid_gen = (
        !!funcs->raid.alloc +
        !!funcs->raid.free +
        !!funcs->raid.gen);

    const int raid_rec = (
        !!funcs->raid.new_parity +
        !!funcs->raid.cmp +
        !!funcs->raid.rec);

    const int file = (
            !!funcs->file.open +
            !!funcs->file.write +
            !!funcs->file.close);

    const int disk = (
            !!funcs->disk.open +
            !!funcs->disk.invalidate +
            !!funcs->disk.write +
            !!funcs->disk.flush +
            !!funcs->disk.close);

    // get bitmap of bad function groups
    const int rc = (
        (!(required == 6)?DPUSM_PROVIDER_BAD_REQUIRED:0) |
        (!((bulk == 0) || (bulk == 2))?DPUSM_PROVIDER_BAD_BULK:0) |
        (!((raid_gen == 0) || (raid_gen == 3))?DPUSM_PROVIDER_BAD_RAID_GEN:0) |
        (!((raid_rec == 0) || ((raid_gen == 3) && (raid_rec == 3)))?DPUSM_PROVIDER_BAD_RAID_REC:0) |
        (!((file == 0) || (file == 3))?DPUSM_PROVIDER_BAD_FILE:0) |
        (!((disk == 0) || (disk == 5))?DPUSM_PROVIDER_BAD_DISK:0)
    );

    return rc;
}

/* simple linear search */
/* caller locks */
static dpusm_ph_t *
find_provider(dpusm_t *dpusm, const char *name) {
    const size_t name_len = strlen(name);

    struct list_head *it = NULL;
    list_for_each(it, &dpusm->providers) {
        dpusm_ph_t *dpusmph = list_entry(it, dpusm_ph_t, list);
        const char *p_name = dpusmph->name;
        const size_t p_name_len = strlen(p_name);
        if (name_len == p_name_len) {
            if (memcmp(name, p_name, p_name_len) == 0) {
                return dpusmph;
            }
        }
    }

    return NULL;
}

/* add a new provider */
int
dpusm_provider_register(dpusm_t *dpusm, const char *name, const dpusm_pf_t *funcs) {
    const int rc = dpusm_provider_sane_at_load(funcs);
    if (rc != DPUSM_OK) {
        printk("DPUSM Provider \"%s\" does not provide "
            "a valid set of functions. Bad function bitmap: %d\n", name, rc);
        return -EINVAL;
    }

    dpusm_provider_write_lock(dpusm);

    dpusm_ph_t *provider = find_provider(dpusm, name);
    if (provider) {
        printk("DPUSM Provider with the name \"%s\" (%p) already exists. %zu providers registered.\n",
               name, provider, dpusm->count);
        dpusm_provider_write_unlock(dpusm);
        return -EEXIST;
    }

    provider = dpusmph_init(name, funcs);
    if (!provider) {
        dpusm_provider_write_unlock(dpusm);
        return -ECANCELED;
    }

    list_add(&provider->list, &dpusm->providers);
    dpusm->count++;
    printk("DPUSM Provider \"%s\" (%p) added. Now %zu providers registered.\n",
        name, provider, dpusm->count);

    dpusm_provider_write_unlock(dpusm);

    return 0;
}

/* remove provider from list */
/* can't prevent provider module from unloading */
/* locking is done by caller */
int
dpusm_provider_unregister_handle(dpusm_t *dpusm, dpusm_ph_t *provider) {
    if (!provider) {
        printk("dpusm_provider_unregister: Bad provider handle.\n");
        return -EINVAL;
    }

    int rc = 0;
    const int refs = atomic_read(&provider->refs);
    if (refs) {
        printk("Unregistering provider \"%s\" with %d references remaining.\n", provider->name, refs);
        rc = -EBUSY;
    }

    list_del(&provider->list);
    atomic_sub(refs, &dpusm->active); /* remove this provider's references from the global active count */
    printk("Unregistered %s\n", provider->name);

    dpusmph_destroy(provider);
    dpusm->count--;

    return rc;
}

int
dpusm_provider_unregister(dpusm_t *dpusm, const char *name) {
    dpusm_provider_write_lock(dpusm);

    dpusm_ph_t *provider = find_provider(dpusm, name);
    if (!provider) {
        printk("dpusm_provider_unregister: Could not find provider with name \"%s\"\n", name);
        dpusm_provider_write_unlock(dpusm);
        return DPUSM_ERROR;
    }

    const int rc = dpusm_provider_unregister_handle(dpusm, provider);

    dpusm_provider_write_unlock(dpusm);
    return rc;
}

/*
 * called by the user, but is located here
 * to have access to the global list
 */

/* get a provider by name */
dpusm_ph_t *
dpusm_provider_get(dpusm_t *dpusm, const char *name) {
    read_lock(&dpusm->lock);
    dpusm_ph_t *provider = find_provider(dpusm, name);
    if (provider) {
        atomic_inc(&provider->refs);
        atomic_inc(&dpusm->active);
        printk("dpusm_get: User has been given a handle to \"%s\" (now %d users).\n", name, atomic_read(&provider->refs));
    }
    else {
        printk("dpusm_get Error: Did not find provider \"%s\"\n", name);
    }
    read_unlock(&dpusm->lock);
    return provider;
}

/* declare that the provider is no longer being used by the caller */
int
dpusm_provider_put(dpusm_t *dpusm, void *handle) {
    dpusm_ph_t *provider = (dpusm_ph_t *) handle;
    if (!provider) {
        printk("dpusm_put Error: Invalid handle\n");
        return DPUSM_ERROR;
    }

    if (!atomic_read(&provider->refs)) {
        printk("dpusm_put Error: Cannot decrement provider \"%s\" user count already at 0.\n", provider->name);
        return DPUSM_ERROR;
    }

    atomic_dec(&provider->refs);
    atomic_dec(&dpusm->active);
    printk("dpusm_put: User has returned a handle to \"%s\" (now %d users).\n", provider->name, atomic_read(&provider->refs));
    return DPUSM_OK;
}

void
dpusm_provider_write_lock(dpusm_t *dpusm) {
    write_lock(&dpusm->lock);
}

void
dpusm_provider_write_unlock(dpusm_t *dpusm) {
    write_unlock(&dpusm->lock);
}
