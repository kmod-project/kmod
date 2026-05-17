/*
 * libkmod-whitelist.c - module loading whitelist
 *
 * Out-of-tree addition. Intentionally self-contained: no kmod internal
 * headers, no callbacks into library infrastructure. Only libc.
 *
 * Semantics:
 *   - If WHITELIST_CONF does not exist, all modules are allowed (opt-in).
 *   - If the file exists but is empty, all modules are denied.
 *   - If the file has entries, only listed modules are allowed.
 *
 * Config file format (/etc/kmod/whitelist.conf):
 *   # comment lines start with '#'
 *   whitelist <modulename>
 *   whitelist <modulename>
 *   ...
 *
 * Module names must be in normalised form (underscores, not hyphens),
 * both in this file and as passed by the caller. kmod guarantees that
 * mod->name is always normalised before reaching whitelist_allowed().
 */

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#define WHITELIST_CONF "/etc/kmod/whitelist.conf"

/*
 * Internal state - loaded once per process lifetime.
 *
 * Note on thread safety: libkmod is designed around an explicit kmod_ctx
 * passed through every call, which would make it thread-safe (one context
 * per thread, no shared state) were it not for this file. These statics are
 * intentionally process-global and are NOT thread-safe. This is acceptable
 * because this implementation is scoped to the single-threaded kmod CLI
 * tools (modprobe, insmod, etc.) and is not intended for use in a
 * multithreaded embedding context.
 */
static char  **wl_entries = NULL;
static size_t  wl_count   = 0;

/*
 * 0 = not yet loaded
 * 1 = loaded, whitelist active (file was present)
 * 2 = file absent, allow everything
 */
static int wl_state = 0;

static void trim_trailing_comment(char *s)
{
	char *comment = strchr(s, '#');
	if (comment)
		*comment = '\0';
}

static void trim_trailing_whitespace(char *s)
{
	char *end;

	if (*s == '\0')
		return;

	end = s + strlen(s) - 1;
	while (end > s && isspace((unsigned char)*end))
		end--;
	*(end + 1) = '\0';
}

static char *skip_whitespace(char *p)
{
	while (isspace((unsigned char)*p))
		p++;
	return p;
}

static void whitelist_load(void)
{
	FILE *fp;
	char buf[512];
	char *line, *modname, *entry;
	size_t cap = 0;
	unsigned int linenum = 0;

	fp = fopen(WHITELIST_CONF, "r");
	if (fp == NULL) {
		if (errno == ENOENT) {
			/* File absent: whitelist inactive, allow everything. */
			wl_state = 2;
		} else {
			syslog(LOG_ERR,
			       "kmod whitelist: failed to open " WHITELIST_CONF
			       ": %m\n");
			/* Fail safe: deny all if config is unreadable. */
			wl_state = 1;
		}
		return;
	}

	wl_state = 1;

	while (fgets(buf, sizeof(buf), fp) != NULL) {
		line = buf;

		linenum++;
		trim_trailing_comment(line);
		trim_trailing_whitespace(line);
		line = skip_whitespace(line);

		if (line[0] == '\0')
			continue;

		/* Expect: "whitelist <modname>" */
		if (!(strncmp(line, "whitelist", 9) == 0 &&
		      isspace((unsigned char)line[9]))) {
			syslog(LOG_WARNING,
			       "kmod whitelist: " WHITELIST_CONF
			       ":%u: ignoring unrecognised line\n", linenum);
			continue;
		}

		modname = skip_whitespace(line + 9);
		if (modname[0] == '\0') {
			syslog(LOG_WARNING,
			       "kmod whitelist: " WHITELIST_CONF
			       ":%u: missing module name\n", linenum);
			continue;
		}

		/* Grow the entry array. */
		if (wl_count >= cap) {
			size_t newcap = (cap == 0) ? 16 : cap * 2;
			char **tmp = realloc(wl_entries,
					     newcap * sizeof(*wl_entries));
			if (tmp == NULL) {
				syslog(LOG_ERR,
				       "kmod whitelist: out of memory\n");
				break;
			}
			wl_entries = tmp;
			cap = newcap;
		}

		entry = strdup(modname);
		if (entry == NULL) {
			syslog(LOG_ERR, "kmod whitelist: out of memory\n");
			break;
		}

		wl_entries[wl_count++] = entry;
	}

	fclose(fp);
}

bool whitelist_allowed(const char *modname);

/*
 * whitelist_allowed - check whether a module may be loaded.
 *
 * Returns true  if the module is permitted (or whitelist is inactive).
 * Returns false if the whitelist is active and the module is not listed.
 *
 * This is the sole public symbol in this file. It is declared with a
 * forward declaration directly in libkmod-module.c to avoid polluting
 * any header.
 */
bool whitelist_allowed(const char *modname)
{
	size_t i;

	if (wl_state == 0)
		whitelist_load();

	/* File was absent: allow everything. */
	if (wl_state == 2)
		return true;

	for (i = 0; i < wl_count; i++) {
		if (strcmp(wl_entries[i], modname) == 0)
			return true;
	}

	syslog(LOG_NOTICE,
	       "kmod whitelist: module '%s' not in whitelist, denied\n",
	       modname);
	return false;
}
