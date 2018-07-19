SOURCE_FILES=tls_server.cpp socket_kernel.cpp tls.cpp
server:$(SOURCE_FILES)
	g++ $^ -o $@ -lssl -lcrypto
clean:
	rm -f server
