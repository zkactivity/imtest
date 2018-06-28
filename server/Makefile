LIBS=/usr/local/lib/libevent.a

server:server.cpp $(LIBS)
	g++ $^ -o $@ -levent
clean:
	rm -f server
