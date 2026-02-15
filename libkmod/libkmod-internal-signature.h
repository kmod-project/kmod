// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Emil Velikov <emil.l.velikov@gmail.com>

#include <libkmod/libkmod-internal.h>

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
