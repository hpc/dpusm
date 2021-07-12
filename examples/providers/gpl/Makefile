PROVIDER = $(PARENT)/gpl

TARGET = example_gpl_dpusm_provider
obj-m += $(TARGET).o
$(TARGET)-objs := provider.o ../common.o

ccflags-y=-std=gnu99 -Wno-declaration-after-statement -g3 -I$(PROVIDER) -I$(PROVIDER)/.. -I$(DPUSM)/include

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PROVIDER) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PROVIDER) clean
