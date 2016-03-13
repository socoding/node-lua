#include <stdlib.h>
#include "errors.h"

#define NL_ERR_NAME_GEN(val, name, s) case NL_##name : return #name;
const char* nl_err_name(nl_err_code err_code) {
	switch (err_code) {
		NL_ERRNO_MAP(NL_ERR_NAME_GEN)
	default:
		assert(0);
		return NULL;
	}
}
#undef NL_ERR_NAME_GEN


#define NL_STRERROR_GEN(val, name, s) case NL_##name : return s;
const char* nl_strerror(nl_err_code err_code) {
	switch (err_code) {
		NL_ERRNO_MAP(NL_STRERROR_GEN)
	default:
		return "Unknown system error";
	}
}
#undef NL_STRERROR_GEN

///////////////////////////////////////////////////////////////////////////////////////

const char* common_err_name(int32_t err_code) {
	if (err_code < UV_MAX_ERRORS) {
		uv_err_t err = { (uv_err_code)err_code, 0 };
		return uv_err_name(err);
	}
	if (err_code < NL_MAX_ERRORS) {
		return nl_err_name((nl_err_code)err_code);
	}
	assert(0);
	return NULL;
}

const char* common_strerror(int32_t err_code) {
	if (err_code < UV_MAX_ERRORS) {
		uv_err_t err = { (uv_err_code)err_code, 0 };
		return uv_strerror(err);
	}
	if (err_code < NL_MAX_ERRORS) {
		return nl_strerror((nl_err_code)err_code);
	}
	return "Unknown system error";
}
