CC=gcc
CFLAGS=-c -O2 -Wall
LIBFLAGS=
EXECUTABLE=readlines-main
LIBRARY=libreadlines.so

all: readlines-main unsed

all-shared: readlines-main-shared unsed-shared

readlines-main: readlines.o
	$(CC) readlines-main.c readlines.o -o $(EXECUTABLE)

readlines-main-shared: libreadlines
	$(CC) readlines-main.c -L. -lreadlines -o $(EXECUTABLE)

readlines.o:
	$(CC) $(CFLAGS) readlines.c

libreadlines: readlines-shared.o
	$(CC) -shared readlines-shared.o -o $(LIBRARY)

readlines-shared.o:
	$(CC) -fPIC -c readlines.c -o readlines-shared.o

unsed: readlines.o
	$(CC) unsed.c readlines.o -lpcre -o unsed

unsed-shared: libreadlines
	$(CC) unsed.c -L. -lreadlines -lpcre -o unsed

clean:
	rm -f *.o $(EXECUTABLE) $(LIBRARY) unsed
