CFLAGS=-Wall -Wextra -std=c99 -pedantic -ggdb

all: grep

grep: grep.o
	cc $^ $(CFLAGS) -o grep

%.o: %.c Makefile
	cc $(CFLAGS) -c $< -o $@

run:
	$(MAKE) grep
	./grep file.txt better

mem:
	$(MAKE) grep
	valgrind ./grep file.txt better

debug:
	@#gdb --batch -x debug.gdb ./grep -nx
	gdb -x debug.gdb ./grep

clean:
	rm *.o grep

.PHONY: debug clean run mem
