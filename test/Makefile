
main:https-server.cpp
	g++ $^ -o main -lssl -lcrypto -lpthread

all:server_test.cpp
	rm -f a.out
	g++ $^ -levent -lssl -lcrypto -pthread

clean:
	rm -f a.out main
