TARGET=tagfs
SRC=$(wildcard *.c) $(wildcard cutil/*.c)
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

clean: 
	$(RM) $(OBJ) $(DEP) *.log *.o
	make -C cutil clean
