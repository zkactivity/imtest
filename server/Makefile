SOURCE_FILES=tls_server.cpp socket_kernel.cpp tls.cpp

main:main.cpp connection.cpp
	g++ $^ -o main -lssl -lcrypto -lpthread

https-server:https-server.cpp
	g++ $^ -o https-server -levent -lssl -lcrypto

http-server:http-server.cpp
	g++ $^ -o http-server -levent
	@./http-server public

server:$(SOURCE_FILES)
	g++ $^ -o $@ -lssl -lcrypto

clean:
	rm -f server http-server https-server tcpserver main
