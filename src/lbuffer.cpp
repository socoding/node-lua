#include "lbuffer.h"

static int32_t lbuffer_new(lua_State* L)
{
	size_t cap = DEFAULT_BUFFER_CAPACITY;
	const char *init = NULL;
	size_t init_len = 0;
	int32_t type = lua_type(L, 1);
	if (type == LUA_TNUMBER) {
		cap = lua_tounsigned(L, 1);
		if (lua_gettop(L) > 1) {
			init = luaL_checklstring(L, 2, &init_len);
		}
	} else if (type == LUA_TSTRING) {
		init = lua_tolstring(L, 1, &init_len);
		cap = init_len + DEFAULT_BUFFER_CAPACITY;
	} else if (type != LUA_TNIL && type != LUA_TNONE) {
		buffer_t* buffer = (buffer_t*)luaL_testudata(L, 1, BUFFER_METATABLE);
		if (!buffer) {
			luaL_argerror(L, 1, "unexpected argument type");
		}
		create_buffer(L, *buffer);
		return 1;
	}
	create_buffer(L, cap, init, init_len);
	return 1;
}

static int32_t lbuffer_append(lua_State* L)
{
	buffer_t* buffer = (buffer_t*)luaL_checkudata(L, 1, BUFFER_METATABLE);
	size_t len = 0;
	const char* str = lua_tolstring(L, 2, &len);
	if (str) {
		lua_pushboolean(L, buffer_append(*buffer, str, len));
	} else {
		buffer_t* append = (buffer_t*)luaL_testudata(L, 2, BUFFER_METATABLE);
		if (!append) {
			luaL_argerror(L, 2, "unexpected argument type");
		}
		lua_pushboolean(L, buffer_concat(*buffer, *append));
	}
	return 1;
}

static int32_t lbuffer_split(lua_State* L)
{
	buffer_t* buffer = (buffer_t*)luaL_checkudata(L, 1, BUFFER_METATABLE);
	int32_t top = lua_gettop(L);
	uint32_t length = top >= 2 ? luaL_checkunsigned(L, 2) : buffer_data_length(*buffer);
	buffer_t* split_buffer = create_buffer(L);
	*split_buffer = buffer_split(*buffer, length);
	return 1;
}

/* translate a relative string position: negative means back from end */
static lua_Integer posrelat(lua_Integer pos, size_t len) {
	if (pos >= 0) return pos;
	else if (0u - (size_t)pos > len) return 0;
	else return (lua_Integer)len + pos + 1;
}

static int32_t lbuffer_remove(lua_State* L)
{
	buffer_t* buffer = (buffer_t*)luaL_checkudata(L, 1, BUFFER_METATABLE);
	lua_Integer length = buffer_data_length(*buffer);
	lua_Integer pos = posrelat(luaL_optinteger(L, 2, 1), length);
	
	size_t start = pos >= 1 ? pos - 1 : 0;
	size_t rmlen = luaL_optunsigned(L, 3, length);

	rmlen = buffer_remove(*buffer, start, rmlen);
	lua_pushinteger(L, rmlen);
	return 1;
}

static int32_t lbuffer_clear(lua_State* L)
{
	buffer_t* buffer = (buffer_t*)luaL_checkudata(L, 1, BUFFER_METATABLE);
	lua_pushboolean(L, buffer_clear(*buffer));
	return 1;
}

static int32_t lbuffer_length(lua_State* L)
{
	buffer_t* buffer = (buffer_t*)luaL_checkudata(L, 1, BUFFER_METATABLE);
	lua_pushinteger(L, buffer_data_length(*buffer));
	return 1;
}

static int32_t lbuffer_unfill(lua_State* L)
{
	buffer_t* buffer = (buffer_t*)luaL_checkudata(L, 1, BUFFER_METATABLE);
	lua_pushinteger(L, buffer_write_size(*buffer));
	return 1;
}

static int32_t lbuffer_tostring(lua_State* L)
{
	buffer_t* buffer = (buffer_t*)luaL_checkudata(L, 1, BUFFER_METATABLE);
	if (buffer_is_valid(*buffer)) {
		lua_pushlstring(L, buffer_data_ptr(*buffer), buffer_data_length(*buffer));
	} else {
		lua_pushstring(L, "");
	}
	return 1;
}

static int32_t lbuffer_find(lua_State* L)
{
	buffer_t* buffer = (buffer_t*)luaL_checkudata(L, 1, BUFFER_METATABLE);
	if (buffer_is_valid(*buffer)) {
		int32_t top = lua_gettop(L);
		if (top >= 2) {
			const char* ptr = buffer_data_ptr(*buffer);
			const char* find = strstr(ptr, luaL_checklstring(L, 2, NULL));
			if (find) {
				lua_pushinteger(L, (int32_t)(find - ptr) + 1);
				return 1;
			}
			return 0;
		} else {
			lua_pushinteger(L, 1);
			return 1;
		}
	}
	return 0;
}

static int32_t lbuffer_valid(lua_State* L)
{
	buffer_t* buffer = (buffer_t*)luaL_checkudata(L, 1, BUFFER_METATABLE);
	lua_pushboolean(L, buffer_is_valid(*buffer));
	return 1;
}

static int32_t lbuffer_release(lua_State* L)
{
	buffer_t* buffer = (buffer_t*)luaL_checkudata(L, 1, BUFFER_METATABLE);
	if (buffer_is_valid(*buffer)) {
		buffer_release(*buffer);
		buffer_make_invalid(*buffer);
	}
	return 0;
}


/*
** Some sizes are better limited to fit in 'int', but must also fit in
** 'size_t'. (We assume that 'lua_Integer' cannot be smaller than 'int'.)
*/
#define MAX_SIZET	((size_t)(~(size_t)0))

#define MAXSIZE  \
	(sizeof(size_t) < sizeof(int) ? MAX_SIZET : (size_t)(INT_MAX))

/*
** {======================================================
** PACK/UNPACK
** =======================================================
*/


/* value used for padding */
#if !defined(LUA_PACKPADBYTE)
#define LUA_PACKPADBYTE		0x00
#endif

/* maximum size for the binary representation of an integer */
#define MAXINTSIZE	16

/* number of bits in a character */
#define NB	CHAR_BIT

/* mask for one character (NB 1's) */
#define MC	((1 << NB) - 1)

/* size of a lua_Integer */
#define SZINT	((int)sizeof(lua_Integer))


/* dummy union to get native endianness */
static const union {
	int dummy;
	char little;  /* true iff machine is little endian */
} nativeendian = { 1 };


/* dummy structure to get native alignment requirements */
struct cD {
	char c;
	union { double d; void *p; lua_Integer i; lua_Number n; } u;
};

#define MAXALIGN	(offsetof(struct cD, u))


/*
** Union for serializing floats
*/
typedef union Ftypes {
	float f;
	double d;
	lua_Number n;
	char buff[5 * sizeof(lua_Number)];  /* enough for any float type */
} Ftypes;


/*
** information to pack/unpack stuff
*/
typedef struct Header {
	lua_State *L;
	int islittle;
	int maxalign;
} Header;


/*
** options for pack/unpack
*/
typedef enum KOption {
	Kint,		/* signed integers */
	Kuint,	/* unsigned integers */
	Kfloat,	/* floating-point numbers */
	Kchar,	/* fixed-length strings */
	Kstring,	/* strings with prefixed length */
	Kzstr,	/* zero-terminated strings */
	Kpadding,	/* padding */
	Kpaddalign,	/* padding for alignment */
	Knop		/* no-op (configuration or spaces) */
} KOption;


/*
** Read an integer numeral from string 'fmt' or return 'df' if
** there is no numeral
*/
static int digit(int c) { return '0' <= c && c <= '9'; }

static int getnum(const char **fmt, int df) {
	if (!digit(**fmt))  /* no number? */
		return df;  /* return default value */
	else {
		int a = 0;
		do {
			a = a * 10 + (*((*fmt)++) - '0');
		} while (digit(**fmt) && a <= ((int)MAXSIZE - 9) / 10);
		return a;
	}
}


/*
** Read an integer numeral and raises an error if it is larger
** than the maximum size for integers.
*/
static int getnumlimit(Header *h, const char **fmt, int df) {
	int sz = getnum(fmt, df);
	if (sz > MAXINTSIZE || sz <= 0)
		luaL_error(h->L, "integral size (%d) out of limits [1,%d]",
		sz, MAXINTSIZE);
	return sz;
}


/*
** Initialize Header
*/
static void initheader(lua_State *L, Header *h) {
	h->L = L;
	h->islittle = nativeendian.little;
	h->maxalign = 1;
}


/*
** Read and classify next option. 'size' is filled with option's size.
*/
static KOption getoption(Header *h, const char **fmt, int *size) {
	int opt = *((*fmt)++);
	*size = 0;  /* default */
	switch (opt) {
	case 'b': *size = sizeof(char); return Kint;
	case 'B': *size = sizeof(char); return Kuint;
	case 'h': *size = sizeof(short); return Kint;
	case 'H': *size = sizeof(short); return Kuint;
	case 'l': *size = sizeof(long); return Kint;
	case 'L': *size = sizeof(long); return Kuint;
	case 'j': *size = sizeof(lua_Integer); return Kint;
	case 'J': *size = sizeof(lua_Integer); return Kuint;
	case 'T': *size = sizeof(size_t); return Kuint;
	case 'f': *size = sizeof(float); return Kfloat;
	case 'd': *size = sizeof(double); return Kfloat;
	case 'n': *size = sizeof(lua_Number); return Kfloat;
	case 'i': *size = getnumlimit(h, fmt, sizeof(int)); return Kint;
	case 'I': *size = getnumlimit(h, fmt, sizeof(int)); return Kuint;
	case 's': *size = getnumlimit(h, fmt, sizeof(size_t)); return Kstring;
	case 'c':
		*size = getnum(fmt, -1);
		if (*size == -1)
			luaL_error(h->L, "missing size for format option 'c'");
		return Kchar;
	case 'z': return Kzstr;
	case 'x': *size = 1; return Kpadding;
	case 'X': return Kpaddalign;
	case ' ': break;
	case '<': h->islittle = 1; break;
	case '>': h->islittle = 0; break;
	case '=': h->islittle = nativeendian.little; break;
	case '!': h->maxalign = getnumlimit(h, fmt, MAXALIGN); break;
	default: luaL_error(h->L, "invalid format option '%c'", opt);
	}
	return Knop;
}


/*
** Read, classify, and fill other details about the next option.
** 'psize' is filled with option's size, 'notoalign' with its
** alignment requirements.
** Local variable 'size' gets the size to be aligned. (Kpadal option
** always gets its full alignment, other options are limited by
** the maximum alignment ('maxalign'). Kchar option needs no alignment
** despite its size.
*/
static KOption getdetails(Header *h, size_t totalsize,
	const char **fmt, int *psize, int *ntoalign) {
	KOption opt = getoption(h, fmt, psize);
	int align = *psize;  /* usually, alignment follows size */
	if (opt == Kpaddalign) {  /* 'X' gets alignment from following option */
		if (**fmt == '\0' || getoption(h, fmt, &align) == Kchar || align == 0)
			luaL_argerror(h->L, 1, "invalid next option for option 'X'");
	}
	if (align <= 1 || opt == Kchar)  /* need no alignment? */
		*ntoalign = 0;
	else {
		if (align > h->maxalign)  /* enforce maximum alignment */
			align = h->maxalign;
		if ((align & (align - 1)) != 0)  /* is 'align' not a power of 2? */
			luaL_argerror(h->L, 1, "format asks for alignment not power of 2");
		*ntoalign = (align - (int)(totalsize & (align - 1))) & (align - 1);
	}
	return opt;
}


/*
** Pack integer 'n' with 'size' bytes and 'islittle' endianness.
** The final 'if' handles the case when 'size' is larger than
** the size of a Lua integer, correcting the extra sign-extension
** bytes if necessary (by default they would be zeros).
*/
static void packint(lua_State *L, buffer_t *b, lua_Unsigned n,
	int islittle, int size, int neg) {
	char *buff = buffer_reserve(*b, size);
	if (!buff)
		luaL_error(L, "not enough memory for lbuffer allocation");

	int i;
	buff[islittle ? 0 : size - 1] = (char)(n & MC);  /* first byte */
	for (i = 1; i < size; i++) {
		n >>= NB;
		buff[islittle ? i : size - 1 - i] = (char)(n & MC);
	}
	if (neg && size > SZINT) {  /* negative number need sign extension? */
		for (i = SZINT; i < size; i++)  /* correct extra bytes */
			buff[islittle ? i : size - 1 - i] = (char)MC;
	}
	buffer_adjust_len(*b, size);  /* add result to buffer */
}


/*
** Copy 'size' bytes from 'src' to 'dest', correcting endianness if
** given 'islittle' is different from native endianness.
*/
static void copywithendian(volatile char *dest, volatile const char *src,
	int size, int islittle) {
	if (islittle == nativeendian.little) {
		while (size-- != 0)
			*(dest++) = *(src++);
	}
	else {
		dest += size - 1;
		while (size-- != 0)
			*(dest--) = *(src++);
	}
}


static int lbuffer_pack(lua_State *L) {
	Header h;
	buffer_t* b = (buffer_t*)luaL_checkudata(L, 1, BUFFER_METATABLE);
	if (!buffer_is_valid(*b))
		luaL_error(L, "invalid lbuffer to pack");
	const char *fmt = luaL_checkstring(L, 2);  /* format string */
	int arg = 2;  /* current argument to pack */
	size_t totalsize = 0;  /* accumulate total size of result */
	initheader(L, &h);
	lua_pushnil(L);  /* mark to separate arguments from string buffer */
	while (*fmt != '\0') {
		int size, ntoalign;
		KOption opt = getdetails(&h, totalsize, &fmt, &size, &ntoalign);
		totalsize += ntoalign + size;
		while (ntoalign-- > 0)
			buffer_append_char(*b, LUA_PACKPADBYTE);  /* fill alignment */
		arg++;
		switch (opt) {
		case Kint: {  /* signed integers */
			lua_Integer n = luaL_checkinteger(L, arg);
			if (size < SZINT) {  /* need overflow check? */
				lua_Integer lim = (lua_Integer)1 << ((size * NB) - 1);
				luaL_argcheck(L, -lim <= n && n < lim, arg, "integer overflow");
			}
			packint(L, b, (lua_Unsigned)n, h.islittle, size, (n < 0));
			break;
		}
		case Kuint: {  /* unsigned integers */
			lua_Integer n = luaL_checkinteger(L, arg);
			if (size < SZINT)  /* need overflow check? */
				luaL_argcheck(L, (lua_Unsigned)n < ((lua_Unsigned)1 << (size * NB)),
				arg, "unsigned overflow");
			packint(L, b, (lua_Unsigned)n, h.islittle, size, 0);
			break;
		}
		case Kfloat: {  /* floating-point options */
			volatile Ftypes u;
			char *buff = buffer_reserve(*b, size);
			lua_Number n = luaL_checknumber(L, arg);  /* get argument */
			if (size == sizeof(u.f)) u.f = (float)n;  /* copy it into 'u' */
			else if (size == sizeof(u.d)) u.d = (double)n;
			else u.n = n;
			/* move 'u' to final result, correcting endianness if needed */
			copywithendian(buff, u.buff, size, h.islittle);
			buffer_adjust_len(*b, size);  /* add result to buffer */
			break;
		}
		case Kchar: {  /* fixed-size string */
			size_t len;
			const char *s = luaL_checklstring(L, arg, &len);
			if ((size_t)size <= len)  /* string larger than (or equal to) needed? */
				buffer_append(*b, s, size);  /* truncate string to asked size */
			else {  /* string smaller than needed */
				buffer_append(*b, s, len);  /* add it all */
				while (len++ < (size_t)size)  /* pad extra space */
					buffer_append_char(*b, LUA_PACKPADBYTE);
			}
			break;
		}
		case Kstring: {  /* strings with length count */
			size_t len;
			const char *s = luaL_checklstring(L, arg, &len);
			luaL_argcheck(L, size >= (int)sizeof(size_t) ||
				len < ((size_t)1 << (size * NB)),
				arg, "string length does not fit in given size");
			packint(L, b, (lua_Unsigned)len, h.islittle, size, 0);  /* pack length */
			buffer_append(*b, s, len);
			totalsize += len;
			break;
		}
		case Kzstr: {  /* zero-terminated string */
			size_t len;
			const char *s = luaL_checklstring(L, arg, &len);
			luaL_argcheck(L, strlen(s) == len, arg, "string contains zeros");
			buffer_append(*b, s, len);
			buffer_append_char(*b, '\0');  /* add zero at the end */
			totalsize += len + 1;
			break;
		}
		case Kpadding: buffer_append_char(*b, LUA_PACKPADBYTE);  /* FALLTHROUGH */
		case Kpaddalign: case Knop:
			arg--;  /* undo increment */
			break;
		}
	}
	return 0;
}


static int lbuffer_packsize(lua_State *L) {
	Header h;
	const char *fmt = luaL_checkstring(L, 1);  /* format string */
	size_t totalsize = 0;  /* accumulate total size of result */
	initheader(L, &h);
	while (*fmt != '\0') {
		int size, ntoalign;
		KOption opt = getdetails(&h, totalsize, &fmt, &size, &ntoalign);
		size += ntoalign;  /* total space used by option */
		luaL_argcheck(L, totalsize <= MAXSIZE - size, 1,
			"format result too large");
		totalsize += size;
		switch (opt) {
		case Kstring:  /* strings with length count */
		case Kzstr:    /* zero-terminated string */
			luaL_argerror(L, 1, "variable-length format");
			/* call never return, but to avoid warnings: *//* FALLTHROUGH */
		default:  break;
		}
	}
	lua_pushinteger(L, (lua_Integer)totalsize);
	return 1;
}


/*
** Unpack an integer with 'size' bytes and 'islittle' endianness.
** If size is smaller than the size of a Lua integer and integer
** is signed, must do sign extension (propagating the sign to the
** higher bits); if size is larger than the size of a Lua integer,
** it must check the unread bytes to see whether they do not cause an
** overflow.
*/
static lua_Integer unpackint(lua_State *L, const char *str,
	int islittle, int size, int issigned) {
	lua_Unsigned res = 0;
	int i;
	int limit = (size <= SZINT) ? size : SZINT;
	for (i = limit - 1; i >= 0; i--) {
		res <<= NB;
		res |= (lua_Unsigned)(unsigned char)str[islittle ? i : size - 1 - i];
	}
	if (size < SZINT) {  /* real size smaller than lua_Integer? */
		if (issigned) {  /* needs sign extension? */
			lua_Unsigned mask = (lua_Unsigned)1 << (size*NB - 1);
			res = ((res ^ mask) - mask);  /* do sign extension */
		}
	}
	else if (size > SZINT) {  /* must check unread bytes */
		int mask = (!issigned || (lua_Integer)res >= 0) ? 0 : MC;
		for (i = limit; i < size; i++) {
			if ((unsigned char)str[islittle ? i : size - 1 - i] != mask)
				luaL_error(L, "%d-byte integer does not fit into Lua Integer", size);
		}
	}
	return (lua_Integer)res;
}


static int lbuffer_unpack(lua_State *L) {
	Header h;
	buffer_t* b = (buffer_t*)luaL_checkudata(L, 1, BUFFER_METATABLE);
	if (!buffer_is_valid(*b))
		luaL_error(L, "invalid lbuffer to unpack");
	const char *fmt = luaL_checkstring(L, 2);  /* format string */
	size_t ld = buffer_data_length(*b);
	const char *data = buffer_data_ptr(*b);
	size_t pos = (size_t)posrelat(luaL_optinteger(L, 3, 1), ld) - 1;
	int n = 0;  /* number of results */
	luaL_argcheck(L, pos <= ld, 3, "initial position out of lbuffer");
	initheader(L, &h);
	while (*fmt != '\0') {
		int size, ntoalign;
		KOption opt = getdetails(&h, pos, &fmt, &size, &ntoalign);
		if ((size_t)ntoalign + size > ~pos || pos + ntoalign + size > ld)
			luaL_argerror(L, 2, "data string too short");
		pos += ntoalign;  /* skip alignment */
		/* stack space for item + next position */
		luaL_checkstack(L, 2, "too many results");
		n++;
		switch (opt) {
		case Kint:
		case Kuint: {
			lua_Integer res = unpackint(L, data + pos, h.islittle, size,
				(opt == Kint));
			lua_pushinteger(L, res);
			break;
		}
		case Kfloat: {
			volatile Ftypes u;
			lua_Number num;
			copywithendian(u.buff, data + pos, size, h.islittle);
			if (size == sizeof(u.f)) num = (lua_Number)u.f;
			else if (size == sizeof(u.d)) num = (lua_Number)u.d;
			else num = u.n;
			lua_pushnumber(L, num);
			break;
		}
		case Kchar: {
			lua_pushlstring(L, data + pos, size);
			break;
		}
		case Kstring: {
			size_t len = (size_t)unpackint(L, data + pos, h.islittle, size, 0);
			luaL_argcheck(L, pos + len + size <= ld, 2, "data string too short");
			lua_pushlstring(L, data + pos + size, len);
			pos += len;  /* skip string */
			break;
		}
		case Kzstr: {
			size_t len = (int)strlen(data + pos);
			lua_pushlstring(L, data + pos, len);
			pos += len + 1;  /* skip string plus final '\0' */
			break;
		}
		case Kpaddalign: case Kpadding: case Knop:
			n--;  /* undo increment */
			break;
		}
		pos += size;
	}
	lua_pushinteger(L, pos + 1);  /* next position */
	return n + 1;
}

/* }====================================================== */

/////////////////////////////////////////////////////////////////////////////////////////////////////////

static luaL_Reg lbuffer[] = {
	{ "new", lbuffer_new },
	{ "append", lbuffer_append },
	{ "pack", lbuffer_pack },
	{ "unpack", lbuffer_unpack },
	{ "packsize", lbuffer_packsize },
	{ "split", lbuffer_split },
	{ "remove", lbuffer_remove },
	{ "clear", lbuffer_clear },
	{ "length", lbuffer_length },
	{ "unfill", lbuffer_unfill },
	{ "find", lbuffer_find },
	{ "tostring", lbuffer_tostring },
	{ "valid", lbuffer_valid },
	{ "release", lbuffer_release },
	{ "__gc", lbuffer_release },
	{ "__len", lbuffer_length },
	{ "__tostring", lbuffer_tostring },
	{ NULL, NULL },
};

int luaopen_buffer(lua_State *L)
{
	luaL_newlib(L, lbuffer);
	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

buffer_t* create_buffer(lua_State* L)
{
	buffer_t* ret = (buffer_t*)lua_newuserdata(L, sizeof(buffer_t));
	buffer_make_invalid(*ret);
	if (luaL_newmetatable(L, BUFFER_METATABLE)) { /* create new metatable */
		lua_pushvalue(L, -1);
		lua_setfield(L, -2, "__index");
		luaL_setfuncs(L, lbuffer, 0);
	}
	lua_setmetatable(L, -2);
	return ret;
}

buffer_t* create_buffer(lua_State* L, size_t cap, const char *init, size_t init_len)
{
	buffer_t* ret = create_buffer(L);
	*ret = buffer_new(cap, init, init_len);
	return ret;
}

buffer_t* create_buffer(lua_State* L, buffer_t& buffer)
{
	buffer_t* ret = create_buffer(L);
	buffer_grab(buffer);
	*ret = buffer;
	return ret;
}
