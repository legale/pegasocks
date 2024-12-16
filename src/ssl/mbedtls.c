#include "defs.h"
#include "ssl.h"

#include <mbedtls/ssl.h>
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include <mbedtls/error.h>

#include <stdlib.h>

struct pgs_ssl_ctx_s {
	mbedtls_ssl_config conf;
	mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context ctr_drbg;
	mbedtls_x509_crt cacert;
};

pgs_ssl_ctx_t *pgs_ssl_ctx_new(pgs_config_t *config)
{
	pgs_ssl_ctx_t *ctx = malloc(sizeof(pgs_ssl_ctx_t));

	mbedtls_ssl_config_init(&ctx->conf);
	mbedtls_ctr_drbg_init(&ctx->ctr_drbg);
	mbedtls_entropy_init(&ctx->entropy);
	mbedtls_x509_crt_init(&ctx->cacert);

	if (mbedtls_ctr_drbg_seed(&ctx->ctr_drbg, mbedtls_entropy_func,
				  &ctx->entropy, NULL, 0)) {
		goto error;
	}

	if (mbedtls_ssl_config_defaults(&ctx->conf, MBEDTLS_SSL_IS_CLIENT,
					MBEDTLS_SSL_TRANSPORT_STREAM,
					MBEDTLS_SSL_PRESET_DEFAULT)) {
		goto error;
	}

	mbedtls_ssl_conf_rng(&ctx->conf, mbedtls_ctr_drbg_random,
			     &ctx->ctr_drbg);

	if (!config->ssl_verify) {
		mbedtls_ssl_conf_authmode(&ctx->conf, MBEDTLS_SSL_VERIFY_NONE);
	} else {
		bool cert_loaded = false;
		if (config->ssl_crt) {
			if (mbedtls_x509_crt_parse_file(&ctx->cacert,
							config->ssl_crt) != 0) {
				pgs_config_error(config,
						 "Failed to load cert: %s",
						 config->ssl_crt);
			} else {
				cert_loaded = true;
				pgs_config_info(config, "cert: %s loaded",
						config->ssl_crt);
				mbedtls_ssl_conf_ca_chain(&ctx->conf,
							  &ctx->cacert, NULL);
				mbedtls_ssl_conf_authmode(
					&ctx->conf,
					MBEDTLS_SSL_VERIFY_REQUIRED);
			}
		}
		if (!cert_loaded) {
			// fallback
			mbedtls_ssl_conf_authmode(&ctx->conf,
						  MBEDTLS_SSL_VERIFY_NONE);
		}
	}

	return ctx;

error:
	pgs_ssl_ctx_free(ctx);
	return NULL;
}

void pgs_ssl_ctx_free(pgs_ssl_ctx_t *ctx)
{
	mbedtls_x509_crt_free(&ctx->cacert);
	mbedtls_ctr_drbg_free(&ctx->ctr_drbg);
	mbedtls_entropy_free(&ctx->entropy);
	mbedtls_ssl_config_free(&ctx->conf);
	free(ctx);
}

int pgs_session_outbound_ssl_bev_init(struct bufferevent **bev, int fd,
				      struct event_base *base,
				      pgs_ssl_ctx_t *ssl_ctx, const char *sni)
{
	// Allocate and initialize the mbedTLS SSL context
	mbedtls_ssl_context *ssl = malloc(sizeof(mbedtls_ssl_context));
	if (!ssl) {
		return -1; // Memory allocation failure
	}
	mbedtls_ssl_init(ssl);

	// Set up the mbedTLS SSL context
	int ret = 0;
	if ((ret = mbedtls_ssl_setup(ssl, &ssl_ctx->conf)) != 0) {
		mbedtls_ssl_free(ssl);
		free(ssl);
		return -1;
	}
	if ((ret = mbedtls_ssl_set_hostname(ssl, sni)) != 0) {
		mbedtls_ssl_free(ssl);
		free(ssl);
		return -1;
	}

	// Create a new bufferevent with the initialized mbedTLS SSL context
	*bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
	if (!*bev) {
		mbedtls_ssl_free(ssl);
		free(ssl);
		return -1;
	}

	// Set callbacks and associate mbedTLS SSL context as the cbarg
	bufferevent_setcb(*bev, NULL, NULL, NULL, ssl);

	return 0;
}
