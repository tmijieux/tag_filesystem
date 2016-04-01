TARGET=tagfs
SRC=$(wildcard *.c) $(wildcard cutil/*.c)
DEBUG=1
CFLAGS=-Wall -std=gnu99 -I.. -D_FILE_OFFSET_BITS=64 \
	-Wno-unused-label -Wno-unused-function 

ifdef DEBUG
	CFLAGS+=-ggdb -O0 -DDEBUG
else
	CFLAGS+=-g -O2 -march=native
endif

LDFLAGS= -L. -lfuse

OBJ=$(SRC:.c=.o) 
DEP=$(SRC:.c=.d) 

all: $(TARGET)

-include $(DEP)

$(TARGET): $(OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

%.o: %.c
	gcc -c $(CFLAGS) $*.c -o $*.o
	gcc -MM $(CFLAGS) $*.c > $*.d

mnt: 
	mkdir -p mnt/

clean: 
	$(RM) $(OBJ) $(DEP) *.log *.o
	make -C cutil clean

start: tagfs
	./tagfs . mnt -f -d -s

restart: mnt stop kill start

debug: mnt tagfs stop
	gdb --args ./tagfs . mnt -f -d -s

stop:
	fusermount -u mnt || true

kill:
	kill -9 $(shell pidof tagfs) || true
