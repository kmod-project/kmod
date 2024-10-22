// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2008  Alan Jenkins <alan-jenkins@tuffmail.co.uk>.
 * Copyright (C) 2011-2013  ProFUSION embedded systems
 */

#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/utsname.h>

#include <shared/array.h>
#include <shared/hash.h>
#include <shared/macro.h>
#include <shared/strbuf.h>
#include <shared/util.h>
#include <shared/scratchbuf.h>

#include <libkmod/libkmod-internal.h>

#undef ERR
#undef DBG

#include "kmod.h"

#define DEFAULT_VERBOSE LOG_WARNING
static int verbose = DEFAULT_VERBOSE;

static const char *module_directory = MODULE_DIRECTORY;
static const char CFG_BUILTIN_KEY[] = "built-in";
static const char CFG_EXTERNAL_KEY[] = "external";
static const char *const default_cfg_paths[] = {
	// clang-format off
	SYSCONFDIR "/depmod.d",
	"/run/depmod.d",
	"/usr/local/lib/depmod.d",
	DISTCONFDIR "/depmod.d",
	"/lib/depmod.d",
	NULL,
	// clang-format on
};

static const char cmdopts_s[] = "aAb:m:o:C:E:F:evnP:wVh";
static const struct option cmdopts[] = {
	{ "all", no_argument, 0, 'a' },
	{ "quick", no_argument, 0, 'A' },
	{ "basedir", required_argument, 0, 'b' },
	{ "moduledir", required_argument, 0, 'm' },
	{ "outdir", required_argument, 0, 'o' },
	{ "config", required_argument, 0, 'C' },
	{ "symvers", required_argument, 0, 'E' },
	{ "filesyms", required_argument, 0, 'F' },
	{ "errsyms", no_argument, 0, 'e' },
	{ "verbose", no_argument, 0, 'v' },
	{ "show", no_argument, 0, 'n' },
	{ "dry-run", no_argument, 0, 'n' },
	{ "symbol-prefix", required_argument, 0, 'P' },
	{ "warn", no_argument, 0, 'w' },
	{ "version", no_argument, 0, 'V' },
	{ "help", no_argument, 0, 'h' },
	{},
};

static void help(void)
{
	printf("Usage:\n"
	       "\t%s -[aA] [options] [forced_version]\n"
	       "\n"
	       "If no arguments (except options) are given, \"depmod -a\" is assumed\n"
	       "\n"
	       "depmod will output a dependency list suitable for the modprobe utility.\n"
	       "\n"
	       "Options:\n"
	       "\t-a, --all            Probe all modules\n"
	       "\t-A, --quick          Only does the work if there's a new module\n"
	       "\t-e, --errsyms        Report not supplied symbols\n"
	       "\t-n, --show           Write the dependency file on stdout only\n"
	       "\t-P, --symbol-prefix  Architecture symbol prefix\n"
	       "\t-C, --config=PATH    Read configuration from PATH\n"
	       "\t-v, --verbose        Enable verbose mode\n"
	       "\t-w, --warn           Warn on duplicates\n"
	       "\t-V, --version        show version\n"
	       "\t-h, --help           show this help\n"
	       "\n"
	       "The following options are useful for people managing distributions:\n"
	       "\t-b, --basedir=DIR    Root path (default: /).\n"
	       "\t-m, --moduledir=DIR  Module directory (default: " MODULE_DIRECTORY
	       ").\n"
	       "\t-o, --outdir=DIR     Output root path (default: same as <basedir>).\n"
	       "\t-F, --filesyms=FILE  Use the file instead of the\n"
	       "\t                     current kernel symbols.\n"
	       "\t-E, --symvers=FILE   Use Module.symvers file to check\n"
	       "\t                     symbol versions.\n",
	       program_invocation_short_name);
}

_printf_format_(1, 2) static inline void _show(const char *fmt, ...)
{
	va_list args;

	if (verbose <= DEFAULT_VERBOSE)
		return;

	va_start(args, fmt);
	vfprintf(stdout, fmt, args);
	fflush(stdout);
	va_end(args);
}
#define SHOW(...) _show(__VA_ARGS__)

/* see documentation in libkmod/libkmod-index.c */
#define INDEX_MAGIC 0xB007F457
#define INDEX_VERSION_MAJOR 0x0002
#define INDEX_VERSION_MINOR 0x0001
#define INDEX_VERSION ((INDEX_VERSION_MAJOR << 16) | INDEX_VERSION_MINOR)
#define INDEX_CHILDMAX 128

struct index_value {
	struct index_value *next;
	unsigned int priority;
	char value[0];
};

/* In-memory index (depmod only) */
struct index_node {
	char *prefix; /* path compression */
	struct index_value *values;
	unsigned char first; /* range of child nodes */
	unsigned char last;
	struct index_node *children[INDEX_CHILDMAX]; /* indexed by character */
};

/* Format of node offsets within index file */
enum node_offset {
	INDEX_NODE_FLAGS = 0xF0000000, /* Flags in high nibble */
	INDEX_NODE_PREFIX = 0x80000000,
	INDEX_NODE_VALUES = 0x40000000,
	INDEX_NODE_CHILDS = 0x20000000,

	INDEX_NODE_MASK = 0x0FFFFFFF, /* Offset value */
};

static noreturn void fatal_oom(void)
{
	ERR("out of memory\n");
	exit(EXIT_FAILURE);
}

static struct index_node *index_create(void)
{
	struct index_node *node;

	node = calloc(1, sizeof(struct index_node));
	if (node == NULL)
		return NULL;

	node->prefix = strdup("");
	if (node->prefix == NULL) {
		free(node);
		return NULL;
	}

	node->first = INDEX_CHILDMAX;

	return node;
}

static void index_values_free(struct index_value *values)
{
	while (values) {
		struct index_value *value = values;

		values = value->next;
		free(value);
	}
}

static void index_destroy(struct index_node *node)
{
	int c;

	for (c = node->first; c <= node->last; c++) {
		struct index_node *child = node->children[c];

		if (child)
			index_destroy(child);
	}
	index_values_free(node->values);
	free(node->prefix);
	free(node);
}

static void index__checkstring(const char *str)
{
	int i;

	for (i = 0; str[i]; i++) {
		int ch = str[i];

		if (ch >= INDEX_CHILDMAX)
			CRIT("Module index: bad character '%c'=0x%x - only 7-bit ASCII is supported:"
			     "\n%s\n",
			     (char)ch, (int)ch, str);
	}
}

static int index_add_value(struct index_value **values, const char *value,
			   unsigned int priority)
{
	struct index_value *v;
	int duplicate = 0;
	int len;

	/* report the presence of duplicate values */
	for (v = *values; v; v = v->next) {
		if (streq(v->value, value))
			duplicate = 1;
	}

	/* find position to insert value */
	while (*values && (*values)->priority < priority)
		values = &(*values)->next;

	len = strlen(value);
	v = calloc(1, sizeof(struct index_value) + len + 1);
	if (v == NULL)
		fatal_oom();
	v->next = *values;
	v->priority = priority;
	memcpy(v->value, value, len + 1);
	*values = v;

	return duplicate;
}

static int index_insert(struct index_node *node, const char *key, const char *value,
			unsigned int priority)
{
	int i = 0; /* index within str */
	int ch;

	index__checkstring(key);
	index__checkstring(value);

	while (1) {
		int j; /* index within node->prefix */

		/* Ensure node->prefix is a prefix of &str[i].
		   If it is not already, then we must split node. */
		for (j = 0; node->prefix[j]; j++) {
			ch = node->prefix[j];

			if (ch != key[i + j]) {
				char *prefix = node->prefix;
				struct index_node *n;

				/* New child is copy of node with prefix[j+1..N] */
				n = memdup(node, sizeof(struct index_node));
				if (n == NULL)
					fatal_oom();
				n->prefix = strdup(&prefix[j + 1]);
				if (n->prefix == NULL)
					fatal_oom();

				/* Parent has prefix[0..j], child at prefix[j] */
				memset(node, 0, sizeof(struct index_node));
				prefix[j] = '\0';
				node->prefix = prefix;
				node->first = ch;
				node->last = ch;
				node->children[ch] = n;

				break;
			}
		}
		/* j is now length of node->prefix */
		i += j;

		ch = key[i];
		if (ch == '\0')
			return index_add_value(&node->values, value, priority);

		if (!node->children[ch]) {
			struct index_node *child;

			if (ch < node->first)
				node->first = ch;
			if (ch > node->last)
				node->last = ch;
			node->children[ch] = calloc(1, sizeof(struct index_node));
			if (node->children[ch] == NULL)
				fatal_oom();

			child = node->children[ch];
			child->prefix = strdup(&key[i + 1]);
			if (child->prefix == NULL)
				fatal_oom();
			child->first = INDEX_CHILDMAX;
			index_add_value(&child->values, value, priority);

			return 0;
		}

		/* Descend into child node and continue */
		node = node->children[ch];
		i++;
	}
}

static int index__haschildren(const struct index_node *node)
{
	return node->first < INDEX_CHILDMAX;
}

/* Recursive post-order traversal

   Pre-order would make for better read-side buffering / readahead / caching.
   (post-order means you go backwards in the file as you descend the tree).
   However, index reading is already fast enough.
   Pre-order is simpler for writing, and depmod is already slow.
 */
static uint32_t index_write__node(const struct index_node *node, FILE *out)
{
	uint32_t *child_offs = NULL;
	int child_count = 0;
	long offset;

	if (!node)
		return 0;

	/* Write children and save their offsets */
	if (index__haschildren(node)) {
		const struct index_node *child;
		int i;

		child_count = node->last - node->first + 1;
		child_offs = malloc(child_count * sizeof(uint32_t));
		if (child_offs == NULL)
			fatal_oom();

		for (i = 0; i < child_count; i++) {
			child = node->children[node->first + i];
			child_offs[i] = htonl(index_write__node(child, out));
		}
	}

	/* Now write this node */
	offset = ftell(out);

	if (node->prefix[0]) {
		fputs(node->prefix, out);
		fputc('\0', out);
		offset |= INDEX_NODE_PREFIX;
	}

	if (child_count) {
		fputc(node->first, out);
		fputc(node->last, out);
		fwrite(child_offs, sizeof(uint32_t), child_count, out);
		offset |= INDEX_NODE_CHILDS;
	}

	free(child_offs);

	if (node->values) {
		const struct index_value *v;
		unsigned int value_count;
		uint32_t u;

		value_count = 0;
		for (v = node->values; v != NULL; v = v->next)
			value_count++;
		u = htonl(value_count);
		fwrite(&u, sizeof(u), 1, out);

		for (v = node->values; v != NULL; v = v->next) {
			u = htonl(v->priority);
			fwrite(&u, sizeof(u), 1, out);
			fputs(v->value, out);
			fputc('\0', out);
		}
		offset |= INDEX_NODE_VALUES;
	}

	return offset;
}

static void index_write(const struct index_node *node, FILE *out)
{
	long initial_offset, final_offset;
	uint32_t u;

	u = htonl(INDEX_MAGIC);
	fwrite(&u, sizeof(u), 1, out);
	u = htonl(INDEX_VERSION);
	fwrite(&u, sizeof(u), 1, out);

	/* Second word is reserved for the offset of the root node */
	initial_offset = ftell(out);
	assert(initial_offset >= 0);
	u = 0;
	fwrite(&u, sizeof(uint32_t), 1, out);

	/* Dump trie */
	u = htonl(index_write__node(node, out));

	/* Update first word */
	final_offset = ftell(out);
	assert(final_offset >= 0);
	(void)fseek(out, initial_offset, SEEK_SET);
	fwrite(&u, sizeof(uint32_t), 1, out);
	(void)fseek(out, final_offset, SEEK_SET);
}

/* configuration parsing **********************************************/
struct cfg_override {
	struct cfg_override *next;
	size_t len;
	char path[];
};

enum search_type {
	SEARCH_PATH,
	SEARCH_BUILTIN,
	SEARCH_EXTERNAL,
};

struct cfg_search {
	struct cfg_search *next;
	enum search_type type;
	size_t len;
	char path[];
};

struct cfg_external {
	struct cfg_external *next;
	size_t len;
	char path[];
};

struct cfg_exclude {
	struct cfg_exclude *next;
	char exclude_dir[];
};

struct cfg {
	const char *kversion;
	char dirname[PATH_MAX];
	size_t dirnamelen;
	char outdirname[PATH_MAX];
	size_t outdirnamelen;
	char sym_prefix;
	uint8_t check_symvers;
	uint8_t print_unknown;
	uint8_t warn_dups;
	struct cfg_override *overrides;
	struct cfg_search *searches;
	struct cfg_external *externals;
	struct cfg_exclude *excludes;
};

static enum search_type cfg_define_search_type(const char *path)
{
	if (streq(path, CFG_BUILTIN_KEY))
		return SEARCH_BUILTIN;
	if (streq(path, CFG_EXTERNAL_KEY))
		return SEARCH_EXTERNAL;
	return SEARCH_PATH;
}

static int cfg_search_add(struct cfg *cfg, const char *path)
{
	struct cfg_search *s;
	size_t len;
	enum search_type type;

	type = cfg_define_search_type(path);

	if (type != SEARCH_PATH)
		len = 0;
	else
		len = strlen(path) + 1;

	s = malloc(sizeof(struct cfg_search) + len);
	if (s == NULL) {
		ERR("search add: out of memory\n");
		return -ENOMEM;
	}
	s->type = type;
	if (type != SEARCH_PATH)
		s->len = 0;
	else {
		s->len = len - 1;
		memcpy(s->path, path, len);
	}

	DBG("search add: %s, search type=%hhu\n", path, (unsigned char)type);

	s->next = cfg->searches;
	cfg->searches = s;
	return 0;
}

static void cfg_search_free(struct cfg_search *s)
{
	free(s);
}

static int cfg_override_add(struct cfg *cfg, const char *modname, const char *subdir)
{
	struct cfg_override *o;
	size_t modnamelen = strlen(modname);
	size_t subdirlen = strlen(subdir);
	size_t i;

	o = malloc(sizeof(struct cfg_override) + subdirlen + 1 + modnamelen + 1);
	if (o == NULL) {
		ERR("override add: out of memory\n");
		return -ENOMEM;
	}
	memcpy(o->path, subdir, subdirlen);
	i = subdirlen;
	o->path[i] = '/';
	i++;

	memcpy(o->path + i, modname, modnamelen);
	i += modnamelen;
	o->path[i] = '\0'; /* no extension, so we can match .ko/.ko.gz */

	o->len = i;

	DBG("override add: %s\n", o->path);

	o->next = cfg->overrides;
	cfg->overrides = o;
	return 0;
}

static void cfg_override_free(struct cfg_override *o)
{
	free(o);
}

static int cfg_external_add(struct cfg *cfg, const char *path)
{
	struct cfg_external *ext;
	size_t len = strlen(path);

	ext = malloc(sizeof(struct cfg_external) + len + 1);
	if (ext == NULL) {
		ERR("external add: out of memory\n");
		return -ENOMEM;
	}

	strcpy(ext->path, path);
	ext->len = len;

	DBG("external add: %s\n", ext->path);

	ext->next = cfg->externals;
	cfg->externals = ext;
	return 0;
}

static void cfg_external_free(struct cfg_external *ext)
{
	free(ext);
}

static int cfg_exclude_add(struct cfg *cfg, const char *path)
{
	struct cfg_exclude *exc;
	size_t len = strlen(path);

	exc = malloc(sizeof(struct cfg_exclude) + len + 1);
	if (exc == NULL) {
		ERR("exclude add: out of memory\n");
		return -ENOMEM;
	}
	memcpy(exc->exclude_dir, path, len + 1);

	DBG("exclude add: %s\n", path);

	exc->next = cfg->excludes;
	cfg->excludes = exc;
	return 0;
}

static void cfg_exclude_free(struct cfg_exclude *exc)
{
	free(exc);
}

static int cfg_kernel_matches(const struct cfg *cfg, const char *pattern)
{
	regex_t re;
	int status;

	/* old style */
	if (streq(pattern, "*"))
		return 1;

	if (regcomp(&re, pattern, REG_EXTENDED | REG_NOSUB) != 0)
		return 0;

	status = regexec(&re, cfg->kversion, 0, NULL, 0);
	regfree(&re);

	return status == 0;
}

static int cfg_file_parse(struct cfg *cfg, const char *filename)
{
	char *line;
	FILE *fp;
	unsigned int linenum = 0;
	int err;

	fp = fopen(filename, "r");
	if (fp == NULL) {
		err = -errno;
		ERR("file parse %s: %m\n", filename);
		return err;
	}

	while ((line = freadline_wrapped(fp, &linenum)) != NULL) {
		char *cmd, *saveptr;

		if (line[0] == '\0' || line[0] == '#')
			goto done_next;

		cmd = strtok_r(line, "\t ", &saveptr);
		if (cmd == NULL)
			goto done_next;

		if (streq(cmd, "search")) {
			const char *sp;
			while ((sp = strtok_r(NULL, "\t ", &saveptr)) != NULL) {
				cfg_search_add(cfg, sp);
			}
		} else if (streq(cmd, "override")) {
			const char *modname = strtok_r(NULL, "\t ", &saveptr);
			const char *version = strtok_r(NULL, "\t ", &saveptr);
			const char *subdir = strtok_r(NULL, "\t ", &saveptr);

			if (modname == NULL || version == NULL || subdir == NULL)
				goto syntax_error;

			if (!cfg_kernel_matches(cfg, version)) {
				INF("%s:%u: override kernel did not match %s\n", filename,
				    linenum, version);
				goto done_next;
			}

			cfg_override_add(cfg, modname, subdir);
		} else if (streq(cmd, "external")) {
			const char *version = strtok_r(NULL, "\t ", &saveptr);
			const char *dir = strtok_r(NULL, "\t ", &saveptr);

			if (version == NULL || dir == NULL)
				goto syntax_error;

			if (!cfg_kernel_matches(cfg, version)) {
				INF("%s:%u: external directory did not match %s\n",
				    filename, linenum, version);
				goto done_next;
			}

			cfg_external_add(cfg, dir);
		} else if (streq(cmd, "exclude")) {
			const char *sp;
			while ((sp = strtok_r(NULL, "\t ", &saveptr)) != NULL) {
				cfg_exclude_add(cfg, sp);
			}
		} else if (streq(cmd, "include") || streq(cmd, "make_map_files")) {
			INF("%s:%u: command %s not implemented yet\n", filename, linenum,
			    cmd);
		} else {
syntax_error:
			ERR("%s:%u: ignoring bad line starting with '%s'\n", filename,
			    linenum, cmd);
		}

done_next:
		free(line);
	}

	fclose(fp);

	return 0;
}

static int cfg_files_filter_out(DIR *d, const char *dir, const char *name)
{
	size_t len = strlen(name);
	struct stat st;

	if (name[0] == '.')
		return 1;

	if (len < 6 || !streq(name + len - 5, ".conf")) {
		INF("All cfg files need .conf: %s/%s\n", dir, name);
		return 1;
	}

	fstatat(dirfd(d), name, &st, 0);
	if (S_ISDIR(st.st_mode)) {
		ERR("Directories inside directories are not supported: %s/%s\n", dir,
		    name);
		return 1;
	}

	return 0;
}

struct cfg_file {
	size_t dirlen;
	size_t namelen;
	const char *name;
	char path[];
};

static void cfg_file_free(struct cfg_file *f)
{
	free(f);
}

static int cfg_files_insert_sorted(struct cfg_file ***p_files, size_t *p_n_files,
				   const char *dir, const char *name)
{
	struct cfg_file **files, *f;
	size_t i, n_files, namelen, dirlen;
	void *tmp;

	dirlen = strlen(dir);
	if (name != NULL)
		namelen = strlen(name);
	else {
		name = basename(dir);
		namelen = strlen(name);
		dirlen -= namelen + 1;
	}

	n_files = *p_n_files;
	files = *p_files;
	for (i = 0; i < n_files; i++) {
		int cmp = strcmp(name, files[i]->name);
		if (cmp == 0) {
			DBG("Ignoring duplicate config file: %.*s/%s\n", (int)dirlen, dir,
			    name);
			return -EEXIST;
		} else if (cmp < 0)
			break;
	}

	f = malloc(sizeof(struct cfg_file) + dirlen + namelen + 2);
	if (f == NULL) {
		ERR("files insert sorted: out of memory\n");
		return -ENOMEM;
	}

	tmp = realloc(files, sizeof(struct cfg_file *) * (n_files + 1));
	if (tmp == NULL) {
		ERR("files insert sorted: out of memory\n");
		free(f);
		return -ENOMEM;
	}
	*p_files = files = tmp;

	if (i < n_files) {
		memmove(files + i + 1, files + i,
			sizeof(struct cfg_file *) * (n_files - i));
	}
	files[i] = f;

	f->dirlen = dirlen;
	f->namelen = namelen;
	f->name = f->path + dirlen + 1;
	memcpy(f->path, dir, dirlen);
	f->path[dirlen] = '/';
	memcpy(f->path + dirlen + 1, name, namelen);
	f->path[dirlen + 1 + namelen] = '\0';

	*p_n_files = n_files + 1;
	return 0;
}

/*
 * Insert configuration files ignoring duplicates
 */
static int cfg_files_list(struct cfg_file ***p_files, size_t *p_n_files, const char *path)
{
	struct dirent *dent;
	DIR *d;
	int err = 0;
	struct stat st;

	if (stat(path, &st) != 0) {
		err = -errno;
		DBG("could not stat '%s': %m\n", path);
		return err;
	}

	if (!S_ISDIR(st.st_mode)) {
		cfg_files_insert_sorted(p_files, p_n_files, path, NULL);
		return 0;
	}

	d = opendir(path);
	if (d == NULL) {
		ERR("files list %s: %m\n", path);
		return -EINVAL;
	}

	for (dent = readdir(d); dent != NULL; dent = readdir(d)) {
		if (cfg_files_filter_out(d, path, dent->d_name))
			continue;

		cfg_files_insert_sorted(p_files, p_n_files, path, dent->d_name);
	}

	closedir(d);
	DBG("parsed configuration files from %s\n", path);
	return err;
}

static int cfg_load(struct cfg *cfg, const char *const *cfg_paths)
{
	size_t i, n_files = 0;
	struct cfg_file **files = NULL;

	if (cfg_paths == NULL)
		cfg_paths = default_cfg_paths;

	for (i = 0; cfg_paths[i] != NULL; i++)
		cfg_files_list(&files, &n_files, cfg_paths[i]);

	for (i = 0; i < n_files; i++) {
		struct cfg_file *f = files[i];
		cfg_file_parse(cfg, f->path);
		cfg_file_free(f);
	}
	free(files);

	/* For backward compatibility add "updates" to the head of the search
	 * list here. But only if there was no "search" option specified.
	 */
	if (cfg->searches == NULL)
		cfg_search_add(cfg, "updates");

	return 0;
}

static void cfg_free(struct cfg *cfg)
{
	while (cfg->overrides) {
		struct cfg_override *tmp = cfg->overrides;
		cfg->overrides = cfg->overrides->next;
		cfg_override_free(tmp);
	}

	while (cfg->searches) {
		struct cfg_search *tmp = cfg->searches;
		cfg->searches = cfg->searches->next;
		cfg_search_free(tmp);
	}

	while (cfg->externals) {
		struct cfg_external *tmp = cfg->externals;
		cfg->externals = cfg->externals->next;
		cfg_external_free(tmp);
	}

	while (cfg->excludes) {
		struct cfg_exclude *tmp = cfg->excludes;
		cfg->excludes = cfg->excludes->next;
		cfg_exclude_free(tmp);
	}
}

/* depmod calculations ***********************************************/
struct vertex;
struct mod {
	struct kmod_module *kmod;
	char *path;
	const char *relpath; /* path relative to '$ROOT$MODULE_DIRECTORY/$VER/' */
	char *uncrelpath; /* same as relpath but ending in .ko */
	struct kmod_list *info_list;
	struct kmod_list *dep_sym_list;
	struct array deps; /* struct symbol */
	size_t baselen; /* points to start of basename/filename */
	size_t modnamesz;
	int sort_idx; /* sort index using modules.order */
	int dep_sort_idx; /* topological sort index */
	uint16_t idx; /* index in depmod->modules.array */
	uint16_t users; /* how many modules depend on this one */
	bool visited; /* helper field to report cycles */
	struct vertex *vertex; /* helper field to report cycles */
	char modname[];
};

struct symbol {
	struct mod *owner;
	uint64_t crc;
	char name[];
};

struct depmod {
	const struct cfg *cfg;
	struct kmod_ctx *ctx;
	struct array modules;
	struct hash *modules_by_uncrelpath;
	struct hash *modules_by_name;
	struct hash *symbols;
};

static void mod_free(struct mod *mod)
{
	DBG("free %p kmod=%p, path=%s\n", mod, mod->kmod, mod->path);
	array_free_array(&mod->deps);
	kmod_module_unref(mod->kmod);
	kmod_module_info_free_list(mod->info_list);
	kmod_module_dependency_symbols_free_list(mod->dep_sym_list);
	free(mod->uncrelpath);
	free(mod->path);
	free(mod);
}

static int mod_add_dependency(struct mod *mod, struct symbol *sym)
{
	int err;

	DBG("%s depends on %s %s\n", mod->path, sym->name,
	    sym->owner != NULL ? sym->owner->path : "(unknown)");

	if (sym->owner == NULL)
		return 0;

	err = array_append_unique(&mod->deps, sym->owner);
	if (err == -EEXIST)
		return 0;
	if (err < 0) {
		CRIT("failed to add symbol %s to module %s: %s\n", sym->name, mod->path,
		     strerror(-err));
		return err;
	}

	sym->owner->users++;
	SHOW("%s needs \"%s\": %s\n", mod->path, sym->name, sym->owner->path);
	return 0;
}

static void symbol_free(void *data)
{
	struct symbol *sym = data;

	DBG("free %p sym=%s, owner=%p %s\n", sym, sym->name, sym->owner,
	    sym->owner != NULL ? sym->owner->path : "");
	free(sym);
}

static int depmod_init(struct depmod *depmod, struct cfg *cfg, struct kmod_ctx *ctx)
{
	int err = 0;

	depmod->cfg = cfg;
	depmod->ctx = ctx;

	array_init(&depmod->modules, 128);

	depmod->modules_by_uncrelpath = hash_new(512, NULL);
	if (depmod->modules_by_uncrelpath == NULL) {
		err = -errno;
		goto modules_by_uncrelpath_failed;
	}

	depmod->modules_by_name = hash_new(512, NULL);
	if (depmod->modules_by_name == NULL) {
		err = -errno;
		goto modules_by_name_failed;
	}

	depmod->symbols = hash_new(2048, symbol_free);
	if (depmod->symbols == NULL) {
		err = -errno;
		goto symbols_failed;
	}

	return 0;

symbols_failed:
	hash_free(depmod->modules_by_name);
modules_by_name_failed:
	hash_free(depmod->modules_by_uncrelpath);
modules_by_uncrelpath_failed:
	return err;
}

static void depmod_shutdown(struct depmod *depmod)
{
	size_t i;

	hash_free(depmod->symbols);

	hash_free(depmod->modules_by_uncrelpath);

	hash_free(depmod->modules_by_name);

	for (i = 0; i < depmod->modules.count; i++)
		mod_free(depmod->modules.array[i]);
	array_free_array(&depmod->modules);

	kmod_unref(depmod->ctx);
}

static int depmod_module_add(struct depmod *depmod, struct kmod_module *kmod)
{
	const struct cfg *cfg = depmod->cfg;
	const char *modname, *lastslash;
	size_t modnamesz;
	struct mod *mod;
	int err;

	modname = kmod_module_get_name(kmod);
	modnamesz = strlen(modname) + 1;

	mod = calloc(1, sizeof(struct mod) + modnamesz);
	if (mod == NULL)
		return -ENOMEM;
	mod->kmod = kmod;
	mod->sort_idx = depmod->modules.count + 1;
	mod->dep_sort_idx = INT32_MAX;
	memcpy(mod->modname, modname, modnamesz);
	mod->modnamesz = modnamesz;

	array_init(&mod->deps, 4);

	mod->path = strdup(kmod_module_get_path(kmod));
	if (mod->path == NULL) {
		free(mod);
		return -ENOMEM;
	}
	lastslash = strrchr(mod->path, '/');
	mod->baselen = lastslash - mod->path;
	if (strncmp(mod->path, cfg->dirname, cfg->dirnamelen) == 0 &&
	    mod->path[cfg->dirnamelen] == '/')
		mod->relpath = mod->path + cfg->dirnamelen + 1;
	else
		mod->relpath = NULL;

	err = hash_add_unique(depmod->modules_by_name, mod->modname, mod);
	if (err < 0) {
		ERR("hash_add_unique %s: %s\n", mod->modname, strerror(-err));
		goto fail;
	}

	if (mod->relpath != NULL) {
		size_t uncrelpathlen = lastslash - mod->relpath + modnamesz +
				       strlen(KMOD_EXTENSION_UNCOMPRESSED);
		mod->uncrelpath = memdup(mod->relpath, uncrelpathlen + 1);
		if (mod->uncrelpath == NULL) {
			err = -ENOMEM;
			hash_del(depmod->modules_by_name, mod->modname);
			goto fail;
		}
		mod->uncrelpath[uncrelpathlen] = '\0';
		err = hash_add_unique(depmod->modules_by_uncrelpath, mod->uncrelpath, mod);
		if (err < 0) {
			ERR("hash_add_unique %s: %s\n", mod->uncrelpath, strerror(-err));
			hash_del(depmod->modules_by_name, mod->modname);
			goto fail;
		}
	}

	DBG("add %p kmod=%p, path=%s\n", mod, kmod, mod->path);

	return 0;

fail:
	free(mod->uncrelpath);
	free(mod);
	return err;
}

static int depmod_module_del(struct depmod *depmod, struct mod *mod)
{
	DBG("del %p kmod=%p, path=%s\n", mod, mod->kmod, mod->path);

	if (mod->uncrelpath != NULL)
		hash_del(depmod->modules_by_uncrelpath, mod->uncrelpath);

	hash_del(depmod->modules_by_name, mod->modname);

	mod_free(mod);
	return 0;
}

static const char *search_to_string(const struct cfg_search *s)
{
	switch (s->type) {
	case SEARCH_EXTERNAL:
		return "external";
	case SEARCH_BUILTIN:
		return "built-in";
	default:
		return s->path;
	}
}

static bool depmod_is_path_starts_with(const char *path, size_t pathlen,
				       const char *prefix, size_t prefix_len)
{
	if (pathlen <= prefix_len)
		return false;
	if (path[prefix_len] != '/')
		return false;
	if (memcmp(path, prefix, prefix_len) != 0)
		return false;

	return true;
}

/* returns if existing module @mod is higher priority than newpath.
 * note this is the inverse of module-init-tools is_higher_priority()
 */
static int depmod_module_is_higher_priority(const struct depmod *depmod,
					    const struct mod *mod, size_t baselen,
					    size_t namelen, size_t modnamelen,
					    const char *newpath)
{
	const struct cfg *cfg = depmod->cfg;
	const struct cfg_override *ov;
	const struct cfg_search *se;
	const struct cfg_external *ext;

	/* baselen includes the last '/' and mod->baselen doesn't. So it's
	 * actually correct to use modnamelen in the first and modnamesz in
	 * the latter */
	size_t newlen = baselen + modnamelen;
	size_t oldlen = mod->baselen + mod->modnamesz;
	const char *oldpath = mod->path;
	int i, bprio = -1, oldprio = -1, newprio = -1;
	size_t relnewlen = 0;
	size_t reloldlen = 0;
	const char *relnewpath = NULL;
	const char *reloldpath = NULL;

	DBG("comparing priorities of %s and %s\n", oldpath, newpath);

	if (strncmp(newpath, cfg->dirname, cfg->dirnamelen) == 0) {
		relnewpath = newpath + cfg->dirnamelen + 1;
		relnewlen = newlen - (cfg->dirnamelen + 1);
	}
	if (strncmp(oldpath, cfg->dirname, cfg->dirnamelen) == 0) {
		reloldpath = oldpath + cfg->dirnamelen + 1;
		reloldlen = oldlen - (cfg->dirnamelen + 1);
	}

	for (ov = cfg->overrides; ov != NULL; ov = ov->next) {
		DBG("override %s\n", ov->path);
		if (relnewlen == ov->len && memcmp(ov->path, relnewpath, relnewlen) == 0)
			return 0;
		if (reloldlen == ov->len && memcmp(ov->path, reloldpath, reloldlen) == 0)
			return 1;
	}

	for (i = 0, se = cfg->searches; se != NULL; se = se->next, i++) {
		DBG("search %s\n", search_to_string(se));
		if (se->type == SEARCH_BUILTIN)
			bprio = i;
		else if (se->type == SEARCH_EXTERNAL) {
			for (ext = cfg->externals; ext != NULL; ext = ext->next, i++) {
				if (depmod_is_path_starts_with(newpath, newlen, ext->path,
							       ext->len))
					newprio = i;
				if (depmod_is_path_starts_with(oldpath, oldlen, ext->path,
							       ext->len))
					oldprio = i;
			}
		} else if (relnewlen > se->len && relnewpath[se->len] == '/' &&
			   memcmp(se->path, relnewpath, se->len) == 0)
			newprio = i;
		else if (reloldlen > se->len && reloldpath[se->len] == '/' &&
			 memcmp(se->path, reloldpath, se->len) == 0)
			oldprio = i;
	}

	if (newprio < 0)
		newprio = bprio;
	if (oldprio < 0)
		oldprio = bprio;

	DBG("priorities: built-in: %d, old: %d, new: %d\n", bprio, oldprio, newprio);

	return newprio <= oldprio;
}

static int depmod_modules_search_file(struct depmod *depmod, size_t baselen,
				      size_t namelen, const char *path)
{
	struct kmod_module *kmod;
	struct mod *mod;
	const char *relpath;
	char modname[PATH_MAX];
	size_t modnamelen;
	int err;

	if (!path_ends_with_kmod_ext(path + baselen, namelen))
		return 0;

	if (path_to_modname(path, modname, &modnamelen) == NULL) {
		ERR("could not get modname from path %s\n", path);
		return -EINVAL;
	}

	relpath = path + depmod->cfg->dirnamelen + 1;
	DBG("try %s (%s)\n", relpath, modname);

	mod = hash_find(depmod->modules_by_name, modname);
	if (mod == NULL)
		goto add;

	if (depmod_module_is_higher_priority(depmod, mod, baselen, namelen, modnamelen,
					     path)) {
		DBG("Ignored lower priority: %s, higher: %s\n", path, mod->path);
		return 0;
	}

	DBG("Replace lower priority %s with new module %s\n", mod->relpath, relpath);
	err = depmod_module_del(depmod, mod);
	if (err < 0) {
		ERR("could not del module %s: %s\n", mod->path, strerror(-err));
		return err;
	}

add:
	err = kmod_module_new_from_path(depmod->ctx, path, &kmod);
	if (err < 0) {
		ERR("could not create module %s: %s\n", path, strerror(-err));
		return err;
	}

	err = depmod_module_add(depmod, kmod);
	if (err < 0) {
		ERR("could not add module %s: %s\n", path, strerror(-err));
		kmod_module_unref(kmod);
		return err;
	}
	return 0;
}

static bool should_exclude_dir(const struct cfg *cfg, const char *name)
{
	struct cfg_exclude *exc;

	if (name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0')))
		return true;

	if (streq(name, "build") || streq(name, "source"))
		return true;

	for (exc = cfg->excludes; exc != NULL; exc = exc->next) {
		if (streq(name, exc->exclude_dir))
			return true;
	}

	return false;
}

static int depmod_modules_search_dir(struct depmod *depmod, DIR *d, size_t baselen,
				     struct scratchbuf *s_path)
{
	struct dirent *de;
	int err = 0, dfd = dirfd(d);
	char *path;

	while ((de = readdir(d)) != NULL) {
		const char *name = de->d_name;
		size_t namelen;
		uint8_t is_dir;

		if (should_exclude_dir(depmod->cfg, name))
			continue;

		namelen = strlen(name);
		if (scratchbuf_alloc(s_path, baselen + namelen + 2) < 0) {
			err = -ENOMEM;
			ERR("No memory\n");
			continue;
		}

		path = scratchbuf_str(s_path);
		memcpy(path + baselen, name, namelen + 1);

		if (de->d_type == DT_REG)
			is_dir = 0;
		else if (de->d_type == DT_DIR)
			is_dir = 1;
		else {
			struct stat st;
			if (fstatat(dfd, name, &st, 0) < 0) {
				ERR("fstatat(%d, %s): %m\n", dfd, name);
				continue;
			} else if (S_ISREG(st.st_mode))
				is_dir = 0;
			else if (S_ISDIR(st.st_mode))
				is_dir = 1;
			else {
				ERR("unsupported file type %s: %o\n", path,
				    st.st_mode & S_IFMT);
				continue;
			}
		}

		if (is_dir) {
			int fd;
			DIR *subdir;
			fd = openat(dfd, name, O_RDONLY);
			if (fd < 0) {
				ERR("openat(%d, %s, O_RDONLY): %m\n", dfd, name);
				continue;
			}
			subdir = fdopendir(fd);
			if (subdir == NULL) {
				ERR("fdopendir(%d): %m\n", fd);
				close(fd);
				continue;
			}
			path[baselen + namelen] = '/';
			path[baselen + namelen + 1] = '\0';
			err = depmod_modules_search_dir(depmod, subdir,
							baselen + namelen + 1, s_path);
			closedir(subdir);
		} else {
			err = depmod_modules_search_file(depmod, baselen, namelen, path);
		}

		if (err < 0) {
			path[baselen + namelen] = '\0';
			ERR("failed %s: %s\n", path, strerror(-err));
			err = 0; /* ignore errors */
		}
	}
	return err;
}

static int depmod_modules_search_path(struct depmod *depmod, const char *path)
{
	char buf[256];
	_cleanup_(scratchbuf_release) struct scratchbuf s_path_buf =
		SCRATCHBUF_INITIALIZER(buf);
	char *path_buf;
	DIR *d;
	size_t baselen;
	int err;

	d = opendir(path);
	if (d == NULL) {
		err = -errno;
		ERR("could not open directory %s: %m\n", path);
		return err;
	}

	baselen = strlen(path);

	if (scratchbuf_alloc(&s_path_buf, baselen + 2) < 0) {
		err = -ENOMEM;
		goto out;
	}
	path_buf = scratchbuf_str(&s_path_buf);

	memcpy(path_buf, path, baselen);
	path_buf[baselen] = '/';
	baselen++;
	path_buf[baselen] = '\0';

	err = depmod_modules_search_dir(depmod, d, baselen, &s_path_buf);
out:
	closedir(d);
	return err;
}

static int depmod_modules_search(struct depmod *depmod)
{
	int err;
	struct cfg_external *ext;

	err = depmod_modules_search_path(depmod, depmod->cfg->dirname);
	if (err < 0)
		return err;

	for (ext = depmod->cfg->externals; ext != NULL; ext = ext->next) {
		err = depmod_modules_search_path(depmod, ext->path);
		if (err < 0 && err == -ENOENT)
			/* ignore external dir absence */
			continue;
	}

	return 0;
}

static int mod_cmp(const void *pa, const void *pb)
{
	const struct mod *a = *(const struct mod **)pa;
	const struct mod *b = *(const struct mod **)pb;
	return a->sort_idx - b->sort_idx;
}

static int depmod_modules_build_array(struct depmod *depmod)
{
	struct hash_iter module_iter;
	const void *v;
	int err;

	hash_iter_init(depmod->modules_by_name, &module_iter);
	while (hash_iter_next(&module_iter, NULL, &v)) {
		struct mod *mod = (struct mod *)v;
		mod->idx = depmod->modules.count;
		err = array_append(&depmod->modules, mod);
		if (err < 0)
			return err;
	}
	if (depmod->modules.count >= UINT16_MAX)
		return -ERANGE;

	return 0;
}

static FILE *dfdopen(const char *dname, const char *filename, int flags, const char *mode)
{
	int fd, dfd;
	FILE *ret;

	dfd = open(dname, O_RDONLY);
	if (dfd < 0) {
		WRN("could not open directory %s: %m\n", dname);
		return NULL;
	}

	fd = openat(dfd, filename, flags);
	if (fd < 0) {
		WRN("could not open %s at %s: %m\n", filename, dname);
		ret = NULL;
	} else {
		ret = fdopen(fd, mode);
		if (!ret) {
			WRN("could not associate stream with %s: %m\n", filename);
			close(fd);
		}
	}
	close(dfd);
	return ret;
}

static void depmod_modules_sort(struct depmod *depmod)
{
	char line[PATH_MAX];
	const char *order_file = "modules.order";
	FILE *fp;
	size_t idx = 0;
	// all sorted modules shall have precedence
	int i = -(int)depmod->modules.count;

	fp = dfdopen(depmod->cfg->dirname, order_file, O_RDONLY, "r");
	if (fp == NULL)
		return;

	while (fgets(line, sizeof(line), fp) != NULL) {
		struct mod *mod;
		size_t len = strlen(line);
		idx++;
		if (len == 0)
			continue;
		if (line[len - 1] != '\n') {
			ERR("%s/%s:%zu corrupted line misses '\\n'\n",
			    depmod->cfg->dirname, order_file, idx);
			goto corrupted;
		}
		line[len - 1] = '\0';
		mod = hash_find(depmod->modules_by_uncrelpath, line);
		if (mod == NULL || mod->sort_idx < 0)
			continue;
		mod->sort_idx = i++;
	}

	array_sort(&depmod->modules, mod_cmp);
	for (idx = 0; idx < depmod->modules.count; idx++) {
		struct mod *m = depmod->modules.array[idx];
		m->idx = idx;
	}

corrupted:
	fclose(fp);
}

static int depmod_symbol_add(struct depmod *depmod, const char *name, bool prefix_skipped,
			     uint64_t crc, const struct mod *owner)
{
	size_t namelen;
	int err;
	struct symbol *sym;

	if (!prefix_skipped && (name[0] == depmod->cfg->sym_prefix))
		name++;

	namelen = strlen(name) + 1;
	sym = malloc(sizeof(struct symbol) + namelen);
	if (sym == NULL)
		return -ENOMEM;

	sym->owner = (struct mod *)owner;
	sym->crc = crc;
	memcpy(sym->name, name, namelen);

	err = hash_add(depmod->symbols, sym->name, sym);
	if (err < 0) {
		free(sym);
		return err;
	}

	DBG("add %p sym=%s, owner=%p %s\n", sym, sym->name, owner,
	    owner != NULL ? owner->path : "");

	return 0;
}

static struct symbol *depmod_symbol_find(const struct depmod *depmod, const char *name)
{
	if (name[0] == '.') /* PPC64 needs this: .foo == foo */
		name++;
	if (name[0] == depmod->cfg->sym_prefix)
		name++;
	return hash_find(depmod->symbols, name);
}

static int depmod_load_modules(struct depmod *depmod)
{
	struct mod **itr, **itr_end;

	DBG("load symbols (%zd modules)\n", depmod->modules.count);

	itr = (struct mod **)depmod->modules.array;
	itr_end = itr + depmod->modules.count;
	for (; itr < itr_end; itr++) {
		struct mod *mod = *itr;
		struct kmod_list *l, *list = NULL;
		int err = kmod_module_get_symbols(mod->kmod, &list);
		if (err < 0) {
			if (err == -ENODATA)
				DBG("ignoring %s: no symbols\n", mod->path);
			else
				ERR("failed to load symbols from %s: %s\n", mod->path,
				    strerror(-err));
			goto load_info;
		}
		kmod_list_foreach(l, list) {
			const char *name = kmod_module_symbol_get_symbol(l);
			uint64_t crc = kmod_module_symbol_get_crc(l);
			depmod_symbol_add(depmod, name, false, crc, mod);
		}
		kmod_module_symbols_free_list(list);

load_info:
		kmod_module_get_info(mod->kmod, &mod->info_list);
		kmod_module_get_dependency_symbols(mod->kmod, &mod->dep_sym_list);
		kmod_module_unref(mod->kmod);
		mod->kmod = NULL;
	}

	DBG("loaded symbols (%zd modules, %u symbols)\n", depmod->modules.count,
	    hash_get_count(depmod->symbols));

	return 0;
}

static int depmod_load_module_dependencies(struct depmod *depmod, struct mod *mod)
{
	const struct cfg *cfg = depmod->cfg;
	struct kmod_list *l;
	int ret = 0;

	DBG("do dependencies of %s\n", mod->path);
	kmod_list_foreach(l, mod->dep_sym_list) {
		const char *name = kmod_module_dependency_symbol_get_symbol(l);
		uint64_t crc = kmod_module_dependency_symbol_get_crc(l);
		int bindtype = kmod_module_dependency_symbol_get_bind(l);
		struct symbol *sym = depmod_symbol_find(depmod, name);
		uint8_t is_weak = bindtype == KMOD_SYMBOL_WEAK;
		int err;

		if (sym == NULL) {
			DBG("%s needs (%c) unknown symbol %s\n", mod->path, bindtype,
			    name);
			if (cfg->print_unknown && !is_weak)
				WRN("%s needs unknown symbol %s\n", mod->path, name);
			continue;
		}

		if (cfg->check_symvers && sym->crc != crc && !is_weak) {
			DBG("symbol %s (%#" PRIx64 ") module %s (%#" PRIx64 ")\n",
			    sym->name, sym->crc, mod->path, crc);
			if (cfg->print_unknown)
				WRN("%s disagrees about version of symbol %s\n",
				    mod->path, name);
		}

		err = mod_add_dependency(mod, sym);
		if (err < 0)
			ret = err;
	}

	return ret;
}

static int depmod_load_dependencies(struct depmod *depmod)
{
	struct mod **itr, **itr_end;
	int ret = 0;

	DBG("load dependencies (%zd modules, %u symbols)\n", depmod->modules.count,
	    hash_get_count(depmod->symbols));

	itr = (struct mod **)depmod->modules.array;
	itr_end = itr + depmod->modules.count;
	for (; itr < itr_end; itr++) {
		struct mod *mod = *itr;
		int err;

		if (mod->dep_sym_list == NULL) {
			DBG("ignoring %s: no dependency symbols\n", mod->path);
			continue;
		}

		err = depmod_load_module_dependencies(depmod, mod);
		if (err < 0)
			ret = err;
	}

	DBG("loaded dependencies (%zd modules, %u symbols)\n", depmod->modules.count,
	    hash_get_count(depmod->symbols));

	return ret;
}

static int dep_cmp(const void *pa, const void *pb)
{
	const struct mod *a = *(const struct mod **)pa;
	const struct mod *b = *(const struct mod **)pb;
	return a->dep_sort_idx - b->dep_sort_idx;
}

static void depmod_sort_dependencies(struct depmod *depmod)
{
	struct mod **itr, **itr_end;
	itr = (struct mod **)depmod->modules.array;
	itr_end = itr + depmod->modules.count;
	for (; itr < itr_end; itr++) {
		struct mod *m = *itr;
		if (m->deps.count > 1)
			array_sort(&m->deps, dep_cmp);
	}
}

struct vertex {
	struct vertex *parent;
	struct mod *mod;
};

static struct vertex *vertex_new(struct mod *mod, struct vertex *parent)
{
	struct vertex *v;

	v = malloc(sizeof(*v));
	if (v == NULL)
		return NULL;

	v->parent = parent;
	v->mod = mod;
	return v;
}

static void depmod_list_remove_data(struct kmod_list **list, void *data)
{
	struct kmod_list *l;

	l = kmod_list_remove_data(*list, data);
	*list = l;
}

static int depmod_report_one_cycle(struct depmod *depmod, struct vertex *vertex,
				   struct kmod_list **roots, struct hash *loop_set)
{
	const char sep[] = " -> ";
	size_t sz;
	char *buf;
	struct array reverse;
	int i;
	int n;
	struct vertex *v;
	int rc;

	array_init(&reverse, 3);

	sz = 0;
	for (v = vertex->parent, n = 0; v != NULL; v = v->parent, n++) {
		sz += v->mod->modnamesz - 1;
		rc = array_append(&reverse, v);
		if (rc < 0)
			return rc;
		rc = hash_add(loop_set, v->mod->modname, NULL);
		if (rc != 0)
			return rc;
		/* the hash will be freed where created */
	}
	sz += vertex->mod->modnamesz - 1;

	buf = malloc(sz + n * strlen(sep) + 1);

	sz = 0;
	for (i = reverse.count - 1; i >= 0; i--) {
		size_t len;

		v = reverse.array[i];

		len = v->mod->modnamesz - 1;
		memcpy(buf + sz, v->mod->modname, len);
		sz += len;
		strcpy(buf + sz, sep);
		sz += strlen(sep);

		depmod_list_remove_data(roots, v->mod);
	}
	strcpy(buf + sz, vertex->mod->modname);
	ERR("Cycle detected: %s\n", buf);

	free(buf);
	array_free_array(&reverse);

	return 0;
}

static int depmod_report_cycles_from_root(struct depmod *depmod, struct mod *root_mod,
					  struct kmod_list **roots, void **stack,
					  size_t stack_size, struct hash *loop_set)
{
	struct kmod_list *free_list = NULL; /* struct vertex */
	struct kmod_list *l;
	struct vertex *root;
	struct vertex *vertex;
	struct vertex *v;
	struct mod *m;
	struct mod **itr, **itr_end;
	size_t is;
	int ret = -ENOMEM;

	root = vertex_new(root_mod, NULL);
	if (root == NULL) {
		ERR("No memory to report cycles\n");
		goto out;
	}

	l = kmod_list_append(free_list, root);
	if (l == NULL) {
		ERR("No memory to report cycles\n");
		goto out;
	}
	free_list = l;

	is = 0;
	stack[is++] = (void *)root;

	while (is > 0) {
		vertex = stack[--is];
		m = vertex->mod;
		/*
		 * because of the topological sort we can start only
		 * from part of a loop or from a branch after a loop
		 */
		if (m->visited && m == root->mod) {
			int rc;
			rc = depmod_report_one_cycle(depmod, vertex, roots, loop_set);
			if (rc != 0) {
				ret = rc;
				goto out;
			}
			continue;
		}

		m->visited = true;
		if (m->deps.count == 0) {
			/*
			 * boundary condition: if there is more than one
			 * single node branch (not a loop), it is
			 * recognized as a loop by the code above:
			 * m->visited because more then one,
			 * m == root->mod since it is a single node.
			 * So, prevent deeping into the branch second
			 * time.
			 */
			depmod_list_remove_data(roots, m);

			continue;
		}

		itr = (struct mod **)m->deps.array;
		itr_end = itr + m->deps.count;
		for (; itr < itr_end; itr++) {
			struct mod *dep = *itr;
			v = vertex_new(dep, vertex);
			if (v == NULL) {
				ERR("No memory to report cycles\n");
				goto out;
			}
			assert(is < stack_size);
			stack[is++] = v;

			l = kmod_list_append(free_list, v);
			if (l == NULL) {
				ERR("No memory to report cycles\n");
				goto out;
			}
			free_list = l;
		}
	}
	ret = 0;

out:
	while (free_list) {
		v = free_list->data;
		l = kmod_list_remove(free_list);
		free_list = l;
		free(v);
	}

	return ret;
}

static void depmod_report_cycles(struct depmod *depmod, uint16_t n_mods, uint16_t *users)
{
	int num_cyclic = 0;
	struct kmod_list *roots = NULL; /* struct mod */
	struct kmod_list *l;
	size_t n_r; /* local n_roots */
	int i;
	int err;
	_cleanup_free_ void **stack = NULL;
	struct mod *m;
	struct mod *root;
	struct hash *loop_set;

	for (i = 0, n_r = 0; i < n_mods; i++) {
		if (users[i] <= 0)
			continue;
		m = depmod->modules.array[i];
		l = kmod_list_append(roots, m);
		if (l == NULL) {
			ERR("No memory to report cycles\n");
			goto out_list;
		}
		roots = l;
		n_r++;
	}

	stack = malloc(n_r * sizeof(void *));
	if (stack == NULL) {
		ERR("No memory to report cycles\n");
		goto out_list;
	}

	loop_set = hash_new(16, NULL);
	if (loop_set == NULL) {
		ERR("No memory to report cycles\n");
		goto out_list;
	}

	while (roots != NULL) {
		root = roots->data;
		l = kmod_list_remove(roots);
		roots = l;
		err = depmod_report_cycles_from_root(depmod, root, &roots, stack, n_r,
						     loop_set);
		if (err < 0)
			goto out_hash;
	}

	num_cyclic = hash_get_count(loop_set);
	ERR("Found %d modules in dependency cycles!\n", num_cyclic);

out_hash:
	hash_free(loop_set);
out_list:
	while (roots != NULL) {
		/* no need to free data, come from outside */
		roots = kmod_list_remove(roots);
	}
}

static int depmod_calculate_dependencies(struct depmod *depmod)
{
	const struct mod **itrm;
	uint16_t *users, *roots, *sorted;
	uint16_t i, n_roots = 0, n_sorted = 0, n_mods = depmod->modules.count;
	int ret = 0;

	users = malloc(sizeof(uint16_t) * n_mods * 3);
	if (users == NULL)
		return -ENOMEM;
	roots = users + n_mods;
	sorted = roots + n_mods;

	DBG("calculate dependencies and ordering (%hu modules)\n", n_mods);

	assert(depmod->modules.count < UINT16_MAX);

	/* populate modules users (how many modules uses it) */
	itrm = (const struct mod **)depmod->modules.array;
	for (i = 0; i < n_mods; i++, itrm++) {
		const struct mod *m = *itrm;
		users[i] = m->users;
		if (users[i] == 0) {
			roots[n_roots] = i;
			n_roots++;
		}
	}

	/* topological sort (outputs modules without users first) */
	while (n_roots > 0) {
		const struct mod **itr_dst, **itr_dst_end;
		struct mod *src;
		uint16_t src_idx = roots[--n_roots];

		src = depmod->modules.array[src_idx];
		src->dep_sort_idx = n_sorted;
		sorted[n_sorted] = src_idx;
		n_sorted++;

		if (src->deps.count == 0)
			continue;

		itr_dst = (const struct mod **)src->deps.array;
		itr_dst_end = itr_dst + src->deps.count;
		for (; itr_dst < itr_dst_end; itr_dst++) {
			const struct mod *dst = *itr_dst;
			uint16_t dst_idx = dst->idx;
			assert(users[dst_idx] > 0);
			users[dst_idx]--;
			if (users[dst_idx] == 0) {
				roots[n_roots] = dst_idx;
				n_roots++;
			}
		}
	}

	if (n_sorted < n_mods) {
		depmod_report_cycles(depmod, n_mods, users);
		ret = -EINVAL;
		goto exit;
	}

	depmod_sort_dependencies(depmod);

	DBG("calculated dependencies and ordering (%hu modules)\n", n_mods);

exit:
	free(users);
	return ret;
}

static int depmod_load(struct depmod *depmod)
{
	int err;

	err = depmod_load_modules(depmod);
	if (err < 0)
		return err;

	err = depmod_load_dependencies(depmod);
	if (err < 0)
		return err;

	err = depmod_calculate_dependencies(depmod);
	if (err < 0)
		return err;

	return 0;
}

static int mod_fill_all_unique_dependencies(const struct mod *mod, struct array *deps)
{
	size_t i;
	int err = 0;

	for (i = 0; i < mod->deps.count; i++) {
		const struct mod *d = mod->deps.array[i];

		err = array_append_unique(deps, d);
		if (err < 0) {
			if (err == -EEXIST) {
				err = 0;
				continue;
			}
			return err;
		}

		err = mod_fill_all_unique_dependencies(d, deps);
		if (err < 0)
			break;
	}
	return err;
}

static bool mod_get_all_sorted_dependencies(const struct mod *mod, struct array *deps)
{
	deps->count = 0;
	if (mod_fill_all_unique_dependencies(mod, deps) < 0)
		return false;

	if (deps->count > 1)
		array_sort(deps, dep_cmp);
	return true;
}

static inline const char *mod_get_compressed_path(const struct mod *mod)
{
	if (mod->relpath != NULL)
		return mod->relpath;
	return mod->path;
}

static int output_deps(struct depmod *depmod, FILE *out)
{
	size_t i;
	struct array deps;

	array_init(&deps, 64);

	for (i = 0; i < depmod->modules.count; i++) {
		const struct mod *mod = depmod->modules.array[i];
		const char *p = mod_get_compressed_path(mod);
		size_t j;

		fprintf(out, "%s:", p);

		if (mod->deps.count == 0)
			goto end;

		if (!mod_get_all_sorted_dependencies(mod, &deps)) {
			ERR("could not get all sorted dependencies of %s\n", p);
			goto end;
		}

		for (j = 0; j < deps.count; j++) {
			const struct mod *d = deps.array[j];
			fprintf(out, " %s", mod_get_compressed_path(d));
		}
end:
		putc('\n', out);
	}

	array_free_array(&deps);
	return 0;
}

static int output_deps_bin(struct depmod *depmod, FILE *out)
{
	struct index_node *idx;
	size_t i;
	struct array array;
	struct strbuf sbuf;

	if (out == stdout)
		return 0;

	idx = index_create();
	if (idx == NULL)
		return -ENOMEM;

	array_init(&array, 64);
	strbuf_init(&sbuf);

	for (i = 0; i < depmod->modules.count; i++) {
		const struct mod *mod = depmod->modules.array[i];
		const char *p = mod_get_compressed_path(mod);
		const char *line;
		size_t j;
		int duplicate;

		if (!mod_get_all_sorted_dependencies(mod, &array)) {
			ERR("could not get all sorted dependencies of %s\n", p);
			continue;
		}

		strbuf_clear(&sbuf);
		if (!strbuf_pushchars(&sbuf, p) || !strbuf_pushchar(&sbuf, ':')) {
			ERR("could not write dependencies of %s\n", p);
			continue;
		}

		for (j = 0; j < array.count; j++) {
			const struct mod *d = array.array[j];
			const char *dp = mod_get_compressed_path(d);

			if (!strbuf_pushchar(&sbuf, ' ') || !strbuf_pushchars(&sbuf, dp)) {
				ERR("could not write dependencies of %s\n", p);
				continue;
			}
		}
		line = strbuf_str(&sbuf);

		duplicate = index_insert(idx, mod->modname, line, mod->idx);
		if (duplicate && depmod->cfg->warn_dups)
			WRN("duplicate module deps:\n%s\n", line);
	}

	strbuf_release(&sbuf);
	array_free_array(&array);
	index_write(idx, out);
	index_destroy(idx);

	return 0;
}

static int output_aliases(struct depmod *depmod, FILE *out)
{
	size_t i;

	fputs("# Aliases extracted from modules themselves.\n", out);

	for (i = 0; i < depmod->modules.count; i++) {
		const struct mod *mod = depmod->modules.array[i];
		struct kmod_list *l;

		kmod_list_foreach(l, mod->info_list) {
			const char *key = kmod_module_info_get_key(l);
			const char *value = kmod_module_info_get_value(l);

			if (!streq(key, "alias"))
				continue;

			fprintf(out, "alias %s %s\n", value, mod->modname);
		}
	}

	return 0;
}

static int output_aliases_bin(struct depmod *depmod, FILE *out)
{
	struct index_node *idx;
	size_t i;

	if (out == stdout)
		return 0;

	idx = index_create();
	if (idx == NULL)
		return -ENOMEM;

	for (i = 0; i < depmod->modules.count; i++) {
		const struct mod *mod = depmod->modules.array[i];
		struct kmod_list *l;

		kmod_list_foreach(l, mod->info_list) {
			const char *key = kmod_module_info_get_key(l);
			const char *value = kmod_module_info_get_value(l);
			char buf[PATH_MAX];
			const char *alias;
			int duplicate;

			if (!streq(key, "alias"))
				continue;

			if (alias_normalize(value, buf, NULL) < 0) {
				WRN("Unmatched bracket in %s\n", value);
				continue;
			}
			alias = buf;

			duplicate = index_insert(idx, alias, mod->modname, mod->idx);
			if (duplicate && depmod->cfg->warn_dups)
				WRN("duplicate module alias:\n%s %s\n", alias,
				    mod->modname);
		}
	}

	index_write(idx, out);
	index_destroy(idx);

	return 0;
}

static int output_softdeps(struct depmod *depmod, FILE *out)
{
	size_t i;

	fputs("# Soft dependencies extracted from modules themselves.\n", out);

	for (i = 0; i < depmod->modules.count; i++) {
		const struct mod *mod = depmod->modules.array[i];
		struct kmod_list *l;

		kmod_list_foreach(l, mod->info_list) {
			const char *key = kmod_module_info_get_key(l);
			const char *value = kmod_module_info_get_value(l);

			if (!streq(key, "softdep"))
				continue;

			fprintf(out, "softdep %s %s\n", mod->modname, value);
		}
	}

	return 0;
}

static int output_weakdeps(struct depmod *depmod, FILE *out)
{
	size_t i;

	fputs("# Weak dependencies extracted from modules themselves.\n", out);

	for (i = 0; i < depmod->modules.count; i++) {
		const struct mod *mod = depmod->modules.array[i];
		struct kmod_list *l;

		kmod_list_foreach(l, mod->info_list) {
			const char *key = kmod_module_info_get_key(l);
			const char *value = kmod_module_info_get_value(l);

			if (!streq(key, "weakdep"))
				continue;

			fprintf(out, "weakdep %s %s\n", mod->modname, value);
		}
	}

	return 0;
}

static int output_symbols(struct depmod *depmod, FILE *out)
{
	struct hash_iter iter;
	const void *v;

	fputs("# Aliases for symbols, used by symbol_request().\n", out);

	hash_iter_init(depmod->symbols, &iter);

	while (hash_iter_next(&iter, NULL, &v)) {
		const struct symbol *sym = v;
		if (sym->owner == NULL)
			continue;

		fprintf(out, "alias symbol:%s %s\n", sym->name, sym->owner->modname);
	}

	return 0;
}

static int output_symbols_bin(struct depmod *depmod, FILE *out)
{
	struct index_node *idx;
	char aliasbuf[1024] = "symbol:";
	_cleanup_(scratchbuf_release) struct scratchbuf salias =
		SCRATCHBUF_INITIALIZER(aliasbuf);
	const size_t baselen = sizeof("symbol:") - 1;
	struct hash_iter iter;
	const void *v;
	int ret = 0;

	if (out == stdout)
		return 0;

	idx = index_create();
	if (idx == NULL)
		return -ENOMEM;

	hash_iter_init(depmod->symbols, &iter);

	while (hash_iter_next(&iter, NULL, &v)) {
		int duplicate;
		const struct symbol *sym = v;
		size_t len;
		char *s;

		if (sym->owner == NULL)
			continue;

		len = strlen(sym->name);

		if (scratchbuf_alloc(&salias, baselen + len + 1) < 0) {
			ret = -ENOMEM;
			goto err_scratchbuf;
		}
		s = scratchbuf_str(&salias);
		memcpy(s + baselen, sym->name, len + 1);
		duplicate = index_insert(idx, s, sym->owner->modname, sym->owner->idx);

		if (duplicate && depmod->cfg->warn_dups)
			WRN("duplicate module syms:\n%s %s\n", s, sym->owner->modname);
	}

	index_write(idx, out);

err_scratchbuf:
	index_destroy(idx);

	if (ret < 0)
		ERR("output symbols: %s\n", strerror(-ret));

	return ret;
}

static int output_builtin_bin(struct depmod *depmod, FILE *out)
{
	FILE *in;
	struct index_node *idx;
	char line[PATH_MAX], modname[PATH_MAX];

	if (out == stdout)
		return 0;

	in = dfdopen(depmod->cfg->dirname, "modules.builtin", O_RDONLY, "r");
	if (in == NULL)
		return 0;

	idx = index_create();
	if (idx == NULL) {
		fclose(in);
		return -ENOMEM;
	}

	while (fgets(line, sizeof(line), in) != NULL) {
		if (!isalpha(line[0])) {
			ERR("Invalid modules.builtin line: %s\n", line);
			continue;
		}

		path_to_modname(line, modname, NULL);
		index_insert(idx, modname, "", 0);
	}

	index_write(idx, out);
	index_destroy(idx);
	fclose(in);

	return 0;
}

static int flush_stream(FILE *in, int endchar)
{
	size_t i = 0;
	int c;

	for (c = fgetc(in); c != EOF && c != endchar && c != '\0'; c = fgetc(in))
		;

	return c == endchar ? i : 0;
}

static int flush_stream_to(FILE *in, int endchar, char *dst, size_t dst_sz)
{
	size_t i = 0;
	int c;

	for (c = fgetc(in); c != EOF && c != endchar && c != '\0' && i < dst_sz;
	     c = fgetc(in))
		dst[i++] = c;

	if (i == dst_sz) {
		WRN("Could not flush stream: %d. Partial content: %.*s\n", ENOSPC,
		    (int)dst_sz, dst);
		i--;
	}

	return c == endchar ? i : 0;
}

static int output_builtin_alias_bin(struct depmod *depmod, FILE *out)
{
	FILE *in;
	struct index_node *idx;
	int ret;

	if (out == stdout)
		return 0;

	in = dfdopen(depmod->cfg->dirname, "modules.builtin.modinfo", O_RDONLY, "r");
	if (in == NULL)
		return 0;

	idx = index_create();
	if (idx == NULL) {
		fclose(in);
		return -ENOMEM;
	}

	/* format: modname.key=value\0 */
	while (!feof(in) && !ferror(in)) {
		char alias[PATH_MAX];
		char modname[PATH_MAX];
		char value[PATH_MAX];
		size_t len;

		len = flush_stream_to(in, '.', modname, sizeof(modname));
		modname[len] = '\0';
		if (!len)
			continue;

		len = flush_stream_to(in, '=', value, sizeof(value));
		value[len] = '\0';
		if (!streq(value, "alias")) {
			flush_stream(in, '\0');
			continue;
		}

		len = flush_stream_to(in, '\0', value, sizeof(value));
		value[len] = '\0';
		if (!len)
			continue;

		alias[0] = '\0';
		if (alias_normalize(value, alias, NULL) < 0) {
			WRN("Unmatched bracket in %s\n", value);
			continue;
		}

		index_insert(idx, alias, modname, 0);
	}

	if (ferror(in)) {
		ret = -EINVAL;
	} else {
		index_write(idx, out);
		ret = 0;
	}

	index_destroy(idx);
	fclose(in);

	return ret;
}

static int output_devname(struct depmod *depmod, FILE *out)
{
	size_t i;
	bool empty = true;

	for (i = 0; i < depmod->modules.count; i++) {
		const struct mod *mod = depmod->modules.array[i];
		struct kmod_list *l;
		const char *devname = NULL;
		char type = '\0';
		unsigned int major = 0, minor = 0;

		kmod_list_foreach(l, mod->info_list) {
			const char *key = kmod_module_info_get_key(l);
			const char *value = kmod_module_info_get_value(l);
			unsigned int maj, min;

			if (!streq(key, "alias"))
				continue;

			if (strstartswith(value, "devname:"))
				devname = value + sizeof("devname:") - 1;
			else if (sscanf(value, "char-major-%u-%u", &maj, &min) == 2) {
				type = 'c';
				major = maj;
				minor = min;
			} else if (sscanf(value, "block-major-%u-%u", &maj, &min) == 2) {
				type = 'b';
				major = maj;
				minor = min;
			}

			if (type != '\0' && devname != NULL)
				break;
		}

		if (devname != NULL) {
			if (type != '\0') {
				if (empty) {
					fputs("# Device nodes to trigger on-demand module loading.\n",
					      out);
					empty = false;
				}
				fprintf(out, "%s %s %c%u:%u\n", mod->modname, devname,
					type, major, minor);
			} else
				ERR("Module '%s' has devname (%s) but "
				    "lacks major and minor information. "
				    "Ignoring.\n",
				    mod->modname, devname);
		}
	}

	return 0;
}

static int depmod_output(struct depmod *depmod, FILE *out)
{
	static const struct depfile {
		const char *name;
		int (*cb)(struct depmod *depmod, FILE *out);
	} *itr, depfiles[] = {
		{ "modules.dep", output_deps },
		{ "modules.dep.bin", output_deps_bin },
		{ "modules.alias", output_aliases },
		{ "modules.alias.bin", output_aliases_bin },
		{ "modules.softdep", output_softdeps },
		{ "modules.weakdep", output_weakdeps },
		{ "modules.symbols", output_symbols },
		{ "modules.symbols.bin", output_symbols_bin },
		{ "modules.builtin.bin", output_builtin_bin },
		{ "modules.builtin.alias.bin", output_builtin_alias_bin },
		{ "modules.devname", output_devname },
		{},
	};
	const char *dname = depmod->cfg->outdirname;
	int dfd, err = 0;
	struct timeval tv;

	gettimeofday(&tv, NULL);

	if (out != NULL)
		dfd = -1;
	else {
		err = mkdir_p(dname, strlen(dname), 0755);
		if (err < 0) {
			CRIT("could not create directory %s: %m\n", dname);
			return err;
		}
		dfd = open(dname, O_RDONLY);
		if (dfd < 0) {
			err = -errno;
			CRIT("could not open directory %s: %m\n", dname);
			return err;
		}
	}

	for (itr = depfiles; itr->name != NULL; itr++) {
		FILE *fp = out;
		char tmp[NAME_MAX] = "";
		int r, ferr;

		if (fp == NULL) {
			int flags = O_CREAT | O_EXCL | O_WRONLY;
			int mode = 0644;
			int fd;
			int n;

			n = snprintf(tmp, sizeof(tmp), "%s.%i.%lli.%lli", itr->name,
				     getpid(), (long long)tv.tv_usec,
				     (long long)tv.tv_sec);
			if (n >= (int)sizeof(tmp)) {
				ERR("bad filename: %s.%i.%lli.%lli: path too long\n",
				    itr->name, getpid(), (long long)tv.tv_usec,
				    (long long)tv.tv_sec);
				continue;
			}
			fd = openat(dfd, tmp, flags, mode);
			if (fd < 0) {
				ERR("openat(%s, %s, %o, %o): %m\n", dname, tmp, flags,
				    mode);
				continue;
			}
			fp = fdopen(fd, "wb");
			if (fp == NULL) {
				ERR("fdopen(%d=%s/%s): %m\n", fd, dname, tmp);
				close(fd);
				continue;
			}
		}

		r = itr->cb(depmod, fp);
		if (fp == out)
			continue;

		ferr = ferror(fp) | fclose(fp);

		if (r < 0) {
			if (unlinkat(dfd, tmp, 0) != 0)
				ERR("unlinkat(%s, %s): %m\n", dname, tmp);

			ERR("Could not write index '%s': %s\n", itr->name, strerror(-r));
			err = -errno;
			break;
		}

		if (renameat(dfd, tmp, dfd, itr->name) != 0) {
			err = -errno;
			CRIT("renameat(%s, %s, %s, %s): %m\n", dname, tmp, dname,
			     itr->name);
			break;
		}

		if (ferr) {
			err = -ENOSPC;
			ERR("Could not create index '%s'. Output is truncated: %s\n",
			    itr->name, strerror(-err));
			break;
		}
	}

	if (dfd >= 0)
		close(dfd);

	return err;
}

static void depmod_add_fake_syms(struct depmod *depmod)
{
	/* __this_module is magically inserted by kernel loader. */
	depmod_symbol_add(depmod, "__this_module", true, 0, NULL);
	/* On S390, this is faked up too */
	depmod_symbol_add(depmod, "_GLOBAL_OFFSET_TABLE_", true, 0, NULL);
	/* On PowerPC64 ABIv2, .TOC. is more or less _GLOBAL_OFFSET_TABLE_ */
	if (!depmod_symbol_find(depmod, "TOC."))
		depmod_symbol_add(depmod, "TOC.", true, 0, NULL);
}

static int depmod_load_symvers(struct depmod *depmod, const char *filename)
{
	char line[10240];
	FILE *fp;
	unsigned int linenum = 0;

	fp = fopen(filename, "r");
	if (fp == NULL) {
		int err = -errno;
		DBG("load symvers: %s: %m\n", filename);
		return err;
	}
	DBG("load symvers: %s\n", filename);

	/* eg. "0xb352177e\tfind_first_bit\tvmlinux\tEXPORT_SYMBOL" */
	while (fgets(line, sizeof(line), fp) != NULL) {
		const char *ver, *sym, *where;
		char *verend;
		uint64_t crc;

		linenum++;

		ver = strtok(line, " \t");
		sym = strtok(NULL, " \t");
		where = strtok(NULL, " \t");
		if (!ver || !sym || !where)
			continue;

		if (!streq(where, "vmlinux"))
			continue;

		crc = strtoull(ver, &verend, 16);
		if (verend[0] != '\0') {
			ERR("%s:%u Invalid symbol version %s: %m\n", filename, linenum,
			    ver);
			continue;
		}

		depmod_symbol_add(depmod, sym, false, crc, NULL);
	}
	depmod_add_fake_syms(depmod);

	DBG("loaded symvers: %s\n", filename);

	fclose(fp);
	return 0;
}

static int depmod_load_system_map(struct depmod *depmod, const char *filename)
{
	const char ksymstr[] = "__ksymtab_";
	const size_t ksymstr_len = sizeof(ksymstr) - 1;
	char line[10240];
	FILE *fp;
	unsigned int linenum = 0;

	fp = fopen(filename, "r");
	if (fp == NULL) {
		int err = -errno;
		DBG("load System.map: %s: %m\n", filename);
		return err;
	}
	DBG("load System.map: %s\n", filename);

	/* eg. c0294200 R __ksymtab_devfs_alloc_devnum */
	while (fgets(line, sizeof(line), fp) != NULL) {
		char *p, *end;

		linenum++;

		p = strchr(line, ' ');
		if (p == NULL)
			goto invalid_syntax;
		p++;
		p = strchr(p, ' ');
		if (p == NULL)
			goto invalid_syntax;
		p++;

		/* skip prefix */
		if (p[0] == depmod->cfg->sym_prefix)
			p++;

		/* Covers gpl-only and normal symbols. */
		if (strncmp(p, ksymstr, ksymstr_len) != 0)
			continue;

		end = strchr(p, '\n');
		if (end != NULL)
			*end = '\0';

		depmod_symbol_add(depmod, p + ksymstr_len, true, 0, NULL);
		continue;

invalid_syntax:
		ERR("%s:%u: invalid line: %s\n", filename, linenum, line);
	}
	depmod_add_fake_syms(depmod);

	DBG("loaded System.map: %s\n", filename);

	fclose(fp);
	return 0;
}

static int depfile_up_to_date_dir(DIR *d, time_t mtime, size_t baselen, char *path)
{
	struct dirent *de;
	int err = 1, dfd = dirfd(d);

	while ((de = readdir(d)) != NULL) {
		const char *name = de->d_name;
		size_t namelen;
		struct stat st;

		if (name[0] == '.' &&
		    (name[1] == '\0' || (name[1] == '.' && name[2] == '\0')))
			continue;
		if (streq(name, "build") || streq(name, "source"))
			continue;
		namelen = strlen(name);
		if (baselen + namelen + 2 >= PATH_MAX) {
			path[baselen] = '\0';
			ERR("path is too long %s%s\n", path, name);
			continue;
		}

		if (fstatat(dfd, name, &st, 0) < 0) {
			ERR("fstatat(%d, %s): %m\n", dfd, name);
			continue;
		}

		if (S_ISDIR(st.st_mode)) {
			int fd;
			DIR *subdir;
			memcpy(path + baselen, name, namelen + 1);
			if (baselen + namelen + 2 + NAME_MAX >= PATH_MAX) {
				ERR("directory path is too long %s\n", path);
				continue;
			}
			fd = openat(dfd, name, O_RDONLY);
			if (fd < 0) {
				ERR("openat(%d, %s, O_RDONLY): %m\n", dfd, name);
				continue;
			}
			subdir = fdopendir(fd);
			if (subdir == NULL) {
				ERR("fdopendir(%d): %m\n", fd);
				close(fd);
				continue;
			}
			path[baselen + namelen] = '/';
			path[baselen + namelen + 1] = '\0';
			err = depfile_up_to_date_dir(subdir, mtime, baselen + namelen + 1,
						     path);
			closedir(subdir);
		} else if (S_ISREG(st.st_mode)) {
			if (!path_ends_with_kmod_ext(name, namelen))
				continue;

			memcpy(path + baselen, name, namelen + 1);
			err = st.st_mtime <= mtime;
			if (err == 0) {
				DBG("%s %" PRIu64 " is newer than %" PRIu64 "\n", path,
				    (uint64_t)st.st_mtime, (uint64_t)mtime);
			}
		} else {
			ERR("unsupported file type %s: %o\n", path, st.st_mode & S_IFMT);
			continue;
		}

		if (err == 0)
			break; /* outdated! */
		else if (err < 0) {
			path[baselen + namelen] = '\0';
			ERR("failed %s: %s\n", path, strerror(-err));
			err = 1; /* ignore errors */
		}
	}

	return err;
}

/* up-to-date: 1, outdated: 0, errors < 0 */
static int depfile_up_to_date(const char *dirname)
{
	char path[PATH_MAX + 1];
	DIR *d = opendir(dirname);
	struct stat st;
	size_t baselen;
	int err;
	if (d == NULL) {
		err = -errno;
		ERR("could not open directory %s: %m\n", dirname);
		return err;
	}

	if (fstatat(dirfd(d), "modules.dep", &st, 0) != 0) {
		err = -errno;
		ERR("could not fstatat(%s, modules.dep): %m\n", dirname);
		closedir(d);
		return err;
	}

	baselen = strlen(dirname);
	memcpy(path, dirname, baselen);
	path[baselen] = '/';
	baselen++;
	path[baselen] = '\0';

	err = depfile_up_to_date_dir(d, st.st_mtime, baselen, path);
	closedir(d);
	return err;
}

static int is_version_number(const char *version)
{
	unsigned int d1, d2;
	return (sscanf(version, "%u.%u", &d1, &d2) == 2);
}

static int do_depmod(int argc, char *argv[])
{
	FILE *out = NULL;
	int err = 0, all = 0, maybe_all = 0, n_config_paths = 0;
	_cleanup_free_ char *root = NULL;
	_cleanup_free_ char *out_root = NULL;
	_cleanup_free_ const char **config_paths = NULL;
	const char *system_map = NULL;
	const char *module_symvers = NULL;
	const char *null_kmod_config = NULL;
	struct utsname un;
	struct kmod_ctx *ctx = NULL;
	struct cfg cfg;
	struct depmod depmod;

	memset(&cfg, 0, sizeof(cfg));
	memset(&depmod, 0, sizeof(depmod));

	for (;;) {
		int c, idx = 0;
		c = getopt_long(argc, argv, cmdopts_s, cmdopts, &idx);
		if (c == -1)
			break;
		switch (c) {
		case 'a':
			all = 1;
			break;
		case 'A':
			maybe_all = 1;
			break;
		case 'b':
			free(root);
			root = path_make_absolute_cwd(optarg);
			if (root == NULL) {
				ERR("invalid image path %s\n", optarg);
				goto cmdline_failed;
			}
			break;
		case 'm':
			module_directory = optarg;
			break;
		case 'o':
			free(out_root);
			out_root = path_make_absolute_cwd(optarg);
			if (out_root == NULL) {
				ERR("invalid output directory %s\n", optarg);
				goto cmdline_failed;
			}
			break;
		case 'C': {
			size_t bytes = sizeof(char *) * (n_config_paths + 2);
			void *tmp = realloc(config_paths, bytes);
			if (!tmp) {
				fputs("Error: out-of-memory\n", stderr);
				goto cmdline_failed;
			}
			config_paths = tmp;
			config_paths[n_config_paths] = optarg;
			n_config_paths++;
			config_paths[n_config_paths] = NULL;
			break;
		}
		case 'E':
			module_symvers = optarg;
			cfg.check_symvers = 1;
			break;
		case 'F':
			system_map = optarg;
			break;
		case 'e':
			cfg.print_unknown = 1;
			break;
		case 'v':
			verbose++;
			break;
		case 'n':
			out = stdout;
			break;
		case 'P':
			if (strlen(optarg) != 1) {
				CRIT("-P only takes a single char\n");
				goto cmdline_failed;
			}
			cfg.sym_prefix = optarg[0];
			break;
		case 'w':
			cfg.warn_dups = 1;
			break;
		case 'h':
			help();
			return EXIT_SUCCESS;
		case 'V':
			kmod_version();
			return EXIT_SUCCESS;
		case '?':
			goto cmdline_failed;
		default:
			ERR("unexpected getopt_long() value '%c'.\n", c);
			goto cmdline_failed;
		}
	}

	if (optind < argc) {
		if (!is_version_number(argv[optind])) {
			ERR("Bad version passed %s\n", argv[optind]);
			goto cmdline_failed;
		}
		cfg.kversion = argv[optind];
		optind++;
	} else {
		if (uname(&un) < 0) {
			CRIT("uname() failed: %s\n", strerror(errno));
			goto cmdline_failed;
		}
		cfg.kversion = un.release;
	}

	cfg.dirnamelen = snprintf(cfg.dirname, PATH_MAX, "%s%s/%s", root ?: "",
				  module_directory, cfg.kversion);
	if (cfg.dirnamelen >= PATH_MAX) {
		ERR("Bad directory %s" MODULE_DIRECTORY "/%s: path too long\n",
		    root ?: "", cfg.kversion);
		goto cmdline_failed;
	}

	cfg.outdirnamelen = snprintf(cfg.outdirname, PATH_MAX, "%s%s/%s",
				     out_root ?: (root ?: ""), module_directory,
				     cfg.kversion);
	if (cfg.outdirnamelen >= PATH_MAX) {
		ERR("Bad directory %s" MODULE_DIRECTORY "/%s: path too long\n",
		    out_root ?: (root ?: ""), cfg.kversion);
		goto cmdline_failed;
	}

	if (optind == argc)
		all = 1;

	if (maybe_all) {
		if (out == stdout)
			goto done;
		/* ignore up-to-date errors (< 0) */
		if (depfile_up_to_date(cfg.dirname) == 1)
			goto done;
		all = 1;
	}

	ctx = kmod_new(cfg.dirname, &null_kmod_config);
	if (ctx == NULL) {
		CRIT("kmod_new(\"%s\", {NULL}) failed: %m\n", cfg.dirname);
		goto cmdline_failed;
	}

	log_setup_kmod_log(ctx, verbose);

	err = depmod_init(&depmod, &cfg, ctx);
	if (err < 0) {
		CRIT("depmod_init: %s\n", strerror(-err));
		goto depmod_init_failed;
	}
	ctx = NULL; /* owned by depmod */

	if (module_symvers != NULL) {
		err = depmod_load_symvers(&depmod, module_symvers);
		if (err < 0) {
			CRIT("could not load %s: %s\n", module_symvers, strerror(-err));
			goto cmdline_failed;
		}
	} else if (system_map != NULL) {
		err = depmod_load_system_map(&depmod, system_map);
		if (err < 0) {
			CRIT("could not load %s: %s\n", system_map, strerror(-err));
			goto cmdline_failed;
		}
	} else if (cfg.print_unknown) {
		WRN("-e needs -E or -F\n");
		cfg.print_unknown = 0;
	}

	if (all) {
		err = cfg_load(&cfg, config_paths);
		if (err < 0) {
			CRIT("could not load configuration files\n");
			goto cmdline_modules_failed;
		}
		err = depmod_modules_search(&depmod);
		if (err < 0) {
			CRIT("could not search modules: %s\n", strerror(-err));
			goto cmdline_modules_failed;
		}
	} else {
		int i;

		for (i = optind; i < argc; i++) {
			const char *path = argv[i];
			struct kmod_module *mod;

			if (path[0] != '/') {
				CRIT("%s: not absolute path.\n", path);
				goto cmdline_modules_failed;
			}

			err = kmod_module_new_from_path(depmod.ctx, path, &mod);
			if (err < 0) {
				CRIT("could not create module %s: %s\n", path,
				     strerror(-err));
				goto cmdline_modules_failed;
			}

			err = depmod_module_add(&depmod, mod);
			if (err < 0) {
				CRIT("could not add module %s: %s\n", path,
				     strerror(-err));
				kmod_module_unref(mod);
				goto cmdline_modules_failed;
			}
		}
	}

	err = depmod_modules_build_array(&depmod);
	if (err < 0) {
		CRIT("could not build module array: %s\n", strerror(-err));
		goto cmdline_modules_failed;
	}

	depmod_modules_sort(&depmod);
	err = depmod_load(&depmod);
	if (err < 0)
		goto cmdline_modules_failed;

	err = depmod_output(&depmod, out);

done:
	depmod_shutdown(&depmod);
	cfg_free(&cfg);
	return err >= 0 ? EXIT_SUCCESS : EXIT_FAILURE;

cmdline_modules_failed:
	depmod_shutdown(&depmod);
depmod_init_failed:
	if (ctx != NULL)
		kmod_unref(ctx);
cmdline_failed:
	cfg_free(&cfg);
	return EXIT_FAILURE;
}

const struct kmod_cmd kmod_cmd_compat_depmod = {
	.name = "depmod",
	.cmd = do_depmod,
	.help = "compat depmod command",
};
