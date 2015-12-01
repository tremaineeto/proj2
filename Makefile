CC = g++
all: sender receiver
sender: sender.cpp
	$(CC) -w sender.cpp -o sender
receiver: receiver.cpp
	$(CC) -w receiver.cpp -o receiver
clean:
	rm sender receiver *_copy
