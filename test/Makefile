all:server_test.cpp
	rm -f a.out
	g++ $^ -levent -lssl -lcrypto -pthread

clean:
	rm -f a.out
