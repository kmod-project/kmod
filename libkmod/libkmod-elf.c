// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2011-2013  ProFUSION embedded systems
 */

#include <assert.h>
#include <elf.h>
#include <endian.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <shared/util.h>

#include "libkmod.h"
#include "libkmod-internal.h"

/* as defined in module-init-tools */
struct kmod_modversion32 {
	uint32_t crc;
	char name[64 - sizeof(uint32_t)];
};

struct kmod_modversion64 {
	uint64_t crc;
	char name[64 - sizeof(uint64_t)];
};

struct kmod_elf {
	const uint8_t *memory;
	uint64_t size;
	bool x32;
	bool msb;
	struct {
		struct {
			uint64_t offset;
			uint16_t count;
			uint16_t entry_size;
		} section;
		struct {
			uint16_t section; /* index of the strings section */
			uint64_t size;
			uint64_t offset;
			uint32_t nameoff; /* offset in strings itself */
		} strings;
		uint16_t machine;
	} header;
};

//#undef ENABLE_ELFDBG
//#define ENABLE_ELFDBG 1

#define ELFDBG(elf, ...)                                                          \
	do {                                                                      \
		if (ENABLE_LOGGING == 1 && ENABLE_ELFDBG == 1)                    \
			_elf_dbg(elf, __FILE__, __LINE__, __func__, __VA_ARGS__); \
	} while (0);

_printf_format_(5, 6) static inline void _elf_dbg(const struct kmod_elf *elf,
						  const char *fname, unsigned line,
						  const char *func, const char *fmt, ...)
{
	va_list args;

	fprintf(stderr, "ELFDBG-%d%c: %s:%u %s() ", elf->x32 ? 32 : 64,
		elf->msb ? 'M' : 'L', fname, line, func);
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
}

static int elf_identify(struct kmod_elf *elf, const void *memory, uint64_t size)
{
	const uint8_t *p = memory;

	if (size <= EI_NIDENT || memcmp(p, ELFMAG, SELFMAG) != 0)
		return -ENOEXEC;

	switch (p[EI_CLASS]) {
	case ELFCLASS32:
		if (size <= sizeof(Elf32_Ehdr))
			return -EINVAL;
		elf->x32 = true;
		break;
	case ELFCLASS64:
		if (size <= sizeof(Elf64_Ehdr))
			return -EINVAL;
		elf->x32 = false;
		break;
	default:
		return -EINVAL;
	}

	switch (p[EI_DATA]) {
	case ELFDATA2LSB:
		elf->msb = false;
		break;
	case ELFDATA2MSB:
		elf->msb = true;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static inline bool elf_range_valid(const struct kmod_elf *elf, uint64_t offset,
				   uint64_t size)
{
	uint64_t min_size;

	if (uadd64_overflow(offset, size, &min_size) || min_size > elf->size) {
		ELFDBG(elf,
		       "out of bounds: %" PRIu64 " + %" PRIu64 " > %" PRIu64
		       " (ELF size)\n",
		       offset, size, elf->size);
		return false;
	}
	return true;
}

static inline uint64_t elf_get_uint(const struct kmod_elf *elf, uint64_t offset,
				    uint16_t size)
{
	const uint8_t *p;
	uint64_t ret = 0;

	assert(size <= sizeof(uint64_t));

	p = elf->memory + offset;

	if (elf->msb) {
		memcpy((char *)&ret + sizeof(ret) - size, p, size);
		ret = be64toh(ret);
	} else {
		memcpy(&ret, p, size);
		ret = le64toh(ret);
	}

	ELFDBG(elf, "size=%" PRIu16 " offset=%" PRIu64 " value=%" PRIu64 "\n", size,
	       offset, ret);

	return ret;
}

static inline int elf_set_uint(const struct kmod_elf *elf, uint64_t offset, uint64_t size,
			       uint64_t value, uint8_t *changed)
{
	uint8_t *p;
	size_t i;

	ELFDBG(elf,
	       "size=%" PRIu64 " offset=%" PRIu64 " value=%" PRIu64 " write memory=%p\n",
	       size, offset, value, changed);

	assert(size <= sizeof(uint64_t));

	p = changed + offset;
	if (elf->msb) {
		for (i = 1; i <= size; i++) {
			p[size - i] = value & 0xff;
			value = (value & 0xffffffffffffff00) >> 8;
		}
	} else {
		for (i = 0; i < size; i++) {
			p[i] = value & 0xff;
			value = (value & 0xffffffffffffff00) >> 8;
		}
	}

	return 0;
}

static inline const void *elf_get_mem(const struct kmod_elf *elf, uint64_t offset)
{
	return elf->memory + offset;
}

/*
 * Returns offset to section header for section with given index or 0 on error
 * (offset 0 cannot be a valid section offset because ELF header is located there).
 */
static inline uint64_t elf_get_section_header_offset(const struct kmod_elf *elf,
						     uint16_t idx)
{
	assert(idx != SHN_UNDEF);
	assert(idx < elf->header.section.count);
	if (idx == SHN_UNDEF || idx >= elf->header.section.count) {
		ELFDBG(elf, "invalid section number: %" PRIu16 ", last=%" PRIu16 "\n",
		       idx, elf->header.section.count);
		return 0;
	}
	return elf->header.section.offset +
	       (uint64_t)(idx * elf->header.section.entry_size);
}

static inline int elf_get_section_info(const struct kmod_elf *elf, uint16_t idx,
				       uint64_t *offset, uint64_t *size,
				       uint32_t *nameoff)
{
	uint64_t off = elf_get_section_header_offset(elf, idx);

	if (off == 0) {
		ELFDBG(elf, "no section at %" PRIu16 "\n", idx);
		goto fail;
	}

#define READV(field) \
	elf_get_uint(elf, off + offsetof(typeof(*hdr), field), sizeof(hdr->field))

	if (elf->x32) {
		Elf32_Shdr *hdr;
		if (!elf_range_valid(elf, off, sizeof(*hdr)))
			goto fail;
		*size = READV(sh_size);
		*offset = READV(sh_offset);
		*nameoff = READV(sh_name);
	} else {
		Elf64_Shdr *hdr;
		if (!elf_range_valid(elf, off, sizeof(*hdr)))
			goto fail;
		*size = READV(sh_size);
		*offset = READV(sh_offset);
		*nameoff = READV(sh_name);
	}
#undef READV

	if (!elf_range_valid(elf, *offset, *size))
		goto fail;

	ELFDBG(elf,
	       "section=%" PRIu16 " is: offset=%" PRIu64 " size=%" PRIu64
	       " nameoff=%" PRIu32 "\n",
	       idx, *offset, *size, *nameoff);

	return 0;
fail:
	*offset = 0;
	*size = 0;
	*nameoff = 0;
	return -EINVAL;
}

static const char *elf_get_strings_section(const struct kmod_elf *elf, uint64_t *size)
{
	*size = elf->header.strings.size;
	return elf_get_mem(elf, elf->header.strings.offset);
}

struct kmod_elf *kmod_elf_new(const void *memory, off_t size)
{
	struct kmod_elf *elf;
	uint64_t min_size;
	size_t shdrs_size, shdr_size;
	int err;

	assert_cc(sizeof(uint16_t) == sizeof(Elf32_Half));
	assert_cc(sizeof(uint16_t) == sizeof(Elf64_Half));
	assert_cc(sizeof(uint32_t) == sizeof(Elf32_Word));
	assert_cc(sizeof(uint32_t) == sizeof(Elf64_Word));

	if (!memory) {
		errno = -EINVAL;
		return NULL;
	}

	elf = malloc(sizeof(struct kmod_elf));
	if (elf == NULL) {
		return NULL;
	}

	err = elf_identify(elf, memory, size);
	if (err < 0) {
		free(elf);
		errno = -err;
		return NULL;
	}

	elf->memory = memory;
	elf->size = size;

#define READV(field) elf_get_uint(elf, offsetof(typeof(*hdr), field), sizeof(hdr->field))

#define LOAD_HEADER                                          \
	elf->header.section.offset = READV(e_shoff);         \
	elf->header.section.count = READV(e_shnum);          \
	elf->header.section.entry_size = READV(e_shentsize); \
	elf->header.strings.section = READV(e_shstrndx);     \
	elf->header.machine = READV(e_machine)
	if (elf->x32) {
		Elf32_Ehdr *hdr;
		shdr_size = sizeof(Elf32_Shdr);
		if (!elf_range_valid(elf, 0, shdr_size))
			goto invalid;
		LOAD_HEADER;
	} else {
		Elf64_Ehdr *hdr;
		shdr_size = sizeof(Elf64_Shdr);
		if (!elf_range_valid(elf, 0, shdr_size))
			goto invalid;
		LOAD_HEADER;
	}
#undef LOAD_HEADER
#undef READV

	ELFDBG(elf,
	       "section: offset=%" PRIu64 " count=%" PRIu16 " entry_size=%" PRIu16
	       " strings index=%" PRIu16 "\n",
	       elf->header.section.offset, elf->header.section.count,
	       elf->header.section.entry_size, elf->header.strings.section);

	if (elf->header.section.entry_size != shdr_size) {
		ELFDBG(elf, "unexpected section entry size: %" PRIu16 ", expected %zu\n",
		       elf->header.section.entry_size, shdr_size);
		goto invalid;
	}
	shdrs_size = shdr_size * elf->header.section.count;
	if (uadd64_overflow(shdrs_size, elf->header.section.offset, &min_size) ||
	    min_size > elf->size) {
		ELFDBG(elf, "file is too short to hold sections\n");
		goto invalid;
	}

	if (elf_get_section_info(elf, elf->header.strings.section,
				 &elf->header.strings.offset, &elf->header.strings.size,
				 &elf->header.strings.nameoff) < 0) {
		ELFDBG(elf, "could not get strings section\n");
		goto invalid;
	} else {
		uint64_t slen;
		const char *s = elf_get_strings_section(elf, &slen);
		if (slen == 0 || s[slen - 1] != '\0') {
			ELFDBG(elf, "strings section does not end with \\0\n");
			goto invalid;
		}
	}

	return elf;

invalid:
	free(elf);
	errno = EINVAL;
	return NULL;
}

void kmod_elf_unref(struct kmod_elf *elf)
{
	free(elf);
}

const void *kmod_elf_get_memory(const struct kmod_elf *elf)
{
	return elf->memory;
}

static int elf_find_section(const struct kmod_elf *elf, const char *section)
{
	uint64_t nameslen;
	const char *names = elf_get_strings_section(elf, &nameslen);
	uint16_t i;

	for (i = 1; i < elf->header.section.count; i++) {
		uint64_t off, size;
		uint32_t nameoff;
		const char *n;
		int err = elf_get_section_info(elf, i, &off, &size, &nameoff);
		if (err < 0)
			continue;
		if (nameoff >= nameslen)
			continue;
		n = names + nameoff;
		if (!streq(section, n))
			continue;

		return i;
	}

	return -ENODATA;
}

/* on success, sec_off and sec_size are range checked and valid */
int kmod_elf_get_section(const struct kmod_elf *elf, const char *section,
			 uint64_t *sec_off, uint64_t *sec_size)
{
	uint64_t nameslen;
	const char *names = elf_get_strings_section(elf, &nameslen);
	uint16_t i;

	*sec_off = 0;
	*sec_size = 0;

	for (i = 1; i < elf->header.section.count; i++) {
		uint64_t off, size;
		uint32_t nameoff;
		const char *n;
		int err = elf_get_section_info(elf, i, &off, &size, &nameoff);
		if (err < 0)
			continue;
		if (nameoff >= nameslen)
			continue;
		n = names + nameoff;
		if (!streq(section, n))
			continue;

		*sec_off = off;
		*sec_size = size;
		return 0;
	}

	return -ENODATA;
}

/* array will be allocated with strings in a single malloc, just free *array */
int kmod_elf_get_strings(const struct kmod_elf *elf, const char *section, char ***array)
{
	size_t i, j, count;
	size_t tmp_size, vec_size, total_size;
	uint64_t off, size;
	const char *strings;
	char *s, **a;
	int err;

	*array = NULL;

	err = kmod_elf_get_section(elf, section, &off, &size);
	if (err < 0)
		return err;

	if (size == 0)
		return 0;
	strings = elf_get_mem(elf, off);

	/* skip zero padding */
	while (strings[0] == '\0' && size > 1) {
		strings++;
		size--;
	}

	if (size <= 1)
		return 0;

	for (i = 0, count = 0; i < size;) {
		if (strings[i] != '\0') {
			i++;
			continue;
		}

		while (strings[i] == '\0' && i < size)
			i++;

		count++;
	}

	if (strings[i - 1] != '\0')
		count++;

	/* (string vector + NULL) * sizeof(char *) + size + NUL */
	if (uaddsz_overflow(count, 1, &tmp_size) ||
	    umulsz_overflow(sizeof(char *), tmp_size, &vec_size) ||
	    uaddsz_overflow(size, vec_size, &tmp_size) ||
	    uaddsz_overflow(1, tmp_size, &total_size)) {
		return -ENOMEM;
	}

	*array = a = malloc(total_size);
	if (*array == NULL)
		return -errno;

	s = (char *)(a + count + 1);
	memcpy(s, strings, size);

	/* make sure the last string is NULL-terminated */
	s[size] = '\0';
	a[count] = NULL;
	a[0] = s;

	for (i = 0, j = 1; j < count && i < size;) {
		if (s[i] != '\0') {
			i++;
			continue;
		}

		while (strings[i] == '\0' && i < size)
			i++;

		a[j] = &s[i];
		j++;
	}

	return count;
}

/* array will be allocated with strings in a single malloc, just free *array */
int kmod_elf_get_modversions(const struct kmod_elf *elf, struct kmod_modversion **array)
{
	size_t off, offcrc, slen;
	uint64_t sec_off, size;
	struct kmod_modversion *a;
	char *itr;
	int i, count, err;
#define MODVERSION_SEC_SIZE (sizeof(struct kmod_modversion64))

	assert_cc(sizeof(struct kmod_modversion64) == sizeof(struct kmod_modversion32));

	if (elf->x32)
		offcrc = sizeof(uint32_t);
	else
		offcrc = sizeof(uint64_t);

	*array = NULL;

	err = kmod_elf_get_section(elf, "__versions", &sec_off, &size);
	if (err < 0)
		return err;

	if (size == 0)
		return 0;

	if (size % MODVERSION_SEC_SIZE != 0)
		return -EINVAL;

	count = size / MODVERSION_SEC_SIZE;

	off = sec_off;
	slen = 0;

	for (i = 0; i < count; i++, off += MODVERSION_SEC_SIZE) {
		const char *symbol = elf_get_mem(elf, off + offcrc);

		if (symbol[0] == '.')
			symbol++;

		slen += strlen(symbol) + 1;
	}

	*array = a = malloc(sizeof(struct kmod_modversion) * count + slen);
	if (*array == NULL)
		return -errno;

	itr = (char *)(a + count);
	off = sec_off;

	for (i = 0; i < count; i++, off += MODVERSION_SEC_SIZE) {
		uint64_t crc = elf_get_uint(elf, off, offcrc);
		const char *symbol = elf_get_mem(elf, off + offcrc);
		size_t symbollen;

		if (symbol[0] == '.')
			symbol++;

		a[i].crc = crc;
		a[i].bind = KMOD_SYMBOL_UNDEF;
		a[i].symbol = itr;
		symbollen = strlen(symbol) + 1;
		memcpy(itr, symbol, symbollen);
		itr += symbollen;
	}

	return count;
}

static int elf_strip_versions_section(const struct kmod_elf *elf, uint8_t *changed)
{
	uint64_t off, size;
	const void *buf;
	int idx = elf_find_section(elf, "__versions");
	uint64_t val;

	if (idx < 0)
		return idx == -ENODATA ? 0 : idx;

	off = elf_get_section_header_offset(elf, idx);

	if (elf->x32) {
		off += offsetof(Elf32_Shdr, sh_flags);
		size = sizeof(((Elf32_Shdr *)buf)->sh_flags);
	} else {
		off += offsetof(Elf64_Shdr, sh_flags);
		size = sizeof(((Elf64_Shdr *)buf)->sh_flags);
	}

	val = elf_get_uint(elf, off, size);
	val &= ~(uint64_t)SHF_ALLOC;

	return elf_set_uint(elf, off, size, val, changed);
}

static int elf_strip_vermagic(const struct kmod_elf *elf, uint8_t *changed)
{
	uint64_t i, sec_off, size;
	const char *strings;
	int err;

	err = kmod_elf_get_section(elf, ".modinfo", &sec_off, &size);
	if (err < 0)
		return err == -ENODATA ? 0 : err;
	if (size == 0)
		return 0;
	strings = elf_get_mem(elf, sec_off);

	/* skip zero padding */
	while (strings[0] == '\0' && size > 1) {
		strings++;
		size--;
	}
	if (size <= 1)
		return 0;

	for (i = 0; i < size; i++) {
		const char *s;
		size_t off, len;

		if (strings[i] == '\0')
			continue;
		if (i + 1 >= size)
			continue;

		s = strings + i;
		len = sizeof("vermagic=") - 1;
		if (i + len >= size)
			continue;
		if (strncmp(s, "vermagic=", len) != 0) {
			i += strlen(s);
			continue;
		}
		off = (const uint8_t *)s - elf->memory;

		len = strlen(s);
		ELFDBG(elf, "clear .modinfo vermagic \"%s\" (%zd bytes)\n", s, len);
		memset(changed + off, '\0', len);
		return 0;
	}

	ELFDBG(elf, "no vermagic found in .modinfo\n");
	return -ENODATA;
}

const void *kmod_elf_strip(const struct kmod_elf *elf, unsigned int flags)
{
	uint8_t *changed;
	int err = 0;

	assert(flags & (KMOD_INSERT_FORCE_MODVERSION | KMOD_INSERT_FORCE_VERMAGIC));

	changed = memdup(elf->memory, elf->size);
	if (changed == NULL)
		return NULL;

	ELFDBG(elf, "copied memory to allow writing.\n");

	if (flags & KMOD_INSERT_FORCE_MODVERSION) {
		err = elf_strip_versions_section(elf, changed);
		if (err < 0) {
			errno = -err;
			goto fail;
		}
	}

	if (flags & KMOD_INSERT_FORCE_VERMAGIC) {
		err = elf_strip_vermagic(elf, changed);
		if (err < 0) {
			errno = -err;
			goto fail;
		}
	}

	return changed;
fail:
	free(changed);
	return NULL;
}

static int kmod_elf_get_symbols_symtab(const struct kmod_elf *elf,
				       struct kmod_modversion **array)
{
	uint64_t i, last, off, size;
	const char *strings;
	char *itr;
	struct kmod_modversion *a;
	int count, err;

	*array = NULL;

	err = kmod_elf_get_section(elf, "__ksymtab_strings", &off, &size);
	if (err < 0)
		return err;
	if (size == 0)
		return 0;
	strings = elf_get_mem(elf, off);

	/* skip zero padding */
	while (strings[0] == '\0' && size > 1) {
		strings++;
		size--;
	}
	if (size <= 1)
		return 0;

	last = 0;
	for (i = 0, count = 0; i < size; i++) {
		if (strings[i] == '\0') {
			if (last == i) {
				last = i + 1;
				continue;
			}
			count++;
			last = i + 1;
		}
	}
	if (strings[i - 1] != '\0')
		count++;

	*array = a = malloc(size + 1 + sizeof(struct kmod_modversion) * count);
	if (*array == NULL)
		return -errno;

	itr = (char *)(a + count);
	last = 0;
	for (i = 0, count = 0; i < size; i++) {
		if (strings[i] == '\0') {
			size_t slen = i - last;
			if (last == i) {
				last = i + 1;
				continue;
			}
			a[count].crc = 0;
			a[count].bind = KMOD_SYMBOL_GLOBAL;
			a[count].symbol = itr;
			memcpy(itr, strings + last, slen);
			itr[slen] = '\0';
			itr += slen + 1;
			count++;
			last = i + 1;
		}
	}
	if (strings[i - 1] != '\0') {
		size_t slen = i - last;
		a[count].crc = 0;
		a[count].bind = KMOD_SYMBOL_GLOBAL;
		a[count].symbol = itr;
		memcpy(itr, strings + last, slen);
		itr[slen] = '\0';
		count++;
	}

	return count;
}

static inline uint8_t kmod_symbol_bind_from_elf(uint8_t elf_value)
{
	switch (elf_value) {
	case STB_LOCAL:
		return KMOD_SYMBOL_LOCAL;
	case STB_GLOBAL:
		return KMOD_SYMBOL_GLOBAL;
	case STB_WEAK:
		return KMOD_SYMBOL_WEAK;
	default:
		return KMOD_SYMBOL_NONE;
	}
}

static uint64_t kmod_elf_resolve_crc(const struct kmod_elf *elf, uint64_t crc,
				     uint16_t shndx)
{
	int err;
	uint64_t off, size;
	uint32_t nameoff;

	if (shndx == SHN_ABS || shndx == SHN_UNDEF)
		return crc;

	err = elf_get_section_info(elf, shndx, &off, &size, &nameoff);
	if (err < 0) {
		ELFDBG(elf, "Could not find section index %" PRIu16 " for crc", shndx);
		return (uint64_t)-1;
	}

	if (size < sizeof(uint32_t) || crc > (size - sizeof(uint32_t))) {
		ELFDBG(elf,
		       "CRC offset %" PRIu64 " is too big, section %" PRIu16
		       " size is %" PRIu64 "\n",
		       crc, shndx, size);
		return (uint64_t)-1;
	}

	crc = elf_get_uint(elf, off + crc, sizeof(uint32_t));
	return crc;
}

/* array will be allocated with strings in a single malloc, just free *array */
int kmod_elf_get_symbols(const struct kmod_elf *elf, struct kmod_modversion **array)
{
	static const char crc_str[] = "__crc_";
	static const size_t crc_strlen = sizeof(crc_str) - 1;
	uint64_t strtablen, symtablen, str_sec_off, sym_sec_off, str_off, sym_off;
	struct kmod_modversion *a;
	char *itr;
	size_t i, count, symcount, slen, symlen;
	int err;

	err = kmod_elf_get_section(elf, ".strtab", &str_sec_off, &strtablen);
	if (err < 0) {
		ELFDBG(elf, "no .strtab found.\n");
		goto fallback;
	}

	err = kmod_elf_get_section(elf, ".symtab", &sym_sec_off, &symtablen);
	if (err < 0) {
		ELFDBG(elf, "no .symtab found.\n");
		goto fallback;
	}

	if (elf->x32)
		symlen = sizeof(Elf32_Sym);
	else
		symlen = sizeof(Elf64_Sym);

	if (symtablen % symlen != 0) {
		ELFDBG(elf,
		       "unexpected .symtab of length %" PRIu64
		       ", not multiple of %" PRIu64 " as expected.\n",
		       symtablen, symlen);
		goto fallback;
	}

	symcount = symtablen / symlen;
	count = 0;
	slen = 0;
	str_off = str_sec_off;
	sym_off = sym_sec_off;
	for (i = 1; i < symcount; i++, sym_off += symlen) {
		const char *name;
		uint32_t name_off;

#define READV(field) \
	elf_get_uint(elf, sym_off + offsetof(typeof(*s), field), sizeof(s->field))
		if (elf->x32) {
			Elf32_Sym *s;
			name_off = READV(st_name);
		} else {
			Elf64_Sym *s;
			name_off = READV(st_name);
		}
#undef READV
		if (name_off >= strtablen) {
			ELFDBG(elf,
			       ".strtab is %" PRIu64 " bytes, but .symtab entry %" PRIu64
			       " wants to access offset %" PRIu32 ".\n",
			       strtablen, i, name_off);
			goto fallback;
		}

		name = elf_get_mem(elf, str_off + name_off);

		if (strncmp(name, crc_str, crc_strlen) != 0)
			continue;
		slen += strlen(name + crc_strlen) + 1;
		count++;
	}

	if (count == 0)
		goto fallback;

	*array = a = malloc(sizeof(struct kmod_modversion) * count + slen);
	if (*array == NULL)
		return -errno;

	itr = (char *)(a + count);
	count = 0;
	str_off = str_sec_off;
	sym_off = sym_sec_off;
	for (i = 1; i < symcount; i++, sym_off += symlen) {
		const char *name;
		uint32_t name_off;
		uint64_t crc;
		uint8_t info, bind;
		uint16_t shndx;

#define READV(field) \
	elf_get_uint(elf, sym_off + offsetof(typeof(*s), field), sizeof(s->field))
		if (elf->x32) {
			Elf32_Sym *s;
			name_off = READV(st_name);
			crc = READV(st_value);
			info = READV(st_info);
			shndx = READV(st_shndx);
		} else {
			Elf64_Sym *s;
			name_off = READV(st_name);
			crc = READV(st_value);
			info = READV(st_info);
			shndx = READV(st_shndx);
		}
#undef READV
		name = elf_get_mem(elf, str_off + name_off);
		if (strncmp(name, crc_str, crc_strlen) != 0)
			continue;
		name += crc_strlen;

		if (elf->x32)
			bind = ELF32_ST_BIND(info);
		else
			bind = ELF64_ST_BIND(info);

		a[count].crc = kmod_elf_resolve_crc(elf, crc, shndx);
		a[count].bind = kmod_symbol_bind_from_elf(bind);
		a[count].symbol = itr;
		slen = strlen(name);
		memcpy(itr, name, slen);
		itr[slen] = '\0';
		itr += slen + 1;
		count++;
	}
	return count;

fallback:
	ELFDBG(elf, "Falling back to __ksymtab_strings!\n");
	return kmod_elf_get_symbols_symtab(elf, array);
}

static int kmod_elf_crc_find(const struct kmod_elf *elf, uint64_t off,
			     uint64_t versionslen, const char *name, uint64_t *crc)
{
	size_t verlen, crclen;
	uint64_t i;

	if (elf->x32) {
		struct kmod_modversion32 *mv;
		verlen = sizeof(*mv);
		crclen = sizeof(mv->crc);
	} else {
		struct kmod_modversion64 *mv;
		verlen = sizeof(*mv);
		crclen = sizeof(mv->crc);
	}

	for (i = 0; i < versionslen; i += verlen) {
		const char *symbol = elf_get_mem(elf, off + i + crclen);
		if (!streq(name, symbol))
			continue;
		*crc = elf_get_uint(elf, off + i, crclen);
		return i / verlen;
	}

	ELFDBG(elf, "could not find crc for symbol '%s'\n", name);
	*crc = 0;
	return -1;
}

/* from module-init-tools:elfops_core.c */
#ifndef STT_REGISTER
#define STT_REGISTER 13 /* Global register reserved to app. */
#endif

/* array will be allocated with strings in a single malloc, just free *array */
int kmod_elf_get_dependency_symbols(const struct kmod_elf *elf,
				    struct kmod_modversion **array)
{
	uint64_t versionslen, strtablen, symtablen, str_off, sym_off, ver_off;
	uint64_t str_sec_off, sym_sec_off;
	struct kmod_modversion *a;
	char *itr;
	size_t slen, verlen, symlen, crclen;
	int i, count, symcount, vercount, err;
	bool handle_register_symbols;
	uint8_t *visited_versions;
	uint64_t *symcrcs;

	err = kmod_elf_get_section(elf, "__versions", &ver_off, &versionslen);
	if (err < 0) {
		versionslen = 0;
		verlen = 0;
		crclen = 0;
	} else {
		if (elf->x32) {
			struct kmod_modversion32 *mv;
			verlen = sizeof(*mv);
			crclen = sizeof(mv->crc);
		} else {
			struct kmod_modversion64 *mv;
			verlen = sizeof(*mv);
			crclen = sizeof(mv->crc);
		}
		if (versionslen % verlen != 0) {
			ELFDBG(elf,
			       "unexpected __versions of length %" PRIu64
			       ", not multiple of %zd as expected.\n",
			       versionslen, verlen);
			ver_off = 0;
			versionslen = 0;
		}
	}

	err = kmod_elf_get_section(elf, ".strtab", &str_sec_off, &strtablen);
	if (err < 0) {
		ELFDBG(elf, "no .strtab found.\n");
		return -EINVAL;
	}

	err = kmod_elf_get_section(elf, ".symtab", &sym_sec_off, &symtablen);
	if (err < 0) {
		ELFDBG(elf, "no .symtab found.\n");
		return -EINVAL;
	}

	if (elf->x32)
		symlen = sizeof(Elf32_Sym);
	else
		symlen = sizeof(Elf64_Sym);

	if (symtablen % symlen != 0) {
		ELFDBG(elf,
		       "unexpected .symtab of length %" PRIu64
		       ", not multiple of %" PRIu64 " as expected.\n",
		       symtablen, symlen);
		return -EINVAL;
	}

	if (versionslen == 0) {
		vercount = 0;
		visited_versions = NULL;
	} else {
		vercount = versionslen / verlen;
		visited_versions = calloc(vercount, sizeof(uint8_t));
		if (visited_versions == NULL)
			return -ENOMEM;
	}

	handle_register_symbols =
		(elf->header.machine == EM_SPARC || elf->header.machine == EM_SPARCV9);

	symcount = symtablen / symlen;
	count = 0;
	slen = 0;
	str_off = str_sec_off;
	sym_off = sym_sec_off + symlen;

	symcrcs = calloc(symcount, sizeof(uint64_t));
	if (symcrcs == NULL) {
		free(visited_versions);
		return -ENOMEM;
	}

	for (i = 1; i < symcount; i++, sym_off += symlen) {
		const char *name;
		uint64_t crc;
		uint32_t name_off;
		uint16_t secidx;
		uint8_t info;
		int idx;

#define READV(field) \
	elf_get_uint(elf, sym_off + offsetof(typeof(*s), field), sizeof(s->field))
		if (elf->x32) {
			Elf32_Sym *s;
			name_off = READV(st_name);
			secidx = READV(st_shndx);
			info = READV(st_info);
		} else {
			Elf64_Sym *s;
			name_off = READV(st_name);
			secidx = READV(st_shndx);
			info = READV(st_info);
		}
#undef READV
		if (secidx != SHN_UNDEF)
			continue;

		if (handle_register_symbols) {
			uint8_t type;
			if (elf->x32)
				type = ELF32_ST_TYPE(info);
			else
				type = ELF64_ST_TYPE(info);

			/* Not really undefined: sparc gcc 3.3 creates
			 * U references when you have global asm
			 * variables, to avoid anyone else misusing
			 * them.
			 */
			if (type == STT_REGISTER)
				continue;
		}

		if (name_off >= strtablen) {
			ELFDBG(elf,
			       ".strtab is %" PRIu64
			       " bytes, but .symtab entry %d wants to access offset %" PRIu32
			       ".\n",
			       strtablen, i, name_off);
			free(visited_versions);
			free(symcrcs);
			return -EINVAL;
		}

		name = elf_get_mem(elf, str_off + name_off);
		if (name[0] == '\0') {
			ELFDBG(elf, "empty symbol name at index %d\n", i);
			continue;
		}

		slen += strlen(name) + 1;
		count++;

		idx = kmod_elf_crc_find(elf, ver_off, versionslen, name, &crc);
		if (idx >= 0 && visited_versions != NULL)
			visited_versions[idx] = 1;
		symcrcs[i] = crc;
	}

	if (visited_versions != NULL) {
		/* module_layout/struct_module are not visited, but needed */
		for (i = 0; i < vercount; i++) {
			if (visited_versions[i] == 0) {
				const char *name;
				name = elf_get_mem(elf, ver_off + i * verlen + crclen);
				slen += strlen(name) + 1;

				count++;
			}
		}
	}

	if (count == 0) {
		free(visited_versions);
		free(symcrcs);
		*array = NULL;
		return 0;
	}

	*array = a = malloc(sizeof(struct kmod_modversion) * count + slen);
	if (*array == NULL) {
		free(visited_versions);
		free(symcrcs);
		return -errno;
	}

	itr = (char *)(a + count);
	count = 0;
	str_off = str_sec_off;
	sym_off = sym_sec_off + symlen;
	for (i = 1; i < symcount; i++, sym_off += symlen) {
		const char *name;
		uint64_t crc;
		uint32_t name_off;
		uint16_t secidx;
		uint8_t info, bind;

#define READV(field) \
	elf_get_uint(elf, sym_off + offsetof(typeof(*s), field), sizeof(s->field))
		if (elf->x32) {
			Elf32_Sym *s;
			name_off = READV(st_name);
			secidx = READV(st_shndx);
			info = READV(st_info);
		} else {
			Elf64_Sym *s;
			name_off = READV(st_name);
			secidx = READV(st_shndx);
			info = READV(st_info);
		}
#undef READV
		if (secidx != SHN_UNDEF)
			continue;

		if (handle_register_symbols) {
			uint8_t type;
			if (elf->x32)
				type = ELF32_ST_TYPE(info);
			else
				type = ELF64_ST_TYPE(info);

			/* Not really undefined: sparc gcc 3.3 creates
			 * U references when you have global asm
			 * variables, to avoid anyone else misusing
			 * them.
			 */
			if (type == STT_REGISTER)
				continue;
		}

		name = elf_get_mem(elf, str_off + name_off);
		if (name[0] == '\0') {
			ELFDBG(elf, "empty symbol name at index %d\n", i);
			continue;
		}

		if (elf->x32)
			bind = ELF32_ST_BIND(info);
		else
			bind = ELF64_ST_BIND(info);
		if (bind == STB_WEAK)
			bind = KMOD_SYMBOL_WEAK;
		else
			bind = KMOD_SYMBOL_UNDEF;

		slen = strlen(name);
		crc = symcrcs[i];

		a[count].crc = crc;
		a[count].bind = bind;
		a[count].symbol = itr;
		memcpy(itr, name, slen);
		itr[slen] = '\0';
		itr += slen + 1;

		count++;
	}

	free(symcrcs);

	if (visited_versions == NULL)
		return count;

	/* add unvisited (module_layout/struct_module) */
	for (i = 0; i < vercount; i++) {
		const char *name;
		uint64_t crc;

		if (visited_versions[i] != 0)
			continue;

		name = elf_get_mem(elf, ver_off + i * verlen + crclen);
		slen = strlen(name);
		crc = elf_get_uint(elf, ver_off + i * verlen, crclen);

		a[count].crc = crc;
		a[count].bind = KMOD_SYMBOL_UNDEF;
		a[count].symbol = itr;
		memcpy(itr, name, slen);
		itr[slen] = '\0';
		itr += slen + 1;

		count++;
	}
	free(visited_versions);
	return count;
}
