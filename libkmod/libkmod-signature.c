// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2013 Michal Marek, SUSE
 */

#include <endian.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <shared/util.h>

#include "libkmod-internal-signature.h"

/* These types and tables were copied from the 3.7 kernel sources.
 * As this is just description of the signature format, it should not be
 * considered derived work (so libkmod can use the LGPL license).
 */
enum pkey_algo {
	PKEY_ALGO_DSA,
	PKEY_ALGO_RSA,
	PKEY_ALGO__LAST,
};

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

const char *const pkey_hash_algo[PKEY_HASH__LAST] = {
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

enum pkey_id_type {
	PKEY_ID_PGP, /* OpenPGP generated key ID */
	PKEY_ID_X509, /* X.509 arbitrary subjectKeyIdentifier */
	PKEY_ID_PKCS7, /* Signature in PKCS#7 message */
	PKEY_ID_TYPE__LAST,
};

const char *const pkey_id_type[PKEY_ID_TYPE__LAST] = {
	[PKEY_ID_PGP] = "PGP",
	[PKEY_ID_X509] = "X509",
	[PKEY_ID_PKCS7] = "PKCS#7",
};

/*
 * Module signature information block.
 */
struct module_signature {
	uint8_t algo; /* Public-key crypto algorithm [enum pkey_algo] */
	uint8_t hash; /* Digest algorithm [enum pkey_hash_algo] */
	uint8_t id_type; /* Key identifier type [enum pkey_id_type] */
	uint8_t signer_len; /* Length of signer's name */
	uint8_t key_id_len; /* Length of key identifier */
	uint8_t __pad[3];
	uint32_t sig_len; /* Length of signature data (big endian) */
};

static bool fill_default(const char *mem, off_t size,
			 const struct module_signature *modsig, size_t sig_len,
			 struct kmod_signature_info **out_sig_info)
{
	struct kmod_signature_info *sig_info = calloc(1, sizeof(*sig_info));
	if (sig_info == NULL)
		return false;

	size -= sig_len;
	sig_info->sig = mem + size;
	sig_info->sig_len = sig_len;

	size -= modsig->key_id_len;
	sig_info->key_id = mem + size;
	sig_info->key_id_len = modsig->key_id_len;

	size -= modsig->signer_len;
	sig_info->signer = mem + size;
	sig_info->signer_len = modsig->signer_len;

	sig_info->hash_algo = pkey_hash_algo[modsig->hash];

	*out_sig_info = sig_info;

	return true;
}

static bool kmod_fill_pkcs7_dummy(const char *mem, off_t size, size_t sig_len,
				  struct kmod_signature_info **out_sig_info)
{
	struct kmod_signature_info *sig_info = calloc(1, sizeof(*sig_info));
	if (sig_info == NULL)
		return false;

	sig_info->hash_algo = "unknown";
	*out_sig_info = sig_info;
	return true;
}

#define SIG_MAGIC "~Module signature appended~\n"

/*
 * A signed module has the following layout:
 *
 * [ module                  ]
 * [ signer's name           ]
 * [ key identifier          ]
 * [ signature data          ]
 * [ struct module_signature ]
 * [ SIG_MAGIC               ]
 */

bool kmod_module_signature_info(const struct kmod_file *file,
				struct kmod_signature_info **sig_info)
{
	const char *mem;
	const void *contents;
	off_t size;
	struct module_signature modsig;
	size_t sig_len;
	bool ret;

	ret = kmod_file_get_contents(file, &contents, &size);
	if (ret)
		return false;

	mem = contents;

	if (size < (off_t)strlen(SIG_MAGIC))
		return false;
	size -= strlen(SIG_MAGIC);
	if (!strstartswith(mem + size, SIG_MAGIC))
		return false;

	if (size < (off_t)sizeof(struct module_signature))
		return false;
	size -= sizeof(struct module_signature);
	memcpy(&modsig, mem + size, sizeof(struct module_signature));
	/* The algo value seems to be hard-coded to PKEY_ALGO_RSA, so we don't bother
	 * parsing and/or printing it.
	 */
	if (modsig.algo >= PKEY_ALGO__LAST || modsig.hash >= PKEY_HASH__LAST ||
	    modsig.id_type >= PKEY_ID_TYPE__LAST)
		return false;
	sig_len = be32toh(modsig.sig_len);
	if (sig_len == 0 ||
	    size < (int64_t)sig_len + modsig.signer_len + modsig.key_id_len)
		return false;

	switch (modsig.id_type) {
	case PKEY_ID_PKCS7:
		ret = kmod_fill_pkcs7_openssl(mem, size, sig_len, sig_info);
		if (!ret)
			ret = kmod_fill_pkcs7_mbedtls(mem, size, sig_len, sig_info);
		if (!ret)
			ret = kmod_fill_pkcs7_dummy(mem, size, sig_len, sig_info);
		break;
	default:
		ret = fill_default(mem, size, &modsig, sig_len, sig_info);
		break;
	}

	if (ret)
		(*sig_info)->id_type = pkey_id_type[modsig.id_type];

	return ret;
}
