/*
  libabc - something with abc

  Copyright (C) 2011 Someone <someone@example.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.¶

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _LIBABC_H_
#define _LIBABC_H_

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * abc_ctx
 *
 * library user context - reads the config and system
 * environment, user variables, allows custom logging
 */
struct abc_ctx;
struct abc_ctx *abc_ref(struct abc_ctx *ctx);
struct abc_ctx *abc_unref(struct abc_ctx *ctx);
int abc_new(struct abc_ctx **ctx);
void abc_set_log_fn(struct abc_ctx *ctx,
		  void (*log_fn)(struct abc_ctx *ctx,
				 int priority, const char *file, int line, const char *fn,
				 const char *format, va_list args));
int abc_get_log_priority(struct abc_ctx *ctx);
void abc_set_log_priority(struct abc_ctx *ctx, int priority);
void *abc_get_userdata(struct abc_ctx *ctx);
void abc_set_userdata(struct abc_ctx *ctx, void *userdata);

/*
 * abc_list
 *
 * access to abc generated lists
 */
struct abc_list_entry;
struct abc_list_entry *abc_list_entry_get_next(struct abc_list_entry *list_entry);
const char *abc_list_entry_get_name(struct abc_list_entry *list_entry);
const char *abc_list_entry_get_value(struct abc_list_entry *list_entry);
#define abc_list_entry_foreach(list_entry, first_entry) \
	for (list_entry = first_entry; \
	     list_entry != NULL; \
	     list_entry = abc_list_entry_get_next(list_entry))

/*
 * abc_thing
 *
 * access to things of abc
 */
struct abc_thing;
struct abc_thing *abc_thing_ref(struct abc_thing *thing);
struct abc_thing *abc_thing_unref(struct abc_thing *thing);
struct abc_ctx *abc_thing_get_ctx(struct abc_thing *thing);
int abc_thing_new_from_string(struct abc_ctx *ctx, const char *string, struct abc_thing **thing);
struct abc_list_entry *abc_thing_get_some_list_entry(struct abc_thing *thing);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
