CPPFLAGS=-D_FILE_OFFSET_BITS=64
CFLAGS=-Wno-unused-label -Wno-unused-function

PROGRAMS = hello grepfs

all: $(PROGRAMS)

%: %.c
	gcc $(CPPFLAGS) $(CFLAGS) -Wall $< -o $@ -lfuse

clean:
	$(RM) $(PROGRAMS) *.log
