NEED_PROVIDER := $(PARENT)/need_provider

TARGET = example_dpusm_need_provider_user

obj-m += $(TARGET).o
$(TARGET)-objs := need_provider.o

ccflags-y=-std=gnu99 -Wno-declaration-after-statement -g3 -I$(NEED_PROVIDER) -I$(DPUSM)/include

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(NEED_PROVIDER) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(NEED_PROVIDER) clean
