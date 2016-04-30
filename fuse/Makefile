TARGET=tagfs
SRC=$(wildcard *.c) $(wildcard ../cutil/*.c)

CFLAGS=-Wall -std=gnu99 -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE
CFLAGS+=$(shell pkg-config --cflags fuse3)
CFLAGS+=-I ../include -iquote.

ifdef DEBUG
	CFLAGS+=-ggdb -O0 -DDEBUG
else
	CFLAGS+=-g -O2 -march=native
endif

LDFLAGS=$(shell pkg-config --libs fuse3)

OBJ=$(SRC:.c=.o) 
DEP=$(SRC:.c=.d) 

all: $(TARGET)

-include $(DEP)

$(TARGET): $(OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

%.o: %.c
	@$(CC) -MM $(CFLAGS) $*.c > $*.d
	$(CC) -c $(CFLAGS) $*.c -o $*.o

mnt: 
	mkdir -p mnt/

clean: 
	$(RM) *.log *.o *.d

start: mnt tagfs
	./tagfs images mnt -f -d -s

restart: stop kill start

debug: tagfs stop
	gdb --args ./tagfs images/ mnt -f -d -s

val3: tagfs stop
	valgrind ./tagfs images mnt -f -d -s

stop:
	fusermount -u mnt || true

kill:
	kill -9 $(shell pidof tagfs) || true

all_in_one:
	../scripts/all_in_one

