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
