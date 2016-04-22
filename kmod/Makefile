ccflags-y= -Wno-unused-function
obj-m += tfs.o
tfs-objs = inode.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
