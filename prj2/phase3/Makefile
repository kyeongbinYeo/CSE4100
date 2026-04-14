# Makefile for myshell

CC = gcc
CFLAGS = -Wall -g
TARGET = myshell
OBJS = myshell.o csapp.o
LIBS = -lpthread
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

myshell.o: myshell.c myshell.h
	$(CC) $(CFLAGS) -c myshell.c

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c

clean:
	rm -f $(TARGET) *.o
