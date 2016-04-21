
all: cutil
	make -C fuse
	make -C fuse_lowlevel
	make -C print_tags

cutil:
	make -C cutil

clean:
	make -C fuse clean
	make -C fuse_lowlevel clean
	make -C print_tags clean
	make -C cutil clean
