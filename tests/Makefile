CFLAGS=-Wall -std=gnu99 -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE \
    -Wno-unused-label -Wno-unused-function -iquote.

ifdef DEBUG
    CFLAGS+=-ggdb -O0 -DDEBUG
else
    CFLAGS+=-g -O2 -march=native
endif
LDFLAGS= -L. -lfuse

SRC=$(wildcard *.c)
PROGS=$(SRC:.c=) 

all: $(PROGS)

