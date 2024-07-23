// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2012-2013  ProFUSION embedded systems
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#include <libkmod/libkmod.h>

#include "kmod.h"

#define PRIO_MAX_SIZE 32

static bool log_use_syslog;
static int log_priority = LOG_WARNING;

static const char *prio_to_str(char buf[static PRIO_MAX_SIZE], int prio)
{
	const char *prioname;

	switch (prio) {
	case LOG_CRIT:
		prioname = "FATAL";
		break;
	case LOG_ERR:
		prioname = "ERROR";
		break;
	case LOG_WARNING:
		prioname = "WARNING";
		break;
	case LOG_NOTICE:
		prioname = "NOTICE";
		break;
	case LOG_INFO:
		prioname = "INFO";
		break;
	case LOG_DEBUG:
		prioname = "DEBUG";
		break;
	default:
		snprintf(buf, PRIO_MAX_SIZE, "LOG-%03d", prio);
		prioname = buf;
	}

	return prioname;
}

_printf_format_(6, 0)
static void log_kmod(void *data, int priority, const char *file, int line,
		     const char *fn, const char *format, va_list args)
{
	char buf[PRIO_MAX_SIZE];
	const char *prioname;
	char *str;

	prioname = prio_to_str(buf, priority);

	if (vasprintf(&str, format, args) < 0)
		return;

	if (log_use_syslog) {
#ifdef ENABLE_DEBUG
		syslog(priority, "%s: %s:%d %s() %s", prioname, file, line,
		       fn, str);
#else
		syslog(priority, "%s: %s", prioname, str);
#endif
	} else {
#ifdef ENABLE_DEBUG
		fprintf(stderr, "%s: %s: %s:%d %s() %s",
			program_invocation_short_name, prioname, file, line,
			fn, str);
#else
		fprintf(stderr, "%s: %s: %s", program_invocation_short_name,
			prioname, str);
#endif
	}

	free(str);
	(void)data;
}
void log_open(bool use_syslog)
{
	log_use_syslog = use_syslog;

	if (log_use_syslog)
		openlog(program_invocation_short_name, LOG_CONS, LOG_DAEMON);
}

void log_close(void)
{
	if (log_use_syslog)
		closelog();
}

void log_printf(int prio, const char *fmt, ...)
{
	char buf[PRIO_MAX_SIZE];
	const char *prioname;
	char *msg;
	va_list args;

	if (prio > log_priority)
		return;

	va_start(args, fmt);
	if (vasprintf(&msg, fmt, args) < 0)
		msg = NULL;
	va_end(args);
	if (msg == NULL)
		return;

	prioname = prio_to_str(buf, prio);

	if (log_use_syslog)
		syslog(prio, "%s: %s", prioname, msg);
	else
		fprintf(stderr, "%s: %s: %s", program_invocation_short_name,
			prioname, msg);
	free(msg);

	if (prio <= LOG_CRIT)
		exit(EXIT_FAILURE);
}

void log_setup_kmod_log(struct kmod_ctx *ctx, int priority)
{
	log_priority = priority;

	kmod_set_log_priority(ctx, log_priority);
	kmod_set_log_fn(ctx, log_kmod, NULL);
}
