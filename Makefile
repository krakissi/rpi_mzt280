CC=gcc -s -Wunused

all: mzt280

profile : CC=gcc -g -pg
profile: all

# Remove build output
clean:
	rm -f mzt280 *.o

# Copy the binary to /usr/bin
install: all
	cp mzt280 /usr/bin/

# Kill running software, patch, and restart
update: all
	@sudo killall mzt280 || echo "No process, no problem."
	@echo "Copying to /usr/bin/"
	@sudo cp mzt280 /usr/bin/
	@echo "Starting display process"
	@sudo mzt280 &

# Display program
mzt280: mzt280.c bcm2835.o lcd.o framebuffer.h
	$(CC) -o mzt280 -l rt mzt280.c bcm2835.o lcd.o

# LCD functions
lcd.o: lcd.h lcd.c
	$(CC) -c lcd.c -o lcd.o

# Broadcom interface object
bcm2835.o: bcm2835.c bcm2835.h
	$(CC) -c bcm2835.c -o bcm2835.o
