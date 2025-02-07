# SPDX-License-Identifier: GPL-3.0-only
mod-name := secret
obj-m += $(mod-name).o

PWD := $(shell pwd)
KDIR := /lib/modules/$(shell uname -r)/build

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean

.PHONY:
install: all
	cp $(mod-name).ko /tmp/ && sudo insmod /tmp/$(mod-name).ko

.PHONY: remove
remove:
	sudo rmmod $(mod-name)
