CC = gcc
CFLAGS = -c -I.

programa: main.o dashboard.o
	$(CC) -o programa main.o dashboard.o -lcurl

main.o: main.c dashboard.h
	$(CC) $(CFLAGS) main.c

dashboard.o: dashboard.c dashboard.h
	$(CC) $(CFLAGS) dashboard.c

.PHONY: clean
clean:
	rm -f programa *.o
