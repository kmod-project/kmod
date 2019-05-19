/*
 * libkmod - module signature display
 *
 * Copyright (C) 2013 Michal Marek, SUSE
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include <endian.h>
#include <inttypes.h>
#ifdef ENABLE_OPENSSL
#include <openssl/pkcs7.h>
#include <openssl/ssl.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
	PKEY_ALGO__LAST
};

static const char *const pkey_algo[PKEY_ALGO__LAST] = {
	[PKEY_ALGO_DSA]		= "DSA",
	[PKEY_ALGO_RSA]		= "RSA",
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
	PKEY_HASH__LAST
};

const char *const pkey_hash_algo[PKEY_HASH__LAST] = {
	[PKEY_HASH_MD4]		= "md4",
	[PKEY_HASH_MD5]		= "md5",
	[PKEY_HASH_SHA1]	= "sha1",
	[PKEY_HASH_RIPE_MD_160]	= "rmd160",
	[PKEY_HASH_SHA256]	= "sha256",
	[PKEY_HASH_SHA384]	= "sha384",
	[PKEY_HASH_SHA512]	= "sha512",
	[PKEY_HASH_SHA224]	= "sha224",
};

enum pkey_id_type {
	PKEY_ID_PGP,		/* OpenPGP generated key ID */
	PKEY_ID_X509,		/* X.509 arbitrary subjectKeyIdentifier */
	PKEY_ID_PKCS7,		/* Signature in PKCS#7 message */
	PKEY_ID_TYPE__LAST
};

const char *const pkey_id_type[PKEY_ID_TYPE__LAST] = {
	[PKEY_ID_PGP]		= "PGP",
	[PKEY_ID_X509]		= "X509",
	[PKEY_ID_PKCS7]		= "PKCS#7",
};

/*
 * Module signature information block.
 */
struct module_signature {
	uint8_t algo;        /* Public-key crypto algorithm [enum pkey_algo] */
	uint8_t hash;        /* Digest algorithm [enum pkey_hash_algo] */
	uint8_t id_type;     /* Key identifier type [enum pkey_id_type] */
	uint8_t signer_len;  /* Length of signer's name */
	uint8_t key_id_len;  /* Length of key identifier */
	uint8_t __pad[3];
	uint32_t sig_len;    /* Length of signature data (big endian) */
};

static bool fill_default(const char *mem, off_t size,
			 const struct module_signature *modsig, size_t sig_len,
			 struct kmod_signature_info *sig_info)
{
	size -= sig_len;
	sig_info->sig = mem + size;
	sig_info->sig_len = sig_len;

	size -= modsig->key_id_len;
	sig_info->key_id = mem + size;
	sig_info->key_id_len = modsig->key_id_len;

	size -= modsig->signer_len;
	sig_info->signer = mem + size;
	sig_info->signer_len = modsig->signer_len;

	sig_info->algo = pkey_algo[modsig->algo];
	sig_info->hash_algo = pkey_hash_algo[modsig->hash];
	sig_info->id_type = pkey_id_type[modsig->id_type];

	return true;
}

#ifdef ENABLE_OPENSSL

struct pkcs7_private {
	PKCS7 *pkcs7;
	unsigned char *key_id;
	BIGNUM *sno;
};

static void pkcs7_free(void *s)
{
	struct kmod_signature_info *si = s;
	struct pkcs7_private *pvt = si->private;

	PKCS7_free(pvt->pkcs7);
	BN_free(pvt->sno);
	free(pvt->key_id);
	free(pvt);
	si->private = NULL;
}

static int obj_to_hash_algo(const ASN1_OBJECT *o)
{
	int nid;

	nid = OBJ_obj2nid(o);
	switch (nid) {
	case NID_md4:
		return PKEY_HASH_MD4;
	case NID_md5:
		return PKEY_HASH_MD5;
	case NID_sha1:
		return PKEY_HASH_SHA1;
	case NID_ripemd160:
		return PKEY_HASH_RIPE_MD_160;
	case NID_sha256:
		return PKEY_HASH_SHA256;
	case NID_sha384:
		return PKEY_HASH_SHA384;
	case NID_sha512:
		return PKEY_HASH_SHA512;
	case NID_sha224:
		return PKEY_HASH_SHA224;
	default:
		return -1;
	}
	return -1;
}

static const char *x509_name_to_str(X509_NAME *name)
{
	int i;
	X509_NAME_ENTRY *e;
	ASN1_STRING *d;
	ASN1_OBJECT *o;
	int nid = -1;
	const char *str;

	for (i = 0; i < X509_NAME_entry_count(name); i++) {
		e = X509_NAME_get_entry(name, i);
		o = X509_NAME_ENTRY_get_object(e);
		nid = OBJ_obj2nid(o);
		if (nid == NID_commonName)
			break;
	}
	if (nid == -1)
		return NULL;

	d = X509_NAME_ENTRY_get_data(e);
	str = (const char *)ASN1_STRING_get0_data(d);

	return str;
}

static bool fill_pkcs7(const char *mem, off_t size,
		       const struct module_signature *modsig, size_t sig_len,
		       struct kmod_signature_info *sig_info)
{
	const char *pkcs7_raw;
	PKCS7 *pkcs7;
	STACK_OF(PKCS7_SIGNER_INFO) *sis;
	PKCS7_SIGNER_INFO *si;
	PKCS7_ISSUER_AND_SERIAL *is;
	X509_NAME *issuer;
	ASN1_INTEGER *sno;
	ASN1_OCTET_STRING *sig;
	BIGNUM *sno_bn;
	X509_ALGOR *dig_alg;
	X509_ALGOR *sig_alg;
	const ASN1_OBJECT *o;
	BIO *in;
	int len;
	unsigned char *key_id_str;
	struct pkcs7_private *pvt;
	const char *issuer_str;

	size -= sig_len;
	pkcs7_raw = mem + size;

	in = BIO_new_mem_buf(pkcs7_raw, sig_len);

	pkcs7 = d2i_PKCS7_bio(in, NULL);
	if (pkcs7 == NULL) {
		BIO_free(in);
		return false;
	}

	BIO_free(in);

	sis = PKCS7_get_signer_info(pkcs7);
	if (sis == NULL)
		goto err;

	si = sk_PKCS7_SIGNER_INFO_value(sis, 0);
	if (si == NULL)
		goto err;

	is = si->issuer_and_serial;
	if (is == NULL)
		goto err;
	issuer = is->issuer;
	sno = is->serial;

	sig = si->enc_digest;
	if (sig == NULL)
		goto err;

	PKCS7_SIGNER_INFO_get0_algs(si, NULL, &dig_alg, &sig_alg);

	sig_info->sig = (const char *)ASN1_STRING_get0_data(sig);
	sig_info->sig_len = ASN1_STRING_length(sig);

	sno_bn = ASN1_INTEGER_to_BN(sno, NULL);
	if (sno_bn == NULL)
		goto err;

	len = BN_num_bytes(sno_bn);
	key_id_str = malloc(len);
	if (key_id_str == NULL)
		goto err2;
	BN_bn2bin(sno_bn, key_id_str);

	sig_info->key_id = (const char *)key_id_str;
	sig_info->key_id_len = len;

	issuer_str = x509_name_to_str(issuer);
	if (issuer_str != NULL) {
		sig_info->signer = issuer_str;
		sig_info->signer_len = strlen(issuer_str);
	}

	X509_ALGOR_get0(&o, NULL, NULL, dig_alg);

	sig_info->hash_algo = pkey_hash_algo[obj_to_hash_algo(o)];
	sig_info->id_type = pkey_id_type[modsig->id_type];

	pvt = malloc(sizeof(*pvt));
	if (pvt == NULL)
		goto err3;

	pvt->pkcs7 = pkcs7;
	pvt->key_id = key_id_str;
	pvt->sno = sno_bn;
	sig_info->private = pvt;

	sig_info->free = pkcs7_free;

	return true;
err3:
	free(key_id_str);
err2:
	BN_free(sno_bn);
err:
	PKCS7_free(pkcs7);
	return false;
}

#else /* ENABLE OPENSSL */

static bool fill_pkcs7(const char *mem, off_t size,
		       const struct module_signature *modsig, size_t sig_len,
		       struct kmod_signature_info *sig_info)
{
	sig_info->hash_algo = "unknown";
	sig_info->id_type = pkey_id_type[modsig->id_type];
	return true;
}

#endif /* ENABLE OPENSSL */

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

bool kmod_module_signature_info(const struct kmod_file *file, struct kmod_signature_info *sig_info)
{
	const char *mem;
	off_t size;
	const struct module_signature *modsig;
	size_t sig_len;

	size = kmod_file_get_size(file);
	mem = kmod_file_get_contents(file);
	if (size < (off_t)strlen(SIG_MAGIC))
		return false;
	size -= strlen(SIG_MAGIC);
	if (memcmp(SIG_MAGIC, mem + size, strlen(SIG_MAGIC)) != 0)
		return false;

	if (size < (off_t)sizeof(struct module_signature))
		return false;
	size -= sizeof(struct module_signature);
	modsig = (struct module_signature *)(mem + size);
	if (modsig->algo >= PKEY_ALGO__LAST ||
			modsig->hash >= PKEY_HASH__LAST ||
			modsig->id_type >= PKEY_ID_TYPE__LAST)
		return false;
	sig_len = be32toh(get_unaligned(&modsig->sig_len));
	if (sig_len == 0 ||
	    size < (int64_t)(modsig->signer_len + modsig->key_id_len + sig_len))
		return false;

	switch (modsig->id_type) {
	case PKEY_ID_PKCS7:
		return fill_pkcs7(mem, size, modsig, sig_len, sig_info);
	default:
		return fill_default(mem, size, modsig, sig_len, sig_info);
	}
}

void kmod_module_signature_info_free(struct kmod_signature_info *sig_info)
{
	if (sig_info->free)
		sig_info->free(sig_info);
}
