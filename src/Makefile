CFLAGS=-Wall -Wextra -std=c99 -pedantic -ggdb
DEPS=linebuffer.h

all: grep

grep: grep.o linebuffer.o $(DEPS)
	cc $^ $(CFLAGS) -o grep

%.o: %.c Makefile $(DEPS)
	cc $(CFLAGS) -c $< -o $@

run:
	$(MAKE) grep
	./grep file.txt better

debug:
	@#gdb --batch -x debug.gdb ./grep -nx
	gdb -x debug.gdb ./grep

clean:
	rm *.o grep

.PHONY: debug clean run mem
