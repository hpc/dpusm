# Modified answer by Nikolai Popov
# https://stackoverflow.com/a/18137056
mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
current_dir := $(dir $(mkfile_path))

DPUSM = $(current_dir)

TARGET = dpusm

obj-m += $(TARGET).o
$(TARGET)-objs := src/dpusm.o src/provider.o src/user.o src/alloc.o src/common.o

ccflags-y=-std=gnu99 -Wno-declaration-after-statement -O3 -g0 -I$(DPUSM)/include -DDEBUG=1 -D_KERNEL=1 -DDPUSM_TRACK_ALLOCS=0

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(DPUSM) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(DPUSM) clean
