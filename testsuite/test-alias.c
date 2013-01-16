/*
 * Copyright (C) 2012-2013  ProFUSION embedded systems
 * Copyright (C) 2012  Pedro Pedruzzi
 *
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libkmod.h>

#include "libkmod-util.h"
#include "testsuite.h"

static int alias_1(const struct test *t)
{
	static const char *input[] = {
		"test1234",
		"test[abcfoobar]2211",
		"bar[aaa][bbbb]sss",
		"kmod[p.b]lib",
		"[az]1234[AZ]",
		NULL,
	};

	char buf[PATH_MAX];
	size_t len;
	const char **alias;

	for (alias = input; *alias != NULL; alias++) {
		int ret;

		ret = alias_normalize(*alias, buf, &len);
		printf("input   %s\n", *alias);
		printf("return  %d\n", ret);

		if (ret == 0) {
			printf("len     %zu\n", len);
			printf("output  %s\n", buf);
		}

		printf("\n");
	}

	return EXIT_SUCCESS;
}
static DEFINE_TEST(alias_1,
	.description = "check if alias_normalize does the right thing",
	.config = {
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-alias/",
	},
	.need_spawn = true,
	.output = {
		.stdout = TESTSUITE_ROOTFS "test-alias/correct.txt",
	});

static const struct test *tests[] = {
	&salias_1,
	NULL,
};

TESTSUITE_MAIN(tests);
