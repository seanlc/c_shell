CC= gcc
CFLAGS= -std=c99 -ggdb -Wall

shell: shell.o
	$(CC) -o shell $(CFLAGS) shell.c

clean:
	rm -f shell.o shell
