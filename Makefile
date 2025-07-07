# Variables
CC = gcc
CFLAGS = -Wall -Wextra -O2 -c -I.

# Regla principal: compilar el programa
programa: main.o dashboard.o
	$(CC) -o programa main.o dashboard.o

# Compilar main.o
main.o: main.c dashboard.h
	$(CC) $(CFLAGS) main.c

# Compilar monitor.o
dashboard.o: dashboard.c dashboard.h
	$(CC) $(CFLAGS) dashboard.c


.PHONY: clean
clean:
	rm -f programa *.o
