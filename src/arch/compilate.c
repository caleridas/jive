/*
 * Copyright 2012 Helge Bahmann <hcb@chaoticmind.net>
 * Copyright 2011 2012 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include <jive/arch/compilate.h>

#include <fcntl.h>
#include <sys/mman.h>

static void jive_section_init(jive_section * self, jive_context * context,
	jive_stdsectionid sectionid)
{
	self->id = sectionid;
	jive_buffer_init(&self->contents, context);
	self->relocations.first = self->relocations.last = 0;
}

static void jive_section_clear(jive_section * self)
{
	jive_buffer_resize(&self->contents, 0);
	jive_relocation_entry * entry, * saved_entry;
	JIVE_LIST_ITERATE_SAFE(self->relocations, entry, saved_entry, section_relocation_list) {
		jive_context_free(self->contents.context, entry);
	}
}

static void jive_section_fini(jive_section * self)
{
	jive_section_clear(self);
	jive_buffer_fini(&self->contents);
}

void
jive_section_put_reloc(jive_section * self, const void * data, size_t size,
	jive_relocation_type type, jive_relocation_target target,
	jive_offset value)
{
	jive_offset offset = self->contents.size;
	jive_section_put(self, data, size);
	
	jive_context * context = self->contents.context;
	
	jive_relocation_entry * entry = jive_context_malloc(context,
		sizeof(*entry));
	entry->offset = offset;
	entry->type = type;
	entry->target = target;
	entry->value = value;
	JIVE_LIST_PUSH_BACK(self->relocations, entry, section_relocation_list);
}


/* round up size of section to next multiple of 4096 (which is assumed
 to be the page size... */
static size_t jive_section_size_roundup(const jive_section * self)
{
	size_t size = self->contents.size;
	return (size + 4095) & ~4095;
}

void
jive_compilate_init(struct jive_compilate * self, struct jive_context * context)
{
	self->context = context;
	self->sections.first = self->sections.last = 0;
}

void
jive_compilate_fini(struct jive_compilate * self)
{
	jive_section * section, * saved_section;
	JIVE_LIST_ITERATE_SAFE(self->sections, section, saved_section, compilate_section_list) {
		jive_section_fini(section);
		jive_context_free(self->context, section);
	}
}

void
jive_compilate_clear(jive_compilate * self)
{
	jive_section * section;
	JIVE_LIST_ITERATE(self->sections, section, compilate_section_list) {
		jive_section_clear(section);
	}
}

jive_section *
jive_compilate_get_standard_section(jive_compilate * self,
	jive_stdsectionid sectionid)
{
	if ((int) sectionid <= 0)
		return NULL;
	
	jive_section * section;
	JIVE_LIST_ITERATE(self->sections, section, compilate_section_list) {
		if (section->id == sectionid)
			return section;
	}
	
	section = jive_context_malloc(self->context, sizeof(*section));
	jive_section_init(section, self->context, sectionid);
	JIVE_LIST_PUSH_BACK(self->sections, section, compilate_section_list);
	
	return section;
}

jive_buffer *
jive_compilate_get_buffer(struct jive_compilate * self, jive_stdsectionid id)
{
	jive_section * section = jive_compilate_get_standard_section(self, id);
	if (section)
		return &section->contents;
	else
		return 0;
}

static int
get_tmpfd(size_t size)
{
	char template[] = "/tmp/jive-exec-buffer-XXXXXX";
#if defined(_GNU_SOURCE) && defined(O_CLOEXEC)
	int fd = mkostemp(template, O_CLOEXEC);
#else
	int fd = mkstemp(template);
	if (fd >= 0) fcntl(fd, F_SETFD, FD_CLOEXEC);
#endif
	if (fd < 0)
		return -1;
	unlink(template);
	
	if (ftruncate(fd, size) != 0) {
		close(fd);
		return fd = -1;
	}
	return fd;
}

static jive_compilate_map *
jive_compilate_map_alloc(size_t section_count)
{
	jive_compilate_map * map = malloc(sizeof(*map));
	if (!map)
		return NULL;
	map->sections = malloc(sizeof(map->sections[0]) * section_count);
	if (!map->sections) {
		free(map);
		return NULL;
	}
	map->nsections = section_count;
	return map;
}

void
jive_compilate_map_destroy(jive_compilate_map * self)
{
	free(self->sections);
	free(self);
}

static bool
resolve_relocation_target(
	jive_relocation_target target,
	const jive_compilate_map * map,
	void ** resolved)
{
	switch (target.type) {
		case jive_relocation_target_type_section: {
			size_t n;
			for (n = 0; n < map->nsections; ++n) {
				if (map->sections[n].section->id == target.value.sectionid) {
					*resolved = map->sections[n].base;
					return true;
				}
			}
			return false;
		}
		case jive_relocation_target_type_label_external: {
			/* FIXME: maybe label_external should have
			just an offset, nothing else? */
			*resolved = (void *) (intptr_t)
				target.value.label_external->address.offset;
			return true;
		}
		default: {
			return false;
		}
	}
}

static bool
section_process_relocations(
	void * base_writable,
	jive_offset base,
	const jive_compilate_map * map,
	const jive_section * section,
	jive_process_relocation_function relocate)
{
	const jive_relocation_entry * entry;
	JIVE_LIST_ITERATE(section->relocations, entry, section_relocation_list) {
		void * where = entry->offset + (char *) base_writable;
		jive_offset offset = entry->offset + base;
		void * target;
		if (!resolve_relocation_target(entry->target, map, &target))
			return false;
		if (!relocate(where, section->contents.size - entry->offset,
			offset, entry->type, (intptr_t) target, entry->value)) {
			return false;
		}
	}
	
	return true;
}

jive_compilate_map *
jive_compilate_load(const jive_compilate * self,
	jive_process_relocation_function relocate)
{
	size_t total_size = 0, section_count = 0;
	const jive_section * section;
	JIVE_LIST_ITERATE(self->sections, section, compilate_section_list) {
		total_size += jive_section_size_roundup(section);
		++ section_count;
	}
	
	int fd = get_tmpfd(total_size);
	if (fd == -1)
		return NULL;
	
	jive_compilate_map * map = jive_compilate_map_alloc(section_count);
	if (!map) {
		close(fd);
		return NULL;
	}
	
	void * writable = mmap(0, total_size, PROT_WRITE, MAP_SHARED, fd, 0);
	if (!writable) {
		close(fd);
		jive_compilate_map_destroy(map);
		return 0;
	}
	
	/* populate all sections with data, but keep them writable independent
	of their designation, so relocation processing can modify their
	contents */
	size_t offset = 0, n = 0;
	JIVE_LIST_ITERATE(self->sections, section, compilate_section_list) {
		void * addr = offset + (char *) writable;
		map->sections[n].section = section;
		map->sections[n].base = addr;
		map->sections[n].size = jive_section_size_roundup(section);
		
		memcpy(addr, section->contents.data, section->contents.size);
		
		/* If this is a code section, create another mapping, this time
		executable. We cannot generally assume that we can later change
		an existing mapping to executable (hello PaX, hello SELinux),
		or that the same address range through which code was written
		is also suitable for execution (hello PowerPC). Creating a
		separate mapping allows the kernel to set things up properly. */
		/* FIXME: use section attributes instead of id to decide
		whether section should be executable. */
		if (section->id == jive_stdsectionid_code) {
			void * exec_addr = mmap(0, map->sections[n].size,
				PROT_READ|PROT_EXEC, MAP_SHARED, fd, offset);
			map->sections[n].base = exec_addr;
		}
		
		offset += map->sections[n].size;
		++n;
	}
	
	/* finalize all sections and switch them over to their correct 
	permissions */
	offset = 0;
	n = 0;
	bool success = true;
	JIVE_LIST_ITERATE(self->sections, section, compilate_section_list) {
		void * base = offset + (char *) writable;
		
		success = success && section_process_relocations(base,
			(jive_offset) (intptr_t) map->sections[n].base, map, section, relocate);
		
		switch (section->id) {
			case jive_stdsectionid_code: {
				munmap(base, map->sections[n].size);
				/* The contents of the memory region might
				have been changed, the following should force
				synchronization of the icache. */
				void * exec_addr = (void *) (intptr_t)
					map->sections[n].base;
				mprotect(exec_addr, map->sections[n].size,
					PROT_NONE);
				mprotect(exec_addr, map->sections[n].size,
					PROT_READ|PROT_EXEC);
				break;
			}
			case jive_stdsectionid_rodata: {
				mprotect(base, map->sections[n].size,
					PROT_READ);
				break;
			}
			default: {
				/* empty */
			}
		}
		
		offset += map->sections[n].size;
		++n;
	}
	
	if (!success) {
		jive_compilate_map_unmap(map);
		jive_compilate_map_destroy(map);
		return NULL;
	}
	
	return map;
}

void
jive_compilate_map_unmap(const jive_compilate_map * self)
{
	size_t n;
	for (n = 0; n < self->nsections; ++n) {
		void * ptr = (void *) (intptr_t) self->sections[n].base;
		munmap(ptr, self->sections[n].size);
	}
}

void *
jive_compilate_map_to_memory(const jive_compilate * self)
{
	jive_compilate_map * map = jive_compilate_load(self, NULL);
	if (!map)
		return 0;
	
	void * executable = 0;
	
	size_t n;
	for (n = 0; n < map->nsections; ++n) {
		if (map->sections[n].section->id == jive_stdsectionid_code)
			executable = map->sections[n].base;
	}
	jive_compilate_map_destroy(map);
	
	return executable;
}