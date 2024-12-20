#ifndef _PGS_SSL_H
#define _PGS_SSL_H

#include "config.h"

#ifdef USE_MBEDTLS
#include <mbedtls/ssl.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#else
#include <event2/bufferevent_ssl.h>
#endif

struct pgs_ssl_ctx_s;

#ifdef USE_MBEDTLS
typedef struct pgs_ssl_ctx_s {
	mbedtls_ssl_config conf;
	mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context ctr_drbg;
	mbedtls_x509_crt cacert;
} pgs_ssl_ctx_t;

typedef struct {
	mbedtls_ssl_context *ssl;
	void *cb_ctx;
} pgs_bev_ctx_t;

#else
typedef struct pgs_ssl_ctx_s pgs_ssl_ctx_t;
#endif

pgs_ssl_ctx_t *pgs_ssl_ctx_new(pgs_config_t *config);
void pgs_ssl_ctx_free(pgs_ssl_ctx_t *ctx);

int pgs_session_outbound_ssl_bev_init(struct bufferevent **bev, int fd,
				      struct event_base *base,
				      pgs_ssl_ctx_t *ssl_ctx, const char *sni);

#endif
