CC=gcc
CFLAGS=-g -Wall -lpthread -lrt -lm

JFLAGS = -g
JC = javac
.SUFFIXES: .java .class
.java.class:
	$(JC) $(JFLAGS) $*.java
CLASSES = client.java


all: server client

client: $(CLASSES:.java=.class)

server: server.o
	$(CC) server.o -o server $(CFLAGS)

server.o: server.c
	$(CC) -c server.c

clean:
	rm *.o client server *.class
