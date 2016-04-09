#include "message.h"

message_array_t* message_array_create(uint32_t count) {
	message_array_t* array = (message_array_t*)nl_calloc(1, sizeof(message_array_t) + (count - 1)*sizeof(message_t));
	array->m_count = count;
	return array;
}

void message_array_release(message_array_t* array) {
	if (array != NULL) {
		for (int32_t i = 0; i < array->m_count; ++i) {
			message_clean(array->m_array[i]);
		}
		nl_free(array);
	}
}

