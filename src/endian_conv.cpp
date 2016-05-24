#include "common.h"
#include "endian_conv.h"

static union { char c[4]; unsigned long mylong; } endian_test = { { LITTLE_ENDIAN_VAL, '?', '?', BIG_ENDIAN_VAL } };
#define ENV_ENDIAN_VAL			((char)endian_test.mylong)
#define ENV_IS_LITTEL_ENDIAN	(ENV_ENDIAN_VAL == LITTLE_ENDIAN_VAL)
#define ENV_IS_BIG_ENDIAN		(ENV_ENDIAN_VAL == BIG_ENDIAN_VAL)

char env_endian()
{
	return ENV_ENDIAN_VAL;
}

int16_t endian_to_int16(char *src, char src_endian)
{
	if (src_endian == ENV_ENDIAN_VAL) {
		return *(int16_t*)src;
	}
	int16_t ret;
	char *p = (char*)&ret;
	p[0] = src[1];
	p[1] = src[0];
	return ret;
}

int32_t endian_to_int32(char *src, char src_endian)
{
	if (src_endian == ENV_ENDIAN_VAL) {
		return *(int32_t*)src;
	}
	int32_t ret;
	char *p = (char*)&ret;
	p[0] = src[3];
	p[1] = src[2];
	p[2] = src[1];
	p[3] = src[0];
	return ret;
}

int64_t endian_to_int64(char *src, char src_endian)
{
	if (src_endian == ENV_ENDIAN_VAL) {
		return *(int64_t*)src;
	}
	int64_t ret;
	char *p = (char*)&ret;
	p[0] = src[7];
	p[1] = src[6];
	p[2] = src[5];
	p[3] = src[4];
	p[4] = src[3];
	p[5] = src[2];
	p[6] = src[1];
	p[7] = src[0];
	return ret;
}

int64_t endian_to_integer(char *src, char src_bytes, char src_endian)
{
	switch (src_bytes)
	{
	case 1 :
		return *src;
	case 2 :
		return endian_to_int16(src, src_endian);
	case 4 :
		return endian_to_int32(src, src_endian);
	case 8 :
		return endian_to_int64(src, src_endian);
	default:
		return 0;
	}
}

void endian_from_int16(int16_t src, char *dest, char dest_endian)
{
	if (dest_endian == ENV_ENDIAN_VAL) {
		*(int16_t*)dest = src;
		return;
	}
	char *p = (char*)&src;
	dest[0] = p[1];
	dest[1] = p[0];
}

void endian_from_int32(int32_t src, char *dest, char dest_endian)
{
	if (dest_endian == ENV_ENDIAN_VAL) {
		*(int32_t*)dest = src;
		return;
	}
	char *p = (char*)&src;
	dest[0] = p[3];
	dest[1] = p[2];
	dest[2] = p[1];
	dest[3] = p[0];
}

void endian_from_int64(int64_t src, char *dest, char dest_endian)
{
	if (dest_endian == ENV_ENDIAN_VAL) {
		*(int64_t*)dest = src;
		return;
	}
	char *p = (char*)&src;
	dest[0] = p[7];
	dest[1] = p[6];
	dest[2] = p[5];
	dest[3] = p[4];
	dest[4] = p[3];
	dest[5] = p[2];
	dest[6] = p[1];
	dest[7] = p[0];
}

void endian_from_integer(int64_t src, char *dest, char dest_bytes, char dest_endian)
{
	switch (dest_bytes)
	{
	case 1:
		*dest = src;
		return;
	case 2:
		endian_from_int16(src, dest, dest_endian);
		return;
	case 4:
		endian_from_int32(src, dest, dest_endian);
		return;
	case 8:
		endian_from_int64(src, dest, dest_endian);
		return;
	default:
		return;
	}
}
