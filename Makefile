# Patrick Lebold & Patrick Polley

all: virtualmemory

virtualmemory: virtualmemory.o
	gcc virtualmemory.o -o virtualmemory

virtualmemory.o: virtualmemory.c
	gcc -c virtualmemory.c virtualmemory.h

clean:
	rm -f virtualmemory *.o *~
