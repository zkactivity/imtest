#include "tls.hpp"

bool tls::m_bOpensslInit = false;

void tls::init_sys_openssl() {
    SSL_load_error_strings();	
    OpenSSL_add_ssl_algorithms();
}

void tls::cleanup_sys_openssl() {
    EVP_cleanup();
}

void tls::release_openssl_resources() {
	if(m_pssl != nullptr) {
        SSL_free(m_pssl);
		m_pssl = nullptr;
	}
	if(m_pctx != nullptr) {
		SSL_CTX_free(m_pctx);
		m_pctx = nullptr;
	}
}

SSL_CTX *  tls::create_context() {
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = SSLv23_server_method();

    ctx = SSL_CTX_new(method);
    if (!ctx) {
		perror("Unable to create SSL context");
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
    }

    return ctx;
}

void tls::configure_context(const std::string & cert_file, const std::string & key_file) {
	SSL_CTX_set_ecdh_auto(m_pctx, 1);

	/* Set the cert and key */
	if(SSL_CTX_use_certificate_file(m_pctx, cert_file.c_str(), SSL_FILETYPE_PEM) <= 0) {
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
	}
	m_server_certification_file = cert_file;

	if(SSL_CTX_use_PrivateKey_file(m_pctx, key_file.c_str(), SSL_FILETYPE_PEM) <= 0) {
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
	}
	m_server_key_file = key_file;
}


void tls::set_socket_fd(socket_fd clifd) {
	if(m_pssl != nullptr) {
		SSL_free(m_pssl);
	}

	m_pssl = SSL_new(m_pctx);
	SSL_set_fd(m_pssl, clifd);

	//ready, waiting for accept
}

tls::tls() {
	m_pctx = nullptr;
	m_pssl = nullptr;
	m_listen_fd = -1;
}


tls::~tls() {
	cleanup_sys_openssl();
}

