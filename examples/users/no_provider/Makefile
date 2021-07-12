NO_PROVIDER := $(PARENT)/no_provider

TARGET = example_dpusm_no_provider_user

obj-m += $(TARGET).o
$(TARGET)-objs := no_provider.o

ccflags-y=-std=gnu99 -Wno-declaration-after-statement -g3 -I$(NO_PROVIDER) -I$(DPUSM)/include

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(NO_PROVIDER) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(NO_PROVIDER) clean
