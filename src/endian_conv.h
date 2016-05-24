#ifndef ENDIAN_CONV_H_
#define ENDIAN_CONV_H_

#define LITTLE_ENDIAN_VAL		((char)'L')
#define BIG_ENDIAN_VAL			((char)'B')

extern char env_endian();
extern int16_t endian_to_int16(char *src, char src_endian);
extern int32_t endian_to_int32(char *src, char src_endian);
extern int64_t endian_to_int64(char *src, char src_endian);
extern int64_t endian_to_integer(char *src, char src_bytes, char src_endian);
extern void endian_from_int16(int16_t src, char *dest, char dest_endian);
extern void endian_from_int32(int32_t src, char *dest, char dest_endian);
extern void endian_from_int64(int64_t src, char *dest, char dest_endian);
extern void endian_from_integer(int64_t src, char *dest, char dest_bytes, char dest_endian);

#endif
