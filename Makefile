
all: cutil
	make -C fuse
	make -C fuse_lowlevel
	make -C fuse_sqlite3
	make -C print_tags
	make -C ls_tags 
	make -C poll_files
	make -C tests

cutil:
	make -C cutil

clean:
	make -C fuse clean
	make -C fuse_lowlevel clean
	make -C fuse_sqlite3 clean
	make -C print_tags clean
	make -C ls_tags  clean
	make -C poll_files clean
	make -C tests clean
	make -C cutil clean
