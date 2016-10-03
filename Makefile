upciedev-objs := upciedev_drv.o pciedev_ufn.o pciedev_probe_exp.o \
	pciedev_remove_exp.o pciedev_rw_exp.o pciedev_ioctl_exp.o criticalregionlock.o
obj-m := upciedev.o 

KVERSION = $(shell uname -r)
all:
	make -C /lib/modules/$(KVERSION)/build V=1 M=$(PWD) modules
	test -d /lib/modules/$(KVERSION)/upciedev || sudo mkdir -p /lib/modules/$(KVERSION)/upciedev
	sudo cp -f $(PWD)/Module.symvers /lib/modules/$(KVERSION)/upciedev/Upciedev.symvers
	cp -f $(PWD)/Module.symvers $(PWD)/Upciedev.symvers
clean:
	test ! -d /lib/modules/$(KVERSION) || make -C /lib/modules/$(KVERSION)/build V=1 M=$(PWD) clean

