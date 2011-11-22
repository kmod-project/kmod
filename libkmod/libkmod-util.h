#ifndef _LIBKMOD_UTIL_H_
#define _LIBKMOD_UTIL_H_

#include <stddef.h>

#define BUILD_ASSERT(cond) \
	do { (void) sizeof(char [1 - 2*!(cond)]); } while(0)

#define EXPR_BUILD_ASSERT(cond) \
	(sizeof(char [1 - 2*!(cond)]) - 1)

#if HAVE_TYPEOF
#define check_type(expr, type)			\
	((typeof(expr) *)0 != (type *)0)

#define check_types_match(expr1, expr2)		\
	((typeof(expr1) *)0 != (typeof(expr2) *)0)
#else
/* Without typeof, we can only test the sizes. */
#define check_type(expr, type)					\
	EXPR_BUILD_ASSERT(sizeof(expr) == sizeof(type))

#define check_types_match(expr1, expr2)				\
	EXPR_BUILD_ASSERT(sizeof(expr1) == sizeof(expr2))
#endif /* HAVE_TYPEOF */

#define container_of(member_ptr, containing_type, member)		\
	((containing_type *)						\
	 ((char *)(member_ptr) - offsetof(containing_type, member))	\
	 - check_types_match(*(member_ptr), ((containing_type *)0)->member))

#endif
