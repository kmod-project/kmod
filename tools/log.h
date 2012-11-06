/*
 * kmod - log infrastructure
 *
 * Copyright (C) 2012  ProFUSION embedded systems
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdbool.h>
#include <stdio.h>
#include <syslog.h>

#include "kmod.h"

void log_open(bool use_syslog);
void log_close(void);
void log_kmod(void *data, int priority, const char *file, int line,
	      const char *fn, const char *format, va_list args);

_always_inline_ const char *prio_to_str(int prio);


/* inline functions */

_always_inline_ const char *prio_to_str(int prio)
{
	const char *prioname;
	char buf[32];

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
		snprintf(buf, sizeof(buf), "LOG-%03d", prio);
		prioname = buf;
	}

	return prioname;
}
