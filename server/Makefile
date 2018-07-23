SOURCE_FILES=tls_server.cpp socket_kernel.cpp tls.cpp
http-server:http-server.cpp
	g++ $^ -o http-server -levent
	@./http-server public

server:$(SOURCE_FILES)
	g++ $^ -o $@ -lssl -lcrypto

clean:
	rm -f server http-server
