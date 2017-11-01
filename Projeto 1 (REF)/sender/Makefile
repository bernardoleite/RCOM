# Makefile for Generator Program

COMPILER_TYPE = -std = gnu99

CC = gcc

PROG = main
SRCS = data_link.c application.c

CFLAGS = 

$(PROG) : $(SRCS) utilities.h data_link.h
	$(CC) $(CFLAGS) $(SRCS) -o $(PROG)

clean:
	rm -f main
	rm -f *.d
	rm -f *.o
