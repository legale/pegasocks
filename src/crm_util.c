#include "crm_util.h"
#include <openssl/evp.h>

void sha224(const crm_buf_t *input, crm_size_t input_len, crm_buf_t *res,
	    crm_size_t *res_len)
{
	EVP_MD_CTX *ctx;
	if ((ctx = EVP_MD_CTX_new()) == NULL)
		goto error;
	if (!EVP_DigestInit_ex(ctx, EVP_sha224(), NULL))
		goto error;
	if (!EVP_DigestUpdate(ctx, input, input_len))
		goto error;
	if (!EVP_DigestFinal_ex(ctx, res, (unsigned int *)res_len))
		goto error;

	EVP_MD_CTX_free(ctx);
	return;

error:
	perror("error sha224");
	if (ctx != NULL)
		EVP_MD_CTX_free(ctx);
	*res_len = 0;
}

crm_buf_t *to_hexstring(const crm_buf_t *buf, crm_size_t size)
{
	crm_buf_t *hexbuf = crm_malloc(sizeof(crm_buf_t) * (2 * size + 1));
	for (int i = 0; i < size; i++) {
		sprintf((char *)hexbuf + i * 2, "%02x", (int)buf[i]);
	}
	hexbuf[2 * size] = '\0';
	return hexbuf;
}
