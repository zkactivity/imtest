extern "C" {
#include <event2/http.h>
#include <event2/http_struct.h>
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/dns.h>
#include <event2/thread.h>

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/queue.h>
#include <event.h>
}

#include <iostream>
using namespace std;


void RemoteReadCallback(evhttp_request * remote_req, void * arg) {
	event_base_loopexit((event_base *)arg, NULL);
}

int ReadHeaderDoneCallback(evhttp_request * remote_req, void *arg) {
	cout << "HTTP/1.1 " << evhttp_request_get_response_code(remote_req) << "  " <<
		evhttp_request_get_response_code_line(remote_req) << endl;
	evkeyvalq * headers = evhttp_request_get_input_headers(remote_req);
	evkeyval* header;
	TAILQ_FOREACH(header, headers, next)
	{
		cout << "< " << header->key << " : " << header->value << endl;
	}
	cout << "<" << endl;
}

void ReadChunkCallback(evhttp_request * remote_req, void * arg) {
	char buf[4096];
	evbuffer * evbuf = evhttp_request_get_input_buffer(remote_req);
	int n = 0;
	cout << "--------------> ReadChunkCallback" << endl;
	while((n = evbuffer_remove(evbuf, buf, 4096)) > 0) {
		cout << buf;
	}
}

void RemoteRequestErrorCallback(evhttp_request_error err, void * arg) {
	cout << "Remote connection closed." << endl;
	event_base_loopexit((event_base *)arg, NULL);
}

void RemoteConnectionCloseCallback(evhttp_connection * conn, void * arg) {
	cout << "Remote connection closed." << endl;
	event_base_loopexit((event_base *)arg, NULL);
}

int main(int argc, char * argv[]) {
	if (argc != 2) {
		cout << "usage: " << argv[0] << " [url]." << endl;
		return 1;
	}
	char * url = argv[1];
	evhttp_uri * uri = evhttp_uri_parse(url);
	if(!uri) {
		cout << "Parsing url failed." << endl;
		return 1;
	}

	event_base * base = event_base_new();
	if(!base) {
		cout << "Create event base failed." << endl;
		return 1;
	}

	evdns_base * dnsbase = evdns_base_new(base, 1);
	if(!dnsbase) {
		cout << "Create dns base failed." << endl;
	}
	assert(dnsbase);

	evhttp_request * request = evhttp_request_new(RemoteReadCallback, base);
	evhttp_request_set_header_cb(request, ReadHeaderDoneCallback);
	evhttp_request_set_chunked_cb(request, ReadChunkCallback);
	evhttp_request_set_error_cb(request, RemoteRequestErrorCallback);

	const char * host = evhttp_uri_get_host(uri);
	if(!host) {
		cout << "Parse host failed." << endl;
		return 1;
	}
	
	int port = evhttp_uri_get_port(uri);
	if(port < 0) {
		port = 80;
	}

	const char * request_url = url;
	const char * path = evhttp_uri_get_path(uri);
	if(path == NULL || strlen(path) == 0) {
		request_url = "/";
	}

	cout << "url: " << url << endl <<
			"host: " << host << endl <<
			"port: " << port << endl <<
			"path: " << path << endl <<
			"request_url: " << request_url << endl;

	evhttp_connection * conn = evhttp_connection_base_new(base, dnsbase, host, port);
	if(!conn) {
		cout << "Create evhttp connection failed." << endl;
		return 1;
	}

	evhttp_connection_set_closecb(conn, RemoteConnectionCloseCallback, base);

	evhttp_add_header(evhttp_request_get_output_headers(request), "Host", host);
	evhttp_make_request(conn, request, EVHTTP_REQ_GET, request_url);

	event_base_dispatch(base);

	return 0;
}





