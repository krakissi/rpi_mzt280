all: mzt280

mzt280: mzt280.c 8x16.h
	gcc -o mzt280 -l rt mzt280.c bcm2835.c
