// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2012-2013  ProFUSION embedded systems
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <syslog.h>

#include "kmod.h"

void log_open(bool use_syslog);
void log_close(void);
void log_printf(int prio, const char *fmt, ...) _printf_format_(2, 3);
#define CRIT(...) log_printf(LOG_CRIT, __VA_ARGS__)
#define ERR(...) log_printf(LOG_ERR, __VA_ARGS__)
#define WRN(...) log_printf(LOG_WARNING, __VA_ARGS__)
#define INF(...) log_printf(LOG_INFO, __VA_ARGS__)
#define DBG(...) log_printf(LOG_DEBUG, __VA_ARGS__)

struct kmod_ctx;
void log_setup_kmod_log(struct kmod_ctx *ctx, int priority);

#define LOG_PTR_INIT(name) static char *name = NULL;

/* SET_LOG_PTR: try to allocate the error message into a string pointed by
 * msg_ptr, but if it fails it will just print it as ERR().
 * msg_ptr must be defined by the caller and initialized to NULL  */
#define SET_LOG_PTR(msg_ptr, ...)				\
	do { 							\
		if (msg_ptr) 					\
			free(msg_ptr); 				\
		if (asprintf(&msg_ptr, __VA_ARGS__) < 0)	\
			ERR(__VA_ARGS__);			\
	} while (0);

static inline char *pop_log_str(char **msg_ptr)
{
       char *ret = *msg_ptr;
       *msg_ptr = NULL;
       return ret;
}

#define PRINT_LOG_PTR(prio, module_dir, module_alt_dir)			\
	do { 								\
		if (module_dir) {					\
			log_printf(prio, "%s", module_dir);		\
			if (module_alt_dir &&				\
			    strcmp(module_dir, module_alt_dir) != 0) {	\
				log_printf(prio, "%s", module_alt_dir);	\
				free(module_alt_dir);			\
			}						\
			free(module_dir);				\
		}							\
	} while (0);
