#ifndef JIVE_BUFFER_H
#define JIVE_BUFFER_H

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <jive/context.h>

/** \file jive/buffer.h */

/**
	\defgroup jive_buffer Expandable buffers 
	
	Management of expandable buffers
	
	@{
*/

/** \brief Expandable buffer */
typedef struct jive_buffer jive_buffer;

/** \brief Expandable buffer */
struct jive_buffer {
	/** \brief Data stored in buffer */
	void * data;
	/** \brief Number of bytes stored in buffer */
	size_t size;
	/** \brief Available storage space before buffer must be expanded */
	size_t available;
	/** \brief Context used for memory allocations */
	jive_context * context;
};

static inline void
jive_buffer_init(jive_buffer * self, jive_context * context);

static inline void
jive_buffer_fini(jive_buffer * self);

static inline void *
jive_buffer_reserve(jive_buffer * self, size_t size);

static inline bool
jive_buffer_put(jive_buffer * self, const void * data, size_t size);

static inline bool
jive_buffer_putbyte(jive_buffer * self, unsigned char byte);

void *
jive_buffer_executable(const jive_buffer * self);

/* implementation */

void *
jive_buffer_reserve_slow(jive_buffer * self, size_t size);

/**
	\brief Initialize expandable buffer
	
	\param buffer Buffer to initialize
*/
static inline void
jive_buffer_init(jive_buffer * self, jive_context * context)
{
	self->data = 0;
	self->size = self->available = 0;
	self->context = context;
}

/**
	\brief Destroy expandable buffer
	\param buffer Buffer to destroy
	
	Frees the memory stored in the buffer
*/
static inline void
jive_buffer_fini(jive_buffer * self)
{
	jive_context_free(self->context, self->data);
}

/**
	\brief Reserve space in buffer
	
	\param buffer Buffer to append to
	\param size Number of bytes to append
	\returns Pointer to first byte to be written, or NULL if resizing failed
	
	Resizes @c buffer such that at least @c size additional bytes
	fit in. The newly-reserved space is uninitialized and should
	immediately be filled with data.
	
	The returned pointer is only valid until the next call to
	\ref jive_buffer_append
*/
static inline void *
jive_buffer_reserve(jive_buffer * self, size_t size)
{
	if (self->size + size > self->available)
		return jive_buffer_reserve_slow(self, size);
	
	void * tmp = self->size + (char *)(self->data);
	self->size += size;
	return tmp;
}

/**
	\brief Resize buffer
	
	\param self Buffer to be resized
	\param size New size of buffer in bytes
	
	Resizes the given buffer to the desired size. The data will be
	truncated or expanded with undefined values.
*/
static inline void
jive_buffer_resize(jive_buffer * self, size_t new_size)
{
	if (new_size > self->available)
		jive_buffer_reserve_slow(self, new_size - self->available);
	self->size = new_size;
}

/**
	\brief Append to buffer
	
	\param buffer Buffer to append to
	\param data Data to append to buffer
	\param size Number of bytes to append
	\returns Whether adding data succeeded
	
	Appends @c data to @c buffer. Returns @c true if data was added
	to buffer, or @c false if no data could be added due to an out-of-memory
	condition.
*/
static inline bool
jive_buffer_put(jive_buffer * self, const void * data, size_t size)
{
	void * ptr = jive_buffer_reserve(self, size);
	if (!ptr) return false;
	memcpy(ptr, data, size);
	return true;
}

static inline bool
jive_buffer_putstr(jive_buffer * self, const char * str)
{
	return jive_buffer_put(self, str, strlen(str));
}

/**
	\brief Append to buffer
	
	\param buffer Buffer to append to
	\param byte Data to append to buffer
	\returns Whether adding data succeeded
	
	Appends @c byte to @c buffer. Returns @c true if data was added
	to buffer, or @c false if no data could be added due to an out-of-memory
	condition.
*/
static inline bool
jive_buffer_putbyte(jive_buffer * self, unsigned char byte)
{
	return jive_buffer_put(self, &byte, 1);
}

/** @} */

#endif
