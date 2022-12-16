#include <dpusm/alloc.h>
#include <dpusm/provider.h>

/* masks for bad function groups */
static const int DPUSM_PROVIDER_BAD_GROUP_STRUCT   = (1 << 0);
static const int DPUSM_PROVIDER_BAD_GROUP_REQUIRED = (1 << 1);
static const int DPUSM_PROVIDER_BAD_GROUP_RAID_GEN = (1 << 2);
static const int DPUSM_PROVIDER_BAD_GROUP_RAID_REC = (1 << 3);
static const int DPUSM_PROVIDER_BAD_GROUP_FILE     = (1 << 4);
static const int DPUSM_PROVIDER_BAD_GROUP_DISK     = (1 << 5);

static const char *DPUSM_PROVIDER_BAD_GROUP_STRINGS[] = {
    "STRUCT",
    "REQUIRED",
    "RAID_GEN",
    "RAID_REC",
    "FILE",
    "DISK",
};

/* check provider sanity when loading */
static int
dpusm_provider_sane_at_load(const dpusm_pf_t *funcs)
{
    if (!funcs) {
        return DPUSM_PROVIDER_BAD_GROUP_STRUCT;
    }

    /*
     * Other functions may be defined as needed. They are
     * not grouped together like these functions.
     */

    const int required = (
        !!funcs->algorithms +
        !!funcs->alloc +
        !!funcs->alloc_ref +
        !!funcs->get_size +
        !!funcs->free +
        !!funcs->copy.from.generic +
        !!funcs->copy.to.generic);

    const int raid_gen = (
        !!funcs->raid.can_compute +
        !!funcs->raid.alloc +
        !!funcs->raid.set_column +
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
        (!(required == 7)?DPUSM_PROVIDER_BAD_GROUP_REQUIRED:0) |
        (!((raid_gen == 0) || (raid_gen == 5))?DPUSM_PROVIDER_BAD_GROUP_RAID_GEN:0) |
        (!((raid_rec == 0) || ((raid_gen == 5) && (raid_rec == 3)))?DPUSM_PROVIDER_BAD_GROUP_RAID_REC:0) |
        (!((file == 0) || (file == 3))?DPUSM_PROVIDER_BAD_GROUP_FILE:0) |
        (!((disk == 0) || (disk == 5))?DPUSM_PROVIDER_BAD_GROUP_DISK:0)
    );

    return rc;
}

/* simple linear search */
/* caller locks */
static dpusm_ph_t **
find_provider(dpusm_t *dpusm, const char *name) {
    const size_t name_len = strlen(name);

    struct list_head *it = NULL;
    list_for_each(it, &dpusm->providers) {
        dpusm_ph_t *dpusmph = list_entry(it, dpusm_ph_t, list);
        const char *p_name = dpusmph->name;
        const size_t p_name_len = strlen(p_name);
        if (name_len == p_name_len) {
            if (memcmp(name, p_name, p_name_len) == 0) {
                return &dpusmph->self;
            }
        }
    }

    return NULL;
}

static void
dpusmph_destroy(dpusm_ph_t *dpusmph)
{
    dpusm_mem_free(dpusmph, sizeof(*dpusmph));
}

static void print_supported(const char *name, const char *func)
{
    printk("Provider %s supports %s\n", name, func);
}

static dpusm_ph_t *
dpusmph_init(const char *name, const dpusm_pf_t *funcs)
{
    dpusm_ph_t *dpusmph = dpusm_mem_alloc(sizeof(dpusm_ph_t));
    if (dpusmph) {
        /* fill in capabilities bitmasks */
        if (funcs->copy.from.ptr) {
            dpusmph->capabilities.optional |= DPUSM_OPTIONAL_COPY_FROM_PTR;
            print_supported(name, enum2str(DPUSM_OPTIONAL_STR, DPUSM_OPTIONAL_COPY_FROM_PTR));
        }

        if (funcs->copy.to.ptr) {
            dpusmph->capabilities.optional |= DPUSM_OPTIONAL_COPY_TO_PTR;
            print_supported(name, enum2str(DPUSM_OPTIONAL_STR, DPUSM_OPTIONAL_COPY_TO_PTR));
        }

        if (funcs->copy.from.scatterlist) {
            dpusmph->capabilities.optional |= DPUSM_OPTIONAL_COPY_FROM_SCATTERLIST;
            print_supported(name, enum2str(DPUSM_OPTIONAL_STR, DPUSM_OPTIONAL_COPY_FROM_SCATTERLIST));
        }

        if (funcs->copy.to.scatterlist) {
            dpusmph->capabilities.optional |= DPUSM_OPTIONAL_COPY_TO_SCATTERLIST;
            print_supported(name, enum2str(DPUSM_OPTIONAL_STR, DPUSM_OPTIONAL_COPY_TO_SCATTERLIST));
        }

        if (funcs->mem_stats) {
            dpusmph->capabilities.optional |= DPUSM_OPTIONAL_MEM_STATS;
            print_supported(name, enum2str(DPUSM_OPTIONAL_STR, DPUSM_OPTIONAL_MEM_STATS));
        }

        if (funcs->zero_fill) {
            dpusmph->capabilities.optional |= DPUSM_OPTIONAL_ZERO_FILL;
            print_supported(name, enum2str(DPUSM_OPTIONAL_STR, DPUSM_OPTIONAL_ZERO_FILL));
        }

        if (funcs->all_zeros) {
            dpusmph->capabilities.optional |= DPUSM_OPTIONAL_ALL_ZEROS;
            print_supported(name, enum2str(DPUSM_OPTIONAL_STR, DPUSM_OPTIONAL_ALL_ZEROS));
        }

        if (funcs->algorithms(&dpusmph->capabilities.compress,
                              &dpusmph->capabilities.decompress,
                              &dpusmph->capabilities.checksum,
                              &dpusmph->capabilities.checksum_byteorder,
                              &dpusmph->capabilities.raid) != DPUSM_OK) {
            dpusmph_destroy(dpusmph);
            return NULL;
        }

        if (!funcs->compress) {
            dpusmph->capabilities.compress = 0;
        }

        for(size_t i = 1 << 0; i < DPUSM_COMPRESS_MAX; i <<= 1) {
            if (dpusmph->capabilities.compress & i) {
                print_supported(name, enum2str(DPUSM_COMPRESS_STR, i));
            }
        }

        if (!funcs->decompress) {
            dpusmph->capabilities.decompress = 0;
        }

        for(size_t i = 1 << 0; i < DPUSM_COMPRESS_MAX; i <<= 1) {
            if (dpusmph->capabilities.decompress & i) {
                print_supported(name, enum2str(DPUSM_DECOMPRESS_STR, i));
            }
        }

        if (!funcs->checksum) {
            dpusmph->capabilities.checksum = 0;
            dpusmph->capabilities.checksum_byteorder = 0;
        }

        for(size_t i = 1 << 0; i < DPUSM_CHECKSUM_MAX; i <<= 1) {
            if (dpusmph->capabilities.checksum & i) {
                print_supported(name, enum2str(DPUSM_CHECKSUM_STR, i));
            }
        }

        for(size_t i = 1 << 0; i < DPUSM_BYTEORDER_MAX; i <<= 1) {
            if (dpusmph->capabilities.checksum_byteorder & i) {
                print_supported(name, enum2str(DPUSM_CHECKSUM_BYTEORDER_STR, i));
            }
        }

        /* already checked for sanity */
        if (!funcs->raid.gen) {
            dpusmph->capabilities.raid = 0;
        }

        for(size_t i = 1 << 0; i < DPUSM_RAID_MAX; i <<= 1) {
            if (dpusmph->capabilities.raid & i) {
                print_supported(name, enum2str(DPUSM_RAID_STR, i));
            }
        }

        /* already checked for sanity */
        if (funcs->file.open) {
            dpusmph->capabilities.io |= DPUSM_IO_FILE;
            print_supported(name, enum2str(DPUSM_IO_STR, DPUSM_IO_FILE));
        }
        else {
            dpusmph->capabilities.io &= ~DPUSM_IO_FILE;
        }

        /* already checked for sanity */
        if (funcs->disk.open) {
            dpusmph->capabilities.io |= DPUSM_IO_DISK;
            print_supported(name, enum2str(DPUSM_IO_STR, DPUSM_IO_DISK));
        }
        else {
            dpusmph->capabilities.io &= ~DPUSM_IO_DISK;
        }

        dpusmph->name = name;
        dpusmph->funcs = funcs;
        dpusmph->self = dpusmph;
        atomic_set(&dpusmph->refs, 0);
    }

    return dpusmph;
}

/* add a new provider */
int
dpusm_provider_register(dpusm_t *dpusm, const char *name, const dpusm_pf_t *funcs) {
    const int rc = dpusm_provider_sane_at_load(funcs);
    if (rc != DPUSM_OK) {
        static const size_t max =
            sizeof(DPUSM_PROVIDER_BAD_GROUP_STRINGS) / sizeof(DPUSM_PROVIDER_BAD_GROUP_STRINGS[0]);

        size_t size = 1; /* NULL terminator */
        for(size_t i = 0; i < max; i++) {
            if (rc & (1 << i)) {
                size += strlen(DPUSM_PROVIDER_BAD_GROUP_STRINGS[i]) + 2; /* str + ", " */
            }
        }

        char *buf = dpusm_mem_alloc(size);
        ssize_t offset = 0;
        for(size_t i = 0; i < max; i++) {
            if (rc & (1 << i)) {
                offset += snprintf(buf + offset, size - offset, "%s, ", DPUSM_PROVIDER_BAD_GROUP_STRINGS[i]);
            }
        }
        buf[offset - 2] = '\0'; /* get rid of trailing ", " */

        printk("%s: DPUSM Provider \"%s\" does not provide "
               "a valid set of functions. Bad function groups: %s\n",
               __func__, name, buf);

        dpusm_mem_free(buf, size);

        return -EINVAL;
    }

    dpusm_provider_write_lock(dpusm);

    dpusm_ph_t **found = find_provider(dpusm, name);
    if (found) {
        printk("%s: DPUSM Provider with the name \"%s\" (%p) already exists. %zu providers registered.\n",
               __func__, name, *found, dpusm->count);
        dpusm_provider_write_unlock(dpusm);
        return -EEXIST;
    }

    dpusm_ph_t *provider = dpusmph_init(name, funcs);
    if (!provider) {
        dpusm_provider_write_unlock(dpusm);
        return -ECANCELED;
    }

    list_add(&provider->list, &dpusm->providers);
    dpusm->count++;
    printk("%s: DPUSM Provider \"%s\" (%p) added. Now %zu providers registered.\n",
           __func__, name, provider, dpusm->count);

    dpusm_provider_write_unlock(dpusm);

    return 0;
}

/* remove provider from list */
/* can't prevent provider module from unloading */
/* locking is done by caller */
int
dpusm_provider_unregister_handle(dpusm_t *dpusm, dpusm_ph_t **provider) {
    if (!provider || !*provider) {
        printk("%s: Bad provider handle.\n", __func__);
        return -EINVAL;
    }

    int rc = 0;
    const int refs = atomic_read(&(*provider)->refs);
    if (refs) {
        printk("%s: Unregistering provider \"%s\" with %d references remaining.\n",
               __func__, (*provider)->name, refs);
        rc = -EBUSY;
    }

    list_del(&(*provider)->list);
    atomic_sub(refs, &dpusm->active); /* remove this provider's references from the global active count */

    dpusmph_destroy(*provider);
    dpusm->count--;

    *provider = NULL;

    return rc;
}

int
dpusm_provider_unregister(dpusm_t *dpusm, const char *name) {
    dpusm_provider_write_lock(dpusm);

    dpusm_ph_t **provider = find_provider(dpusm, name);
    if (!provider) {
        printk("%s: Could not find provider with name \"%s\"\n", __func__, name);
        dpusm_provider_write_unlock(dpusm);
        return DPUSM_ERROR;
    }

    void *addr = *provider;
    const int rc = dpusm_provider_unregister_handle(dpusm, provider);
    printk("%s: Unregistered \"%s\" (%p): %d\n", __func__, name, addr, rc);

    dpusm_provider_write_unlock(dpusm);
    return rc;
}

/*
 * called by the user, but is located here
 * to have access to the global list
 */

/* get a provider by name */
dpusm_ph_t **
dpusm_provider_get(dpusm_t *dpusm, const char *name) {
    read_lock(&dpusm->lock);
    dpusm_ph_t **provider = find_provider(dpusm, name);
    if (provider) {
        atomic_inc(&(*provider)->refs);
        atomic_inc(&dpusm->active);
        printk("%s: User has been given a handle to \"%s\" (now %d users).\n",
               __func__, (*provider)->name, atomic_read(&(*provider)->refs));
    }
    else {
        printk("%s: Error: Did not find provider \"%s\"\n",
               __func__, name);
    }
    read_unlock(&dpusm->lock);
    return provider;
}

/* declare that the provider is no longer being used by the caller */
int
dpusm_provider_put(dpusm_t *dpusm, void *handle) {
    dpusm_ph_t **provider = (dpusm_ph_t **) handle;
    if (!provider || !*provider) {
        printk("%s: Error: Invalid handle\n", __func__);
        return DPUSM_ERROR;
    }

    if (!atomic_read(&(*provider)->refs)) {
        printk("%s Error: Cannot decrement provider \"%s\" user count already at 0.\n",
               __func__, (*provider)->name);
        return DPUSM_ERROR;
    }

    atomic_dec(&(*provider)->refs);
    atomic_dec(&dpusm->active);
    printk("%s: User has returned a handle to \"%s\" (now %d users).\n",
           __func__, (*provider)->name, atomic_read(&(*provider)->refs));
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

void dpusm_provider_invalidate(dpusm_t *dpusm, const char *name) {
    dpusm_provider_write_lock(dpusm);
    dpusm_ph_t **provider = find_provider(dpusm, name);
    if (provider && *provider) {
        (*provider)->funcs = NULL;
        memset(&(*provider)->capabilities, 0, sizeof((*provider)->capabilities));
        printk("%s: Provider \"%s\" has been invalidated with %d users active.\n",
               __func__, name, atomic_read(&(*provider)->refs));
    }
    else {
        printk("%s: Error: Did not find provider \"%s\"\n",
               __func__, name);
    }
    dpusm_provider_write_unlock(dpusm);
}
