#include "common.h"
#include "buffer.h"

buffer_t buffer_new(size_t cap, const char *init, size_t init_len) {
	buffer_t ret = invalid_buffer;
	if (cap < init_len) {
		cap = init_len;
	}
	if (cap >= MAX_BUFFER_CAPACITY) {
		return ret;
	}
	buffer_data_ptr_t buffer = (buffer_data_ptr_t)nl_malloc(sizeof(buffer_data_t) + cap + 1);
	if (buffer) {
		buffer->m_cap = cap;
		buffer->m_ref = 1;
		if (init) {
			buffer->m_len = init_len;
			buffer->m_data[init_len] = 0;
			memcpy(buffer->m_data, init, init_len);
		} else {
			buffer->m_len = 0;
			buffer->m_data[0] = 0;
		}
        ret.m_ptr = buffer;
	}
    return ret;
}

static size_t buffer_gen_capacity(buffer_t& buffer_ref, size_t add_len) {
	size_t new_cap = buffer_ref.m_ptr->m_len + add_len;
	if (new_cap >= MAX_BUFFER_CAPACITY || new_cap < add_len) {
		return 0;
	}
	if (new_cap < MAX_BUFFER_PREALLOC) {
		new_cap <<= 1;
	} else {
		new_cap += MAX_BUFFER_PREALLOC;
	}
	if (new_cap > MAX_BUFFER_CAPACITY) {
		new_cap = MAX_BUFFER_CAPACITY;
	}
	return new_cap;
}

bool buffer_append(buffer_t& buffer_ref, const char *data, size_t data_len) {
    buffer_data_ptr_t buffer = buffer_ref.m_ptr;
	if (buffer && data) {
		if (buffer->m_ref == 1) {
			if (data_len <= (size_t)(buffer->m_cap - buffer->m_len)) {
				memcpy(buffer->m_data + buffer->m_len, data, data_len);
				buffer->m_len += data_len;
				buffer->m_data[buffer->m_len] = 0;
				return true;
			}
			size_t new_cap = buffer_gen_capacity(buffer_ref, data_len);
			if (new_cap > 0) {
				int32_t offset = -1;
				if (data >= (char*)buffer && data < ((char*)buffer + sizeof(buffer_data_t) + buffer->m_cap + 1)) {
					offset = data - (char*)buffer;
				}
				buffer_data_ptr_t new_buffer = (buffer_data_ptr_t)nl_realloc(buffer, sizeof(buffer_data_t) + new_cap + 1);
				if (new_buffer) {
					if (offset >= 0) data = (char*)new_buffer + offset;
					new_buffer->m_cap = new_cap;
					new_buffer->m_len = buffer->m_len + data_len;
					memcpy(new_buffer->m_data + buffer->m_len, data, data_len);
					new_buffer->m_data[new_buffer->m_len] = 0;
					buffer_ref.m_ptr = new_buffer;
					return true;
				}
			}
			return false;
		} else {
			size_t new_cap = buffer_gen_capacity(buffer_ref, data_len);
			if (new_cap > 0) {
				buffer_data_ptr_t new_buffer = (buffer_data_ptr_t)nl_malloc(sizeof(buffer_data_t) + new_cap + 1);
				if (new_buffer) {
					new_buffer->m_cap = new_cap;
					new_buffer->m_ref = 1;
					new_buffer->m_len = buffer->m_len + data_len;
					memcpy(new_buffer->m_data, buffer->m_data, buffer->m_len);
					memcpy(new_buffer->m_data + buffer->m_len, data, data_len);
					new_buffer->m_data[new_buffer->m_len] = 0;
					buffer_release(buffer_ref);
					buffer_ref.m_ptr = new_buffer;
					return true;
				}
			}
			return false;
		}
	}
	return buffer ? true : false;
}

bool buffer_clear(buffer_t& buffer_ref) {
    buffer_data_ptr_t buffer = buffer_ref.m_ptr;
	if (!buffer) return true;
	if (buffer->m_ref == 1) {
		buffer->m_len = 0;
		buffer->m_data[0] = 0;
		return true;
	} else {
		buffer_data_ptr_t new_buffer = (buffer_data_ptr_t)nl_malloc(sizeof(buffer_data_t) + buffer->m_len + 1);
		if (new_buffer) {
			new_buffer->m_cap = buffer->m_len;
			new_buffer->m_ref = 1;
			new_buffer->m_len = 0;
			new_buffer->m_data[0] = 0;
			buffer_release(buffer_ref);
			buffer_ref.m_ptr = new_buffer;
			return true;
		}
		return false;
	}
}

buffer_t& buffer_grab(buffer_t& buffer_ref) {
	if (buffer_ref.m_ptr) {
		atomic_inc(buffer_ref.m_ptr->m_ref);
	}
	return buffer_ref;
}

void buffer_release(buffer_t& buffer_ref) {
	if (buffer_ref.m_ptr && (buffer_ref.m_ptr->m_ref == 1 || atomic_dec(buffer_ref.m_ptr->m_ref) == 1)) {
		nl_free(buffer_ref.m_ptr);
		buffer_ref.m_ptr = NULL;
	}
}

bool buffer_concat(buffer_t& dest_ref, const buffer_t& src_ref) {
	if (dest_ref.m_ptr && src_ref.m_ptr) {
		return buffer_append(dest_ref, src_ref.m_ptr->m_data, src_ref.m_ptr->m_len);
	}
	return !src_ref.m_ptr;
}

bool buffer_adjust_len(buffer_t& buffer_ref, size_t add) {
	if (buffer_ref.m_ptr) {
		size_t new_len = buffer_ref.m_ptr->m_len + add;
		if (new_len >= add && new_len <= (size_t)(buffer_ref.m_ptr->m_cap)) {
			buffer_ref.m_ptr->m_len = new_len;
			return true;
		}
	}
	return add == 0;
}

buffer_t buffer_split(buffer_t& buffer_ref, size_t len)
{
	buffer_data_ptr_t buffer = buffer_ref.m_ptr;
	if (buffer && len > 0 && buffer->m_len > 0) {
		int32_t split_len = len < (size_t)buffer->m_len ? (int32_t)len : buffer->m_len;
		buffer_t split_buffer = buffer_new(len, buffer->m_data, split_len);
		if (buffer_is_valid(split_buffer)) {
			if (buffer->m_ref == 1) {
				buffer->m_len -= split_len;
				if (buffer->m_len > 0) {
					memcpy(buffer->m_data, buffer->m_data + split_len, buffer->m_len);
				}
				buffer->m_data[buffer->m_len] = 0;
			} else {
				if (split_len < buffer->m_len) {
					buffer_t new_buffer = buffer_new(buffer->m_len, buffer->m_data + split_len, buffer->m_len - split_len);
					buffer_release(buffer_ref);
					buffer_ref = new_buffer;
				} else { //split_len == buffer->m_len
					buffer_ref = buffer_new(DEFAULT_BUFFER_CAPACITY, NULL, 0);
				}
			}
			return split_buffer;
		}
	}
	return buffer_new(DEFAULT_BUFFER_CAPACITY, NULL, 0);
}
