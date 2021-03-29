# Tomasz Woszczynski 307690

CC = gcc
STD = -std=gnu17
CFLAGS = -Wall -Wextra -Werror
TARGET = traceroute

$(TARGET): main.o send_packets.o receive_packets.o helpers.o
	$(CC) $(STD) $(CFLAGS) main.o send_packets.o receive_packets.o helpers.o -o $(TARGET)

main.o: main.c
	$(CC) -c $(STD) $(CFLAGS) main.c -o main.o

send_packets.o: send_packets.c send_packets.h
	$(CC) -c $(STD) $(CFLAGS) send_packets.c -o send_packets.o

receive_packets.o: receive_packets.c receive_packets.h
	$(CC) -c $(STD) $(CFLAGS) receive_packets.c -o receive_packets.o

helpers.o: helpers.c helpers.h
	$(CC) -c $(STD) $(CFLAGS) helpers.c -o helpers.o

test:
	sudo ./traceroute 156.17.4.1

clean:
	rm -rf *.o

distclean:
	rm -rf *.o $(TARGET)