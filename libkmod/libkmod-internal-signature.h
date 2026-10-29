// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Emil Velikov <emil.l.velikov@gmail.com>

#include <libkmod/libkmod-internal.h>

enum pkey_hash_algo {
	PKEY_HASH_MD4,
	PKEY_HASH_MD5,
	PKEY_HASH_SHA1,
	PKEY_HASH_RIPE_MD_160,
	PKEY_HASH_SHA256,
	PKEY_HASH_SHA384,
	PKEY_HASH_SHA512,
	PKEY_HASH_SHA224,
	PKEY_HASH_RIPE_MD_128,
	PKEY_HASH_RIPE_MD_256,
	PKEY_HASH_RIPE_MD_320,
	PKEY_HASH_WP_256,
	PKEY_HASH_WP_384,
	PKEY_HASH_WP_512,
	PKEY_HASH_TGR_128,
	PKEY_HASH_TGR_160,
	PKEY_HASH_TGR_192,
	PKEY_HASH_SM3_256,
	PKEY_HASH_STREEBOG_256,
	PKEY_HASH_STREEBOG_512,
	PKEY_HASH_SHA3_256,
	PKEY_HASH_SHA3_384,
	PKEY_HASH_SHA3_512,
	PKEY_HASH__LAST,
};

extern const char *const pkey_hash_algo[PKEY_HASH__LAST];

#if ENABLE_OPENSSL
bool kmod_fill_pkcs7_openssl(const char *mem, off_t size, size_t sig_len,
			     struct kmod_signature_info **out_sig_info);
#else
static inline bool kmod_fill_pkcs7_openssl(const char *mem, off_t size, size_t sig_len,
					   struct kmod_signature_info **out_sig_info)
{
	return false;
}
#endif

#if ENABLE_MBEDTLS
bool kmod_fill_pkcs7_mbedtls(const char *mem, off_t size, size_t sig_len,
			     struct kmod_signature_info **out_sig_info);
#else
static inline bool kmod_fill_pkcs7_mbedtls(const char *mem, off_t size, size_t sig_len,
					   struct kmod_signature_info **out_sig_info)
{
	return false;
}
#endif
