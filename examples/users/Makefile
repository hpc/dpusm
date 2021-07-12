# Modified answer by p0kR
# https://stackoverflow.com/q/42867683
TARGETS = all clean
SUBDIRS = no_provider/ need_provider/

obj-y += $(SUBDIRS)

.PHONY: $(TARGETS) $(SUBDIRS)

$(TARGETS): $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS) DPUSM=$(DPUSM) KBUILD_EXTRA_SYMBOLS=$(KBUILD_EXTRA_SYMBOLS) PARENT=$(PARENT)/users
