/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2011-2013  ProFUSION embedded systems
 */
#pragma once

#include <stddef.h>

#if defined(HAVE_STATIC_ASSERT)
#define assert_cc(expr) \
	_Static_assert((expr), #expr)
#else
#define assert_cc(expr) \
       do { (void) sizeof(char [1 - 2*!(expr)]); } while(0)
#endif

#define check_types_match(expr1, expr2)		\
	((typeof(expr1) *)0 != (typeof(expr2) *)0)

#define container_of(member_ptr, containing_type, member)		\
	((containing_type *)						\
	 ((char *)(member_ptr) - offsetof(containing_type, member))	\
	 - check_types_match(*(member_ptr), ((containing_type *)0)->member))


/* Two gcc extensions.
 * &a[0] degrades to a pointer: a different type from an array */
#define _array_size_chk(arr) ({ \
	assert_cc(!__builtin_types_compatible_p(typeof(arr), typeof(&(arr)[0]))); \
	0; \
	})

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]) + _array_size_chk(arr))

#define XSTRINGIFY(x) #x
#define STRINGIFY(x) XSTRINGIFY(x)

#define XCONCATENATE(x, y) x ## y
#define CONCATENATE(x, y) XCONCATENATE(x, y)
#define UNIQ(x) CONCATENATE(x, __COUNTER__)

/* Attributes */

#define _must_check_ __attribute__((warn_unused_result))
#define _printf_format_(a,b) __attribute__((format (printf, a, b)))
#define _unused_ __attribute__((unused))
#define _always_inline_ __inline__ __attribute__((always_inline))
#define _cleanup_(x) __attribute__((cleanup(x)))
#define _nonnull_(...) __attribute__((nonnull (__VA_ARGS__)))
#define _nonnull_all_ __attribute__((nonnull))

/* Define C11 noreturn without <stdnoreturn.h> and even on older gcc
 * compiler versions */
#ifndef noreturn
#if defined(HAVE_NORETURN)
#define noreturn _Noreturn
#else
#define noreturn __attribute__((noreturn))
#endif
#endif
