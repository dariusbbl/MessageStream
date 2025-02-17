CC=g++
CFLAGS =-std=c++17 -Wall -Wextra -g
LIBS=helpers.h 

PORT = 12345
IP_SERVER = 127.0.0.1

all: server subscriber

server: server.cpp
	${CC} ${CFLAGS} server.cpp ${LIBS} -o server


subscriber: subscriber.cpp
	${CC} ${CFLAGS} subscriber.cpp ${LIBS} -o subscriber

.PHONY: clean run_server run_subscriber

run_server:
	./server ${PORT}


run_subscriber:
	./subscriber ${IP_SERVER} ${PORT}

clean:
	rm -f server subscriber
