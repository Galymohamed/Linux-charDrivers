# Makefile for the multi_kbuf kernel module

obj-m := multi_kbuf.o

# Cross-compile settings (override from command line if needed)
ARCH          ?= arm
CROSS_COMPILE ?= arm-linux-gnueabihf-
KERNEL_DIR    ?= /path/to/kernel/source

PWD := $(shell pwd)

.PHONY: all clean help all_host clean_host help_host

# --- Cross-compile against an external kernel tree ---
all:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_DIR) M=$(PWD) clean

help:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_DIR) M=$(PWD) help

# --- Build against the running host kernel ---
all_host:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean_host:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

help_host:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) help