TARGET=poll_files
SRC=$(wildcard *.c) $(wildcard cutil/*.c)

CFLAGS=-Wall -std=gnu99 -D_GNU_SOURCE
CFLAGS+=-iquote.
CFLAGS+=-I../include

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

clean:
	$(RM) $(OBJ) $(DEP) *.log *.o
