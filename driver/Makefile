obj-m += rpifan.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

reinstall: all
	-sudo rmmod rpifan
	sudo cp rpifan.ko /lib/modules/$(shell uname -r)/kernel/drivers/misc/
	sudo depmod -a
	sudo modprobe rpifan