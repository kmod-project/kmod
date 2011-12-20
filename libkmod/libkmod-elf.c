/*
 * libkmod - interface to kernel module operations
 *
 * Copyright (C) 2011  ProFUSION embedded systems
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <elf.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "libkmod.h"
#include "libkmod-private.h"

enum kmod_elf_class {
	KMOD_ELF_32 = (1 << 1),
	KMOD_ELF_64 = (1 << 2),
	KMOD_ELF_LSB = (1 << 3),
	KMOD_ELF_MSB = (1 << 4)
};

#ifdef WORDS_BIGENDIAN
static const enum kmod_elf_class native_endianess = KMOD_ELF_MSB;
#else
static const enum kmod_elf_class native_endianess = KMOD_ELF_LSB;
#endif

struct kmod_elf {
	const uint8_t *memory;
	uint8_t *changed;
	uint64_t size;
	enum kmod_elf_class class;
	struct kmod_elf_header {
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
	} header;
};

//#define ENABLE_ELFDBG 1

#if defined(ENABLE_LOGGING) && defined(ENABLE_ELFDBG)
#define ELFDBG(elf, ...)			\
	_elf_dbg(elf, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__);

static inline void _elf_dbg(const struct kmod_elf *elf, const char *fname, unsigned line, const char *func, const char *fmt, ...)
{
	va_list args;

	fprintf(stderr, "ELFDBG-%d%c: %s:%u %s() ",
		(elf->class & KMOD_ELF_32) ? 32 : 64,
		(elf->class & KMOD_ELF_MSB) ? 'M' : 'L',
		fname, line, func);
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
}
#else
#define ELFDBG(elf, ...)
#endif


static int elf_identify(const void *memory, uint64_t size)
{
	const uint8_t *p = memory;
	int class = 0;

	if (size <= EI_NIDENT || memcmp(p, ELFMAG, SELFMAG) != 0)
		return -ENOEXEC;

	switch (p[EI_CLASS]) {
	case ELFCLASS32:
		if (size <= sizeof(Elf32_Ehdr))
			return -EINVAL;
		class |= KMOD_ELF_32;
		break;
	case ELFCLASS64:
		if (size <= sizeof(Elf64_Ehdr))
			return -EINVAL;
		class |= KMOD_ELF_64;
		break;
	default:
		return -EINVAL;
	}

	switch (p[EI_DATA]) {
	case ELFDATA2LSB:
		class |= KMOD_ELF_LSB;
		break;
	case ELFDATA2MSB:
		class |= KMOD_ELF_MSB;
		break;
	default:
		return -EINVAL;
	}

	return class;
}

static inline uint64_t elf_get_uint(const struct kmod_elf *elf, uint64_t offset, uint16_t size)
{
	const uint8_t *p;
	uint64_t ret = 0;
	size_t i;

	assert(size <= sizeof(uint64_t));
	assert(offset + size <= elf->size);
	if (offset + size > elf->size) {
		ELFDBG(elf, "out of bounds: %"PRIu64" + %"PRIu16" = %"PRIu64"> %"PRIu64" (ELF size)\n",
		       offset, size, offset + size, elf->size);
		return (uint64_t)-1;
	}

	p = elf->memory + offset;
	if (elf->class & KMOD_ELF_MSB) {
		for (i = 0; i < size; i++)
			ret = (ret << 8) | p[i];
	} else {
		for (i = 1; i <= size; i++)
			ret = (ret << 8) | p[size - i];
	}

	ELFDBG(elf, "size=%"PRIu16" offset=%"PRIu64" value=%"PRIu64"\n",
	       size, offset, ret);

	return ret;
}

static inline int elf_set_uint(struct kmod_elf *elf, uint64_t offset, uint64_t size, uint64_t value)
{
	uint8_t *p;
	size_t i;

	ELFDBG(elf, "size=%"PRIu16" offset=%"PRIu64" value=%"PRIu64" write memory=%p\n",
	       size, offset, value, elf->changed);

	assert(size <= sizeof(uint64_t));
	assert(offset + size <= elf->size);
	if (offset + size > elf->size) {
		ELFDBG(elf, "out of bounds: %"PRIu64" + %"PRIu16" = %"PRIu64"> %"PRIu64" (ELF size)\n",
		       offset, size, offset + size, elf->size);
		return -1;
	}

	if (elf->changed == NULL) {
		elf->changed = malloc(elf->size);
		if (elf->changed == NULL)
			return -errno;
		memcpy(elf->changed, elf->memory, elf->size);
		elf->memory = elf->changed;
		ELFDBG(elf, "copied memory to allow writing.\n");
	}

	p = elf->changed + offset;
	if (elf->class & KMOD_ELF_MSB) {
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
	assert(offset < elf->size);
	if (offset >= elf->size) {
		ELFDBG(elf, "out-of-bounds: %"PRIu64" >= %"PRIu64" (ELF size)\n",
		       offset, elf->size);
		return NULL;
	}
	return elf->memory + offset;
}

static inline const void *elf_get_section_header(const struct kmod_elf *elf, uint16_t idx)
{
	assert(idx != SHN_UNDEF);
	assert(idx < elf->header.section.count);
	if (idx == SHN_UNDEF || idx >= elf->header.section.count) {
		ELFDBG(elf, "invalid section number: %"PRIu16", last=%"PRIu16"\n",
		       idx, elf->header.section.count);
		return NULL;
	}
	return elf_get_mem(elf, elf->header.section.offset +
			   idx * elf->header.section.entry_size);
}

static inline int elf_get_section_info(const struct kmod_elf *elf, uint16_t idx, uint64_t *offset, uint64_t *size, uint32_t *nameoff)
{
	const uint8_t *p = elf_get_section_header(elf, idx);
	uint64_t min_size, off = p - elf->memory;

	if (p == NULL) {
		ELFDBG(elf, "no section at %"PRIu16"\n", idx);
		*offset = 0;
		*size = 0;
		*nameoff = 0;
		return -EINVAL;
	}

#define READV(field) \
	elf_get_uint(elf, off + offsetof(typeof(*hdr), field), sizeof(hdr->field))

	if (elf->class & KMOD_ELF_32) {
		const Elf32_Shdr *hdr = (const Elf32_Shdr *)p;
		*size = READV(sh_size);
		*offset = READV(sh_offset);
		*nameoff = READV(sh_name);
	} else {
		const Elf64_Shdr *hdr = (const Elf64_Shdr *)p;
		*size = READV(sh_size);
		*offset = READV(sh_offset);
		*nameoff = READV(sh_name);
	}
#undef READV

	min_size = *offset + *size;
	if (min_size >= elf->size) {
		ELFDBG(elf, "out-of-bounds: %"PRIu64" >= %"PRIu64" (ELF size)\n",
		       min_size, elf->size);
		return -EINVAL;
	}

	ELFDBG(elf, "section=%"PRIu16" is: offset=%"PRIu64" size=%"PRIu64" nameoff=%"PRIu32"\n",
	       idx, *offset, *size, *nameoff);

	return 0;
}

static const char *elf_get_strings_section(const struct kmod_elf *elf, uint64_t *size)
{
	*size = elf->header.strings.size;
	return elf_get_mem(elf, elf->header.strings.offset);
}

struct kmod_elf *kmod_elf_new(const void *memory, off_t size)
{
	struct kmod_elf *elf;
	size_t hdr_size, shdr_size, min_size;
	int class;

	assert(sizeof(uint16_t) == sizeof(Elf32_Half));
	assert(sizeof(uint16_t) == sizeof(Elf64_Half));
	assert(sizeof(uint32_t) == sizeof(Elf32_Word));
	assert(sizeof(uint32_t) == sizeof(Elf64_Word));

	class = elf_identify(memory, size);
	if (class < 0) {
		errno = -class;
		return NULL;
	}

	elf = malloc(sizeof(struct kmod_elf));
	if (elf == NULL) {
		return NULL;
	}

	elf->memory = memory;
	elf->changed = NULL;
	elf->size = size;
	elf->class = class;

#define READV(field) \
	elf_get_uint(elf, offsetof(typeof(*hdr), field), sizeof(hdr->field))

#define LOAD_HEADER						\
	elf->header.section.offset = READV(e_shoff);		\
	elf->header.section.count = READV(e_shnum);		\
	elf->header.section.entry_size = READV(e_shentsize);	\
	elf->header.strings.section = READV(e_shstrndx)
	if (elf->class & KMOD_ELF_32) {
		const Elf32_Ehdr *hdr = elf_get_mem(elf, 0);
		LOAD_HEADER;
		hdr_size = sizeof(Elf32_Ehdr);
		shdr_size = sizeof(Elf32_Shdr);
	} else {
		const Elf64_Ehdr *hdr = elf_get_mem(elf, 0);
		LOAD_HEADER;
		hdr_size = sizeof(Elf64_Ehdr);
		shdr_size = sizeof(Elf64_Shdr);
	}
#undef LOAD_HEADER
#undef READV

	ELFDBG(elf, "section: offset=%"PRIu64" count=%"PRIu16" entry_size=%"PRIu16" strings index=%"PRIu16"\n",
	       elf->header.section.offset,
	       elf->header.section.count,
	       elf->header.section.entry_size,
	       elf->header.strings.section);

	if (elf->header.section.entry_size != shdr_size) {
		ELFDBG(elf, "unexpected section entry size: %"PRIu16", expected %"PRIu16"\n",
		       elf->header.section.entry_size, shdr_size);
		goto invalid;
	}
	min_size = hdr_size + shdr_size * elf->header.section.count;
	if (min_size >= elf->size) {
		ELFDBG(elf, "file is too short to hold sections\n");
		goto invalid;
	}

	if (elf_get_section_info(elf, elf->header.strings.section,
					&elf->header.strings.offset,
					&elf->header.strings.size,
					&elf->header.strings.nameoff) < 0) {
		ELFDBG(elf, "could not get strings section\n");
		goto invalid;
	} else {
		uint64_t slen;
		const char *s = elf_get_strings_section(elf, &slen);
		if (slen == 0 || s[slen - 1] != '\0') {
			ELFDBG(elf, "strings section does not ends with \\0\n");
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
	free(elf->changed);
	free(elf);
}

const void *kmod_elf_get_memory(const struct kmod_elf *elf)
{
	return elf->memory;
}

static int kmod_elf_get_section(const struct kmod_elf *elf, const char *section, const void **buf, size_t *buf_size)
{
	uint64_t nameslen;
	const char *names = elf_get_strings_section(elf, &nameslen);
	uint16_t i;

	*buf = NULL;
	*buf_size = 0;

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

		*buf = elf_get_mem(elf, off);
		*buf_size = size;
		return 0;
	}

	return -ENOENT;
}

/* array will be allocated with strings in a single malloc, just free *array */
int kmod_elf_get_strings(const struct kmod_elf *elf, const char *section, char ***array)
{
	uint64_t i, last, size;
	const void *buf;
	const char *strings;
	char *itr, **a;
	int count, err;

	*array = NULL;

	err = kmod_elf_get_section(elf, section, &buf, &size);
	if (err < 0)
		return err;

	strings = buf;
	if (strings == NULL || size == 0)
		return 0;

	/* skip zero padding */
	while (strings[0] == '\0' && size > 1) {
		strings++;
		size--;
	}

	if (size <= 1)
		return 0;

	last = 0;
	for (i = 0, count = 0; i < size; i++) {
		if (strings[i] != '\0')
			continue;

		if (last == i) {
			last = i + 1;
			continue;
		}

		count++;
		last = i + 1;
	}

	if (strings[i - 1] != '\0')
		count++;

	*array = a = malloc(size + 1 + sizeof(char *) * (count + 1));
	if (*array == NULL)
		return -errno;

	a[count] = NULL;
	itr = (char *)(a + count);
	last = 0;

	for (i = 0, count = 0; i < size; i++) {
		if (strings[i] == '\0') {
			size_t slen = i - last;
			if (last == i) {
				last = i + 1;
				continue;
			}
			a[count] = itr;
			memcpy(itr, strings + last, slen);
			itr[slen] = '\0';
			itr += slen + 1;
			count++;
			last = i + 1;
		}
	}

	if (strings[i - 1] != '\0') {
		size_t slen = i - last;
		a[count] = itr;
		memcpy(itr, strings + last, slen);
		itr[slen] = '\0';
		itr += slen + 1;
		count++;
	}

	return count;
}

/* array will be allocated with strings in a single malloc, just free *array */
int kmod_elf_get_modversions(const struct kmod_elf *elf, struct kmod_modversion **array)
{
	uint64_t size, secsize, slen, off;
	struct kmod_modversion *a;
	const void *buf;
	char *itr;
	int i, count, err;
	struct kmod_modversion32 {
		uint32_t crc;
		char name[64 - sizeof(uint32_t)];
	};
	struct kmod_modversion64 {
		uint64_t crc;
		char name[64 - sizeof(uint64_t)];
	};

	*array = NULL;

	err = kmod_elf_get_section(elf, "__versions", &buf, &size);
	if (err < 0)
		return err;
	if (buf == NULL || size == 0)
		return 0;

	if (elf->class & KMOD_ELF_32)
		secsize = sizeof(struct kmod_modversion32);
	else
		secsize = sizeof(struct kmod_modversion64);

	if (size % secsize != 0)
		return -EINVAL;
	count = size / secsize;

	off = (const uint8_t *)buf - elf->memory;
	slen = 0;
	for (i = 0; i < count; i++, off += secsize) {
		const char *symbol;
		if (elf->class & KMOD_ELF_32) {
			struct kmod_modversion32 *mv;
			symbol = elf_get_mem(elf, off + sizeof(mv->crc));
		} else {
			struct kmod_modversion64 *mv;
			symbol = elf_get_mem(elf, off + sizeof(mv->crc));
		}
		if (symbol[0] == '.')
			symbol++;
		slen += strlen(symbol) + 1;
	}

	*array = a = malloc(sizeof(struct kmod_modversion) * count + slen);
	if (*array == NULL)
		return -errno;

	itr = (char *)(a + count);
	off = (const uint8_t *)buf - elf->memory;
	for (i = 0; i < count; i++, off += secsize) {
		uint64_t crc;
		const char *symbol;
		size_t symbollen;
		if (elf->class & KMOD_ELF_32) {
			struct kmod_modversion32 *mv;
			crc = elf_get_uint(elf, off, sizeof(mv->crc));
			symbol = elf_get_mem(elf, off + sizeof(mv->crc));
		} else {
			struct kmod_modversion64 *mv;
			crc = elf_get_uint(elf, off, sizeof(mv->crc));
			symbol = elf_get_mem(elf, off + sizeof(mv->crc));
		}
		if (symbol[0] == '.')
			symbol++;

		a[i].crc = crc;
		a[i].symbol = itr;
		symbollen = strlen(symbol) + 1;
		memcpy(itr, symbol, symbollen);
		itr += symbollen;
	}

	return count;
}

int kmod_elf_strip_section(struct kmod_elf *elf, const char *section)
{
	uint64_t size, off;
	const void *buf;
	int err = kmod_elf_get_section(elf, section, &buf, &size);
	if (err < 0)
		return err;

	off = (const uint8_t *)buf - elf->memory;

#define WRITEV(field, value)			\
	elf_set_uint(elf, off + offsetof(typeof(*hdr), field), sizeof(hdr->field), value)
	if (elf->class & KMOD_ELF_32) {
		const Elf32_Shdr *hdr = buf;
		uint32_t val = ~(uint32_t)SHF_ALLOC;
		return WRITEV(sh_flags, val);
	} else {
		const Elf64_Shdr *hdr = buf;
		uint64_t val = ~(uint64_t)SHF_ALLOC;
		return WRITEV(sh_flags, val);
	}
#undef WRITEV
}

int kmod_elf_strip_vermagic(struct kmod_elf *elf)
{
	uint64_t i, size;
	const void *buf;
	const char *strings;
	int err;

	err = kmod_elf_get_section(elf, ".modinfo", &buf, &size);
	if (err < 0)
		return err;
	strings = buf;
	if (strings == NULL || size == 0)
		return 0;

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
		s += len;
		off = (const uint8_t *)s - elf->memory;

		if (elf->changed == NULL) {
			elf->changed = malloc(elf->size);
			if (elf->changed == NULL)
				return -errno;
			memcpy(elf->changed, elf->memory, elf->size);
			elf->memory = elf->changed;
			ELFDBG(elf, "copied memory to allow writing.\n");
		}

		len = strlen(s);
		ELFDBG(elf, "clear .modinfo vermagic \"%s\" (%zd bytes)\n",
		       s, len);
		memset(elf->changed + off, '\0', len);
		return 0;
	}

	ELFDBG(elf, "no vermagic found in .modinfo\n");
	return -ENOENT;
}
