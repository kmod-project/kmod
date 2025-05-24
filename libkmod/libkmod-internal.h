#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <syslog.h>
#include <limits.h>

#include <shared/macro.h>
#include <shared/missing.h>

#include "libkmod.h"

#define kmod_log_cond(ctx, prio, arg...)                                           \
	do {                                                                       \
		if (ENABLE_LOGGING == 1 &&                                         \
		    (ENABLE_DEBUG == 1 || (!ENABLE_DEBUG && prio != LOG_DEBUG)) && \
		    kmod_get_log_priority(ctx) >= prio)                            \
			kmod_log(ctx, prio, __FILE__, __LINE__, __func__, ##arg);  \
	} while (0)

#define DBG(ctx, arg...) kmod_log_cond(ctx, LOG_DEBUG, ##arg)
#define NOTICE(ctx, arg...) kmod_log_cond(ctx, LOG_NOTICE, ##arg)
#define INFO(ctx, arg...) kmod_log_cond(ctx, LOG_INFO, ##arg)
#define ERR(ctx, arg...) kmod_log_cond(ctx, LOG_ERR, ##arg)

#define KMOD_EXPORT __attribute__((visibility("default")))

#define KCMD_LINE_SIZE 4096

#if !HAVE_SECURE_GETENV
#warning secure_getenv is not available
#define secure_getenv getenv
#endif

_printf_format_(6, 7) _nonnull_(1, 3, 5) void kmod_log(const struct kmod_ctx *ctx,
						       int priority, const char *file,
						       int line, const char *fn,
						       const char *format, ...);

struct list_node {
	struct list_node *next, *prev;
};

struct kmod_list {
	struct list_node node;
	void *data;
};

enum kmod_file_compression_type {
	KMOD_FILE_COMPRESSION_NONE = 0,
	KMOD_FILE_COMPRESSION_ZSTD,
	KMOD_FILE_COMPRESSION_XZ,
	KMOD_FILE_COMPRESSION_ZLIB,
};

// clang-format off
_must_check_ _nonnull_(2) struct kmod_list *kmod_list_append(struct kmod_list *list, const void *data);
_must_check_ _nonnull_(2) struct kmod_list *kmod_list_prepend(struct kmod_list *list, const void *data);
_must_check_ struct kmod_list *kmod_list_remove(struct kmod_list *list);
_must_check_ _nonnull_(2) struct kmod_list *kmod_list_remove_data(struct kmod_list *list, const void *data);
_nonnull_(2) struct kmod_list *kmod_list_insert_after(struct kmod_list *list, const void *data);
_nonnull_(2) struct kmod_list *kmod_list_insert_before(struct kmod_list *list, const void *data);
_must_check_ struct kmod_list *kmod_list_append_list(struct kmod_list *list1, struct kmod_list *list2);
#define kmod_list_release(list, free_data)     \
	while (list) {                         \
		free_data((list)->data);       \
		list = kmod_list_remove(list); \
	}

/* libkmod.c */
_nonnull_all_ int kmod_lookup_alias_from_config(struct kmod_ctx *ctx, const char *name, struct kmod_list **list);
_nonnull_all_ int kmod_lookup_alias_from_symbols_file(struct kmod_ctx *ctx, const char *name, struct kmod_list **list);
_nonnull_all_ int kmod_lookup_alias_from_aliases_file(struct kmod_ctx *ctx, const char *name, struct kmod_list **list);
_nonnull_all_ int kmod_lookup_alias_from_moddep_file(struct kmod_ctx *ctx, const char *name, struct kmod_list **list);
_nonnull_all_ int kmod_lookup_alias_from_kernel_builtin_file(struct kmod_ctx *ctx, const char *name, struct kmod_list **list);
_nonnull_all_ int kmod_lookup_alias_from_builtin_file(struct kmod_ctx *ctx, const char *name, struct kmod_list **list);
_nonnull_all_ bool kmod_lookup_alias_is_builtin(struct kmod_ctx *ctx, const char *name);
_nonnull_all_ int kmod_lookup_alias_from_commands(struct kmod_ctx *ctx, const char *name, struct kmod_list **list);
_nonnull_(1) void kmod_set_modules_visited(struct kmod_ctx *ctx, bool visited);
_nonnull_(1) void kmod_set_modules_required(struct kmod_ctx *ctx, bool required);

_nonnull_all_ char *kmod_search_moddep(struct kmod_ctx *ctx, const char *name);

_nonnull_all_ struct kmod_module *kmod_pool_get_module(struct kmod_ctx *ctx, const char *key);
_nonnull_all_ int kmod_pool_add_module(struct kmod_ctx *ctx, struct kmod_module *mod, const char *key);
_nonnull_all_ void kmod_pool_del_module(struct kmod_ctx *ctx, struct kmod_module *mod, const char *key);

_nonnull_all_ const struct kmod_config *kmod_get_config(const struct kmod_ctx *ctx);
_nonnull_all_ enum kmod_file_compression_type kmod_get_kernel_compression(const struct kmod_ctx *ctx);

/* libkmod-config.c */
struct kmod_config_path {
	unsigned long long stamp;
	char path[];
};

struct kmod_config {
	struct kmod_ctx *ctx;
	struct kmod_list *aliases;
	struct kmod_list *blacklists;
	struct kmod_list *options;
	struct kmod_list *remove_commands;
	struct kmod_list *install_commands;
	struct kmod_list *softdeps;
	struct kmod_list *weakdeps;

	struct kmod_list *paths;
};

_nonnull_all_ int kmod_config_new(struct kmod_ctx *ctx, struct kmod_config **config, const char *const *config_paths);
_nonnull_all_ void kmod_config_free(struct kmod_config *config);
_nonnull_all_ const char *kmod_blacklist_get_modname(const struct kmod_list *l);
_nonnull_all_ const char *kmod_alias_get_name(const struct kmod_list *l);
_nonnull_all_ const char *kmod_alias_get_modname(const struct kmod_list *l);
_nonnull_all_ const char *kmod_option_get_options(const struct kmod_list *l);
_nonnull_all_ const char *kmod_option_get_modname(const struct kmod_list *l);
_nonnull_all_ const char *kmod_command_get_command(const struct kmod_list *l);
_nonnull_all_ const char *kmod_command_get_modname(const struct kmod_list *l);

_nonnull_all_ const char *kmod_softdep_get_name(const struct kmod_list *l);
_nonnull_all_ const char *const *kmod_softdep_get_pre(const struct kmod_list *l, unsigned int *count);
const char *const *kmod_softdep_get_post(const struct kmod_list *l, unsigned int *count);

_nonnull_all_ const char * kmod_weakdep_get_name(const struct kmod_list *l);
_nonnull_all_ const char *const *kmod_weakdep_get_weak(const struct kmod_list *l, unsigned int *count);

/* libkmod-module.c */
int kmod_module_new_from_alias(struct kmod_ctx *ctx, const char *alias, const char *name, struct kmod_module **mod);
_nonnull_all_ void kmod_module_parse_depline(struct kmod_module *mod, char *line);
_nonnull_(1) void kmod_module_set_install_commands(struct kmod_module *mod, const char *cmd);
_nonnull_(1) void kmod_module_set_remove_commands(struct kmod_module *mod, const char *cmd);
_nonnull_(1)void kmod_module_set_visited(struct kmod_module *mod, bool visited);
_nonnull_(1) void kmod_module_set_builtin(struct kmod_module *mod, bool builtin);
_nonnull_(1) void kmod_module_set_required(struct kmod_module *mod, bool required);
_nonnull_all_ bool kmod_module_is_builtin(struct kmod_module *mod);

/* libkmod-file.c */
struct kmod_file;
struct kmod_elf;
_must_check_ _nonnull_all_ int kmod_file_open(const struct kmod_ctx *ctx, const char *filename, struct kmod_file **file);
_must_check_ _nonnull_all_ int kmod_file_get_elf(struct kmod_file *file, struct kmod_elf **elf);
_nonnull_all_ int kmod_file_load_contents(struct kmod_file *file);
_must_check_ _nonnull_all_ const void *kmod_file_get_contents(const struct kmod_file *file);
_must_check_ _nonnull_all_ off_t kmod_file_get_size(const struct kmod_file *file);
_must_check_ _nonnull_all_ enum kmod_file_compression_type kmod_file_get_compression(const struct kmod_file *file);
_must_check_ _nonnull_all_ int kmod_file_get_fd(const struct kmod_file *file);
_nonnull_all_ void kmod_file_unref(struct kmod_file *file);

/* libkmod-elf.c */
struct kmod_modversion {
	uint64_t crc;
	enum kmod_symbol_bind bind;
	const char *symbol;
};

_must_check_ _nonnull_all_ int kmod_elf_new(const void *memory, off_t size, struct kmod_elf **elf);
_nonnull_all_ void kmod_elf_unref(struct kmod_elf *elf);
_must_check_ _nonnull_all_ const void *kmod_elf_get_memory(const struct kmod_elf *elf);
_must_check_ _nonnull_all_ int kmod_elf_get_modinfo_strings(const struct kmod_elf *elf, char ***array);
_must_check_ _nonnull_all_ int kmod_elf_get_modversions(const struct kmod_elf *elf, struct kmod_modversion **array);
_must_check_ _nonnull_all_ int kmod_elf_get_symbols(const struct kmod_elf *elf, struct kmod_modversion **array);
_must_check_ _nonnull_all_ int kmod_elf_get_dependency_symbols(const struct kmod_elf *elf, struct kmod_modversion **array);
_must_check_ _nonnull_all_ int kmod_elf_strip(const struct kmod_elf *elf, unsigned int flags, const void **stripped);

/*
 * Debug mock lib need to find section ".gnu.linkonce.this_module" in order to
 * get modname
 */
_must_check_ _nonnull_all_ int kmod_elf_get_section(const struct kmod_elf *elf, const char *section, uint64_t *sec_off, uint64_t *sec_size);

/* libkmod-signature.c */
struct kmod_signature_info {
	const char *signer;
	size_t signer_len;
	const char *key_id;
	size_t key_id_len;
	const char *algo, *hash_algo, *id_type;
	const char *sig;
	size_t sig_len;
	void (*free)(void *);
	void *private;
};
_must_check_ _nonnull_all_ bool kmod_module_signature_info(const struct kmod_file *file, struct kmod_signature_info *sig_info);
_nonnull_all_ void kmod_module_signature_info_free(struct kmod_signature_info *sig_info);

/* libkmod-builtin.c */
_nonnull_all_ ssize_t kmod_builtin_get_modinfo(struct kmod_ctx *ctx, const char *modname, char ***modinfo);
// clang-format on
