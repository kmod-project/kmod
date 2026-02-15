// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2026 Emil Velikov
 */

#define DLSYM_LOCALLY_ENABLED ENABLE_MBEDTLS_DLOPEN

#include <inttypes.h>
#include <mbedtls/oid.h> // for MBEDTLS_OID_DIGEST_ALG_*
#include <mbedtls/pkcs7.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <shared/elf-note.h>
#include <shared/util.h>

#include "libkmod-internal-signature.h"

/* XXX: temporary copy from libkmod-signature.c */
enum pkey_hash_algo {
	PKEY_HASH_MD4,
	PKEY_HASH_MD5,
	PKEY_HASH_SHA1,
	PKEY_HASH_RIPE_MD_160,
	PKEY_HASH_SHA256,
	PKEY_HASH_SHA384,
	PKEY_HASH_SHA512,
	PKEY_HASH_SHA224,
	PKEY_HASH_SM3,
	PKEY_HASH__LAST,
};

/* XXX: ditto */
static const char *const pkey_hash_algo[PKEY_HASH__LAST] = {
	// clang-format off
	[PKEY_HASH_MD4] = "md4",
	[PKEY_HASH_MD5] = "md5",
	[PKEY_HASH_SHA1] = "sha1",
	[PKEY_HASH_RIPE_MD_160] = "rmd160",
	[PKEY_HASH_SHA256] = "sha256",
	[PKEY_HASH_SHA384] = "sha384",
	[PKEY_HASH_SHA512] = "sha512",
	[PKEY_HASH_SHA224] = "sha224",
	[PKEY_HASH_SM3] = "sm3",
	// clang-format on
};

#define DL_SYMBOL_TABLE(M)         \
	M(mbedtls_pkcs7_init)      \
	M(mbedtls_pkcs7_parse_der) \
	M(mbedtls_pkcs7_free)

DL_SYMBOL_TABLE(DECLARE_SYM)

static int dlopen_mbedx509(void)
{
#if !DLSYM_LOCALLY_ENABLED
	return 0;
#else
	static void *dl;

	ELF_NOTE_DLOPEN("mbedtls", "Support for reading module signatures",
			ELF_NOTE_DLOPEN_PRIORITY_SUGGESTED, "libmbedx509.so.7");

	return dlsym_many(&dl, "libmbedx509.so.7", DL_SYMBOL_TABLE(DLSYM_ARG) NULL);
#endif
}

static int to_hash_algo(const mbedtls_pkcs7_buf *oid)
{
	/* MBEDTLS_OID_DIGEST_ALG_MD4 was removed around mbedtls v3.0 */
	if (!MBEDTLS_OID_CMP(MBEDTLS_OID_DIGEST_ALG_MD5, oid))
		return PKEY_HASH_MD5;

	if (!MBEDTLS_OID_CMP(MBEDTLS_OID_DIGEST_ALG_SHA1, oid))
		return PKEY_HASH_SHA1;

	if (!MBEDTLS_OID_CMP(MBEDTLS_OID_DIGEST_ALG_RIPEMD160, oid))
		return PKEY_HASH_RIPE_MD_160;

	if (!MBEDTLS_OID_CMP(MBEDTLS_OID_DIGEST_ALG_SHA256, oid))
		return PKEY_HASH_SHA256;

	if (!MBEDTLS_OID_CMP(MBEDTLS_OID_DIGEST_ALG_SHA384, oid))
		return PKEY_HASH_SHA384;

	if (!MBEDTLS_OID_CMP(MBEDTLS_OID_DIGEST_ALG_SHA512, oid))
		return PKEY_HASH_SHA512;

	if (!MBEDTLS_OID_CMP(MBEDTLS_OID_DIGEST_ALG_SHA224, oid))
		return PKEY_HASH_SHA224;

	/* MBEDTLS_OID_DIGEST_ALG_SM3: Not available, as of 2026-02-14 */
	return -1;
}

bool kmod_fill_pkcs7_mbedtls(const char *mem, off_t size, size_t sig_len,
			     struct kmod_signature_info **out_sig_info)
{
	struct kmod_signature_info *sig_info;
	char *p;
	const struct mbedtls_pkcs7_signed_data *signed_data;
	const struct mbedtls_pkcs7_signer_info *signers;
	struct mbedtls_pkcs7 pkcs7;
	const unsigned char *pkcs7_raw;
	size_t total_len;
	int hash_algo, ret;

	ret = dlopen_mbedx509();
	if (ret < 0)
		return false;

	size -= sig_len;
	pkcs7_raw = (const unsigned char *)mem + size;

	sym_mbedtls_pkcs7_init(&pkcs7);
	ret = sym_mbedtls_pkcs7_parse_der(&pkcs7, pkcs7_raw, sig_len);
	if (ret < 0)
		goto err;

	signed_data = &pkcs7.MBEDTLS_PRIVATE(signed_data);
	signers = &signed_data->MBEDTLS_PRIVATE(signers);

	hash_algo = to_hash_algo(&signed_data->MBEDTLS_PRIVATE(digest_alg_identifiers));
	if (hash_algo < 0)
		goto err;

	/* Calculate the total length */
	total_len = sizeof(struct kmod_signature_info);

	/* signer */
	total_len += signers->MBEDTLS_PRIVATE(issuer).val.len;

	/* key_id */
	total_len += signers->MBEDTLS_PRIVATE(serial).len;

	/* hash_algo */
	/* XXX: using an incomplete and not-fully correct static table */

	/* sig */
	total_len += signers->MBEDTLS_PRIVATE(sig).len;

	sig_info = calloc(1, total_len);
	if (sig_info == NULL)
		goto err;

	p = (char *)sig_info;

	p += sizeof(struct kmod_signature_info);

	/* signer */
	sig_info->signer = p;
	sig_info->signer_len = signers->MBEDTLS_PRIVATE(issuer).val.len;

	memcpy(p, signers->MBEDTLS_PRIVATE(issuer).val.p, sig_info->signer_len);
	p += sig_info->signer_len;

	/* key_id */
	sig_info->key_id = p;
	sig_info->key_id_len = signers->MBEDTLS_PRIVATE(serial).len;

	memcpy(p, signers->MBEDTLS_PRIVATE(serial).p, sig_info->key_id_len);
	p += sig_info->key_id_len;

	/* hash_algo */
	sig_info->hash_algo = pkey_hash_algo[hash_algo];

	/* sig */
	sig_info->sig = p;
	sig_info->sig_len = signers->MBEDTLS_PRIVATE(sig).len;

	memcpy(p, signers->MBEDTLS_PRIVATE(sig).p, sig_info->sig_len);

	sym_mbedtls_pkcs7_free(&pkcs7);
	*out_sig_info = sig_info;

	return true;

err:
	sym_mbedtls_pkcs7_free(&pkcs7);
	return false;
}
