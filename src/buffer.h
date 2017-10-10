#ifndef BUFFER_H_
#define BUFFER_H_
#include "common.h"

#define DEFAULT_BUFFER_CAPACITY (32)
#define MAX_BUFFER_CAPACITY		(0x7fffffff)
#define MAX_BUFFER_PREALLOC		(1024*1024) //adjust capacity and realloc size and strategy.

typedef struct {
    atomic_t m_ref;
    int32_t m_len;
    int32_t m_cap;
    char m_data[0];
} buffer_data_t, *buffer_data_ptr_t;

typedef struct {
    buffer_data_ptr_t m_ptr;
} buffer_t;

extern buffer_t buffer_new(size_t cap, const char *init, size_t init_len);
extern bool buffer_append(buffer_t& buffer_ref, const char *data, size_t data_len);
extern char* buffer_reserve(buffer_t& buffer_ref, size_t data_len);
extern bool buffer_append_char(buffer_t& buffer_ref, char ch);
extern bool buffer_clear(buffer_t& buffer_ref);
extern buffer_t& buffer_grab(buffer_t& buffer_ref);
extern void buffer_release(buffer_t& buffer_ref);
extern bool buffer_concat(buffer_t& dest_ref, const buffer_t& src_ref);
extern bool buffer_adjust_len(buffer_t& buffer_ref, size_t add);
extern buffer_t buffer_split(buffer_t& buffer_ref, size_t len);
extern size_t buffer_remove(buffer_t& buffer_ref, size_t start, size_t len);

#define invalid_buffer						{ NULL }
#define buffer_is_valid(buffer_ref)			((buffer_ref).m_ptr != NULL)
#define buffer_data_ptr(buffer_ref)			((buffer_ref).m_ptr ? (buffer_ref).m_ptr->m_data : NULL)
#define buffer_data_length(buffer_ref)		((buffer_ref).m_ptr ? (buffer_ref).m_ptr->m_len : 0)
#define buffer_write_ptr(buffer_ref)		((buffer_ref).m_ptr ? (buffer_ref).m_ptr->m_data + (buffer_ref).m_ptr->m_len : NULL)
#define buffer_write_size(buffer_ref)		((buffer_ref).m_ptr ? (buffer_ref).m_ptr->m_cap - (buffer_ref).m_ptr->m_len : 0)
#define buffer_make_invalid(buffer_ref)		((buffer_ref).m_ptr = NULL)
#endif
