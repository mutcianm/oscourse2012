CC=clang++
CFLAGS=-g -Wall
LDFLAGS=-lutil
EXECUTABLE=ssh

all: minissh

minissh:
	$(CC) $(CFLAGS) $(LDFLAGS) minissh.cpp -o $(EXECUTABLE)

clean:
	rm $(EXECUTABLE)
