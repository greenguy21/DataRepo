# Makefile for lab 6

CC=gcc

CFLAGS= -lkvm

all: arena arenas arenar fork 

ours1: 
	$(CC) $(CFLAGS) src/425Boyz.c -o 425boyz

ours2: 
	$(CC) $(CFLAGS) src/DegenerateDuo.c -o DegenerateDuo


arena: 
	$(CC) $(CFLAGS) src/arena.c -o arena

arenas: 
	$(CC) $(CFLAGS) src/arena_s.c -o arena_s

arenar: 
	$(CC) $(CFLAGS) src/arena_r.c -o arena_r

fork: 
	$(CC) $(CFLAGS) src/fork.c -o fork

clean:
	rm -f arena arena_s arena_r fork 
