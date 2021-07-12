# Modified answer by Nikolai Popov
# https://stackoverflow.com/a/18137056
mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
current_dir := $(dir $(mkfile_path))

DPUSM = $(realpath $(current_dir)/..)
KBUILD_EXTRA_SYMBOLS = $(DPUSM)/Module.symvers

# Modified question by p0kR
# https://stackoverflow.com/q/42867683
TARGETS = all clean
SUBDIRS = providers users

obj-y += $(SUBDIRS)

.PHONY: $(TARGETS) $(SUBDIRS)

$(TARGETS): $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS) DPUSM=$(DPUSM) KBUILD_EXTRA_SYMBOLS=$(KBUILD_EXTRA_SYMBOLS) PARENT=$(current_dir)
