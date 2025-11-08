obj-m += rpifan.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(CURDIR) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(CURDIR) clean

reinstall: clean all
	-sudo modprobe -r rpifan
	sudo install -m 644 rpifan.ko /lib/modules/$(shell uname -r)/kernel/drivers/misc/
	sudo depmod -a
	sudo modprobe rpifan

.PHONY: all clean reinstall
