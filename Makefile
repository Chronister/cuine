COMPILE_FLAGS= -std=c99 -Wall -g
WARNINGS= -Wno-unused-variable -Wno-unused-label -Wno-missing-braces
LINK_FLAGS=
LIBS= 
.PHONY: all clean dir

all: dir build/metachr

build/metachr: dir code/scanner.c 
	cc $(COMPILE_FLAGS) $(WARNINGS) code/main.c -o $@

dir:
	mkdir -p build/

clean: 
	rm -r build



