#include <string>
#include <openssl/ssl.h>
#include <openssl/err.h>

typedef int socket_fd;

class tls {
private:
	SSL_CTX * m_pctx;
	
	SSL * m_pssl;
	
	std::string m_server_certification_file;
	
	std::string m_server_key_file;

	static bool m_bOpensslInit;

	socket_fd m_listen_fd;
	
	static void init_sys_openssl();

	static void cleanup_sys_openssl();
	
	void release_openssl_resources();

	SSL_CTX * create_context();

	void configure_context(const std::string & cert_file, const std::string & key_file);

	
public:
	tls();
	
	~tls();

	void set_socket_fd(socket_fd);
	
};


