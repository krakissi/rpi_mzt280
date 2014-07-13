all: mzt280

# Kill running software, patch, and restart
update: all
	sudo killall mzt280
	sudo cp mzt280 /usr/bin/
	sudo mzt280 &

# Display program
mzt280: mzt280.c 8x16.h bcm2835.o
	gcc -o mzt280 -l rt mzt280.c bcm2835.o

# Broadcom interface object
bcm2835.o: bcm2835.c bcm2835.h
	gcc -c bcm2835.c -o bcm2835.o
