/*
 * Copyright (C) 2012  Lucas De Marchi <lucas.de.marchi@gmail.com
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "mkdir.h"
#include "testsuite.h"

TS_EXPORT int mkdir_p(const char *path, mode_t mode)
{
	char *start = strdupa(path);
	int len = strlen(path);
	char *end = start + len;
	struct stat st;

	/*
	 * scan backwards, replacing '/' with '\0' while the component doesn't
	 * exist
	 */
	for (;;) {
		if (stat(start, &st) >= 0) {
			if (S_ISDIR(st.st_mode))
				break;
			return -ENOTDIR;
		}

		/* Find the next component, backwards, discarding extra '/'*/
		for (; end != start && *end != '/'; end--)
			;

		for (; end != start - 1 && *end == '/'; end--)
			;

		end++;
		if (end == start)
			break;

		*end = '\0';
	}

	if (end == start + len)
		return 0;

	for (; end < start + len;) {
		*end = '/';
		end += strlen(end);

		if (mkdir(start, mode) < 0)
			return -errno;
	}

	return 0;
}
