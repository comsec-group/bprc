# SPDX-License-Identifier: GPL-3.0-only
obj-m	+= ap.o
ap-objs	:= main.o

PWD=$(shell pwd)

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

.PHONY: install
install: all
	cp ap.ko /tmp/ && sudo insmod /tmp/ap.ko

.PHONY: remove
remove:
	sudo rmmod ap
