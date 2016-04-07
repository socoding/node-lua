#include "message.h"

message_array_t* message_array_create(message_t* messages, uint32_t count) {
	return NULL;
}

void message_array_release(message_array_t* array) {
	for (int32_t i = 0; i < array->m_count; ++i) {
		message_clean(array->m_array[i]);
	}
	nl_free(array);
}
