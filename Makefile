all: my_attempt

my_attempt: my_attempt.c
	gcc -o my_attempt my_attempt.c

clean:
	-rm my_attempt
	-rm Makefile~
