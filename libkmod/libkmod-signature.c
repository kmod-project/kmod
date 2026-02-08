// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2013 Michal Marek, SUSE
 */

#define DLSYM_LOCALLY_ENABLED ENABLE_OPENSSL_DLOPEN

#include <endian.h>
#include <inttypes.h>
#if ENABLE_OPENSSL
#include <openssl/pkcs7.h>
#include <openssl/ssl.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <shared/elf-note.h>
#include <shared/missing.h>
#include <shared/util.h>

#include "libkmod-internal.h"

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

#if ENABLE_OPENSSL

#define DL_SYMBOL_TABLE(M)             \
	M(ASN1_INTEGER_to_BN)          \
	M(ASN1_STRING_get0_data)       \
	M(ASN1_STRING_length)          \
	M(BIO_free)                    \
	M(BIO_new_mem_buf)             \
	M(BN_bn2bin)                   \
	M(BN_free)                     \
	M(BN_num_bits)                 \
	M(d2i_PKCS7_bio)               \
	M(OBJ_obj2nid)                 \
	M(OBJ_obj2txt)                 \
	M(OPENSSL_sk_value)            \
	M(PKCS7_free)                  \
	M(PKCS7_get_signer_info)       \
	M(PKCS7_SIGNER_INFO_get0_algs) \
	M(X509_ALGOR_get0)             \
	M(X509_NAME_entry_count)       \
	M(X509_NAME_ENTRY_get_data)    \
	M(X509_NAME_ENTRY_get_object)  \
	M(X509_NAME_get_entry)

DL_SYMBOL_TABLE(DECLARE_SYM)

/* Portion of the libcrypto/openssl API is nested inline functions and/or macros. As such, we
 * need to copy/paste a few so our forwarding works.
 */
#define sym_BN_num_bytes(a) ((sym_BN_num_bits(a) + 7) / 8)
#define sym_sk_PKCS7_SIGNER_INFO_value(sk, idx)     \
	((PKCS7_SIGNER_INFO *)sym_OPENSSL_sk_value( \
		ossl_check_const_PKCS7_SIGNER_INFO_sk_type(sk), (idx)))

static int dlopen_crypto(void)
{
#if !DLSYM_LOCALLY_ENABLED
	return 0;
#else
	static void *dl;

	ELF_NOTE_DLOPEN("openssl", "Support for reading module signatures",
			ELF_NOTE_DLOPEN_PRIORITY_SUGGESTED, "libcrypto.so.3");

	return dlsym_many(&dl, "libcrypto.so.3", DL_SYMBOL_TABLE(DLSYM_ARG) NULL);
#endif
}

static const char *x509_name_to_str(X509_NAME *name)
{
	int i;
	X509_NAME_ENTRY *e;
	ASN1_STRING *d;
	ASN1_OBJECT *o;
	int nid = -1;
	const char *str;

	for (i = 0; i < sym_X509_NAME_entry_count(name); i++) {
		e = sym_X509_NAME_get_entry(name, i);
		o = sym_X509_NAME_ENTRY_get_object(e);
		nid = sym_OBJ_obj2nid(o);
		if (nid == NID_commonName)
			break;
	}
	if (nid == -1)
		return NULL;

	d = sym_X509_NAME_ENTRY_get_data(e);
	str = (const char *)sym_ASN1_STRING_get0_data(d);

	return str;
}

static bool fill_pkcs7(const char *mem, off_t size, size_t sig_len,
		       struct kmod_signature_info **out_sig_info)
{
	struct kmod_signature_info *sig_info;
	char *p;
	const char *pkcs7_raw;
	PKCS7 *pkcs7;
	STACK_OF(PKCS7_SIGNER_INFO) * sis;
	PKCS7_SIGNER_INFO *si;
	const PKCS7_ISSUER_AND_SERIAL *is;
	const ASN1_OCTET_STRING *sig;
	BIGNUM *sno_bn;
	X509_ALGOR *dig_alg;
	const ASN1_OBJECT *o;
	BIO *in;
	const char *issuer_str;
	int hash_algo_len;
	size_t total_len;
	int ret;

	ret = dlopen_crypto();
	if (ret < 0)
		return false;

	size -= sig_len;
	pkcs7_raw = mem + size;

	in = sym_BIO_new_mem_buf(pkcs7_raw, sig_len);

	pkcs7 = sym_d2i_PKCS7_bio(in, NULL);
	sym_BIO_free(in);

	if (pkcs7 == NULL)
		goto err;

	sis = sym_PKCS7_get_signer_info(pkcs7);
	if (sis == NULL)
		goto err;

	si = sym_sk_PKCS7_SIGNER_INFO_value(sis, 0);
	if (si == NULL || si->issuer_and_serial == NULL || si->enc_digest == NULL)
		goto err;

	is = si->issuer_and_serial;

	/* Calculate the total length */
	total_len = sizeof(struct kmod_signature_info);

	/* signer */
	issuer_str = x509_name_to_str(is->issuer);
	if (issuer_str != NULL)
		total_len += strlen(issuer_str);

	/* key_id */
	sno_bn = sym_ASN1_INTEGER_to_BN(is->serial, NULL);
	if (sno_bn == NULL)
		goto err;

	total_len += sym_BN_num_bytes(sno_bn);

	/* hash_algo */
	sym_PKCS7_SIGNER_INFO_get0_algs(si, NULL, &dig_alg, NULL);
	sym_X509_ALGOR_get0(&o, NULL, NULL, dig_alg);
	hash_algo_len = sym_OBJ_obj2txt(NULL, 0, o, 0);
	if (hash_algo_len < 0)
		goto err1;

	total_len += hash_algo_len + 1;

	/* sig */
	sig = si->enc_digest;
	total_len += sym_ASN1_STRING_length(sig);

	sig_info = calloc(1, total_len);
	if (sig_info == NULL)
		goto err1;

	p = (char *)sig_info;

	p += sizeof(struct kmod_signature_info);

	/* signer */
	if (issuer_str != NULL) {
		sig_info->signer = p;
		sig_info->signer_len = strlen(issuer_str);

		memcpy(p, issuer_str, sig_info->signer_len);
		p += sig_info->signer_len;
	}

	/* key_id */
	sig_info->key_id = p;
	sig_info->key_id_len = sym_BN_num_bytes(sno_bn);

	sym_BN_bn2bin(sno_bn, (unsigned char *)p);
	p += sig_info->key_id_len;

	/* hash_algo */
	sig_info->hash_algo = p;

	hash_algo_len = sym_OBJ_obj2txt(p, hash_algo_len + 1, o, 0);
	if (hash_algo_len < 0)
		goto err2;
	p += hash_algo_len;

	*p = '\0';
	p++;

	/* sig */
	sig_info->sig = p;
	sig_info->sig_len = sym_ASN1_STRING_length(sig);

	memcpy(p, sym_ASN1_STRING_get0_data(sig), sig_info->sig_len);

	sym_BN_free(sno_bn);
	sym_PKCS7_free(pkcs7);

	*out_sig_info = sig_info;

	return true;

err2:
	free(sig_info);
err1:
	sym_BN_free(sno_bn);
err:
	sym_PKCS7_free(pkcs7);
	return false;
}

#else

static bool fill_pkcs7(const char *mem, off_t size, size_t sig_len,
		       struct kmod_signature_info **out_sig_info)
{
	struct kmod_signature_info *sig_info = calloc(1, sizeof(*sig_info));
	if (sig_info == NULL)
		return false;

	sig_info->hash_algo = "unknown";
	*out_sig_info = sig_info;
	return true;
}

#endif

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
		ret = fill_pkcs7(mem, size, sig_len, sig_info);
		break;
	default:
		ret = fill_default(mem, size, &modsig, sig_len, sig_info);
		break;
	}

	if (ret)
		(*sig_info)->id_type = pkey_id_type[modsig.id_type];

	return ret;
}
