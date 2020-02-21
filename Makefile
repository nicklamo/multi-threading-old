CC = gcc
CFLAGS = -Wall -Wextra
LFLAGS = -pthread

multi-lookup: util.o multi-lookup.o
	$(CC) $(CFLAGS) $(LFLAGS) $^ -o $@

util.o: util.c util.h
	$(CC) $(CFLAGS) -c util.c

multi-lookup.o: multi-lookup.c multi-lookup.h
	$(CC) $(CFLAGS) -c multi-lookup.c

clean:
	rm *.o
	rm multi-lookup
