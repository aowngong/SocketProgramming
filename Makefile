# ***
# *** You can modify this file. You do not (should not) submit this file
# ***

WARNING = -Wall -Wshadow --pedantic
ERROR = -Wvla -Werror
GCC = gcc -std=c99 -g $(WARNING) $(ERROR) 
VAL = valgrind --tool=memcheck --log-file=vallog --leak-check=full --verbose
TESTFALGS = -D__SHELL_ARRAY_H__ -D__SHELL_LIST_H__

SRCS = pa3.c packing.c
OBJS = $(SRCS:%.c=%.o)

sort: $(OBJS) 
	$(GCC) $(TESTFALGS) $(OBJS) -o pa3

.c.o: 
	$(GCC) $(TESTFALGS) -c $*.c 

testall: test1 test2

test1: pa3
	./pa3 examples/3.po 3.pr 3.dim 3.pck
	diff 3.pr examples/3.pr
	diff 3.dim examples/3.dim
	diff 3.pck examples/3.pck

test2: pa3
	./pa3 examples/8.po 8.pr 8.dim 8.pck
	diff 8.pr examples/8.pr
	diff 8.dim examples/8.dim
	diff 8.pck examples/8.pck

memory: pa3
	$(VAL) 	./pa3 examples/8.po 8.pr 8.dim 8.pck



clean: # remove all machine generated files
	rm -f sort *.o output?



