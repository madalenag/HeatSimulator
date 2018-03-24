# Makefile
# Sistemas Operativos, DEI/IST/ULisboa 2017-18

CFLAGS= -g -Wall -pedantic -std=gnu99
CC=gcc

heatSim: main.o matrix2d.o
	$(CC) $(CFLAGS) -pthread -o heatSim main.o matrix2d.o

main.o: main.c matrix2d.h
	$(CC) $(CFLAGS) -c main.c

clean:
	rm -f *.o heatSim
