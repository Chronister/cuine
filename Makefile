COMPILE_FLAGS= -std=c99 -Wall -g
LINK_FLAGS=
LIBS= 
.PHONY: all clean dir

all: dir build/metachr

build/metachr: dir code/scanner.c 
	cc $(COMPILE_FLAGS) -c code/scanner.c -o $@

dir:
	mkdir -p build/

clean: 
	rm -r build



