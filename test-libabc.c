/*
    libabc - something with abc

    Copyright (C) 2011 Someone <someone@example.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
¶
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
¶
    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>

#include <libabc.h>

int main(int argc, char *argv[])
{
	struct abc_ctx *ctx;
	struct abc_thing *thing = NULL;
	int err;

	err = abc_new(&ctx);
	if (err < 0)
		exit(EXIT_FAILURE);

	printf("version %s\n", VERSION);

	err = abc_thing_new_from_string(ctx, "foo", &thing);
	if (err >= 0)
		abc_thing_unref(thing);

	abc_unref(ctx);
	return EXIT_SUCCESS;
}
