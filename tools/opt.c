// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2011-2013  ProFUSION embedded systems
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "kmod.h"

int options_from_array(char **args, int nargs, char **output)
{
	char *opts = NULL;
	size_t optslen = 0;
	int i, err = 0;

	for (i = 0; i < nargs; i++) {
		size_t len = strlen(args[i]);
		size_t qlen = 0;
		const char *value;
		void *tmp;

		value = strchr(args[i], '=');
		if (value) {
			value++;
			if (*value != '"' && *value != '\'') {
				if (strchr(value, ' '))
					qlen = 2;
			}
		}

		tmp = realloc(opts, optslen + len + qlen + 2);
		if (!tmp) {
			err = -ENOMEM;
			free(opts);
			opts = NULL;
			ERR("could not gather module options: out-of-memory\n");
			break;
		}
		opts = tmp;
		if (optslen > 0) {
			opts[optslen] = ' ';
			optslen++;
		}
		if (qlen == 0) {
			memcpy(opts + optslen, args[i], len + 1);
			optslen += len;
		} else {
			size_t keylen = value - args[i];
			size_t valuelen = len - keylen;
			memcpy(opts + optslen, args[i], keylen);
			optslen += keylen;
			opts[optslen] = '"';
			optslen++;
			memcpy(opts + optslen, value, valuelen);
			optslen += valuelen;
			opts[optslen] = '"';
			optslen++;
			opts[optslen] = '\0';
		}
	}

	*output = opts;
	return err;
}
