# Patrick Lebold & Patrick Polley

all: vm

vm: vm.o
	gcc vm.o -o vm

vm.o: virtualmemory.c
	gcc -c virtualmemory.c virtualmemory.h

clean:
	rm -f vm  *.o *~
