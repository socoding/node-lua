#ifndef MESSAGE_H_
#define MESSAGE_H_
#include "buffer.h"
#include "lbinary.h"
#include "ltpack.h"

#define  MESSAGE_TYPE_BIT	24
#define	 MAKE_MESSAGE_TYPE(mtype, dtype) (((uint32_t)(dtype) << MESSAGE_TYPE_BIT) | ((mtype) & (((uint32_t)1 << MESSAGE_TYPE_BIT) - 1)))
#define	 DATA_TYPE(type)	(type >> (MESSAGE_TYPE_BIT))
#define	 MESSAGE_TYPE(type)	(type & ((((uint32_t)1 << MESSAGE_TYPE_BIT) - 1)))

typedef struct { uint8_t m_nil; } nil_t;
typedef struct message_array_t message_array_t;

enum data_type {
	NIL				= 0,
	TBOOLEAN		= 1,
	USERDATA		= 2, //light userdata
	NUMBER			= 3,
	INTEGER			= 4, //support after lua53
	BUFFER			= 5, //need to be freed
	STRING			= 6, //need to be freed
	BINARY			= 7, //need to be freed
	TPACK			= 8, //need to be freed
	ARRAY			= 9, //need to be freed
	TERROR			= 10 //
};

typedef union data_t {
	bool			  m_bool;
	void			 *m_userdata;
	double			  m_number;
	int64_t			  m_integer;
	buffer_t		  m_buffer;
	char			 *m_string;
	binary_t		  m_binary;
	tpack_t			  m_tpack;
	message_array_t  *m_array;
	int32_t			  m_error;
} data_t;

enum message_type {
	INVALID_MESSAGE = 0,
	////////////////////////////////////////
	SYSTEM_CTX_DESTROY,
	CONTEXT_QUERY,
	CONTEXT_REPLY,
	SYSTEM_MESSAGE_END,
	////////////////////////////////////////
	LUA_CTX_INIT,
	LUA_CTX_WAIT,
	LUA_CTX_WAKEUP,
	RESPONSE_TCP_LISTEN,
	RESPONSE_TCP_ACCEPT,
	RESPONSE_TCP_CONNECT,
	RESPONSE_TCP_READ,
	RESPONSE_TCP_WRITE,
	RESPONSE_TCP_CLOSING,
	RESPONSE_UDP_OPEN,
	RESPONSE_UDP_READ,
	RESPONSE_UDP_WRITE,
	RESPONSE_UDP_CLOSING,
	RESPONSE_HANDLE_CLOSE,
	RESPONSE_TIMEOUT,
	////////////////////////////////////////
	LOG_MESSAGE
};

#define message_raw_type(msg)		(MESSAGE_TYPE((msg).m_type))

#define message_bool(msg)			((msg).m_data.m_bool)
#define message_userdata(msg)		((msg).m_data.m_userdata)
#define message_number(msg)			((msg).m_data.m_number)
#define message_integer(msg)		((msg).m_data.m_integer)
#define message_buffer(msg)			((msg).m_data.m_buffer)
#define message_string(msg)			((msg).m_data.m_string)
#define message_binary(msg)			((msg).m_data.m_binary)
#define message_tpack(msg)			((msg).m_data.m_tpack)
#define message_array(msg)			((msg).m_data.m_array)
#define message_error(msg)			((msg).m_data.m_error)

#define message_data_type(msg)		(DATA_TYPE((msg).m_type))
#define message_is_nil(msg)			(NIL == message_data_type(msg))
#define message_is_bool(msg)		(TBOOLEAN == message_data_type(msg))
#define message_is_userdata(msg)	(USERDATA == message_data_type(msg))
#define message_is_number(msg)		(NUMBER == message_data_type(msg))
#define message_is_integer(msg)		(INTEGER == message_data_type(msg))
#define message_is_buffer(msg)		(BUFFER == message_data_type(msg))
#define message_is_string(msg)		(STRING == message_data_type(msg))
#define message_is_binary(msg)		(BINARY == message_data_type(msg))
#define message_is_tpack(msg)		(TPACK == message_data_type(msg))
#define message_is_array(msg)		(ARRAY == message_data_type(msg))
#define message_is_error(msg)		(TERROR == message_data_type(msg))
#define message_is_pure_error(msg)	(TERROR == message_data_type(msg) && message_error(msg) != 0)

#define message_buffer_grab(msg)	if (message_is_buffer(msg)) { buffer_grab(msg.m_data.m_buffer); }
#define message_buffer_release(msg)	if (message_is_buffer(msg)) { buffer_release(msg.m_data.m_buffer); }
#define message_clean(msg)			do {															\
										if (message_is_buffer(msg)) {								\
											buffer_release(msg.m_data.m_buffer);					\
										} else if (message_is_string(msg)) {                    	\
											if ((msg).m_data.m_string) {							\
												nl_free((msg).m_data.m_string);						\
												(msg).m_data.m_string = NULL;						\
											}														\
										} else if (message_is_binary(msg)) {                    	\
											if ((msg).m_data.m_binary.m_data) {						\
												sdsfree((msg).m_data.m_binary.m_data);				\
												(msg).m_data.m_binary.m_data = NULL;				\
											}														\
										} else if (message_is_tpack(msg)) {					     	\
											if ((msg).m_data.m_tpack.m_data) {						\
												sdsfree((msg).m_data.m_tpack.m_data);				\
												(msg).m_data.m_tpack.m_data = NULL;					\
											}														\
										} else if (message_is_array(msg)) {					     	\
											if ((msg).m_data.m_array) {								\
												message_array_release((msg).m_data.m_array);		\
												(msg).m_data.m_array = NULL;						\
											}														\
										}															\
									} while (0)

class message_t {
public:
	message_t()
			: m_source(0), m_session(0),
			  m_type(MAKE_MESSAGE_TYPE(INVALID_MESSAGE, NIL)) {}

	message_t(uint32_t source, int32_t session, uint32_t msg_type, nil_t nil)
			: m_source(source), m_session(session),
			  m_type(MAKE_MESSAGE_TYPE(msg_type, NIL)) {}

	message_t(uint32_t source, int32_t session, uint32_t msg_type, bool bval)
			: m_source(source), m_session(session),
			  m_type(MAKE_MESSAGE_TYPE(msg_type, TBOOLEAN)) { m_data.m_bool = bval; }

	message_t(uint32_t source, int32_t session, uint32_t msg_type, void *userdata)
			: m_source(source), m_session(session),
			  m_type(MAKE_MESSAGE_TYPE(msg_type, USERDATA)) { m_data.m_userdata = userdata; }

	message_t(uint32_t source, int32_t session, uint32_t msg_type, const buffer_t& buffer)
			: m_source(source), m_session(session),
			  m_type(MAKE_MESSAGE_TYPE(msg_type, BUFFER)) { m_data.m_buffer = buffer; }

	message_t(uint32_t source, int32_t session, uint32_t msg_type, double number)
			: m_source(source), m_session(session),
			  m_type(MAKE_MESSAGE_TYPE(msg_type, NUMBER)) { m_data.m_number = number; }
	
	message_t(uint32_t source, int32_t session, uint32_t msg_type, int64_t integer)
			: m_source(source), m_session(session),
			  m_type(MAKE_MESSAGE_TYPE(msg_type, INTEGER)) { m_data.m_integer = integer; }

	message_t(uint32_t source, int32_t session, uint32_t msg_type, char *string)
			: m_source(source), m_session(session),
			  m_type(MAKE_MESSAGE_TYPE(msg_type, STRING)) { m_data.m_string = string; }

	message_t(uint32_t source, int32_t session, uint32_t msg_type, uv_err_code err)
			: m_source(source), m_session(session),
			  m_type(MAKE_MESSAGE_TYPE(msg_type, TERROR)) { m_data.m_error = err; }

	message_t(uint32_t source, int32_t session, uint32_t msg_type, nl_err_code err)
			: m_source(source), m_session(session),
			  m_type(MAKE_MESSAGE_TYPE(msg_type, TERROR)) { m_data.m_error = err; }

	message_t(uint32_t source, int32_t session, uint32_t msg_type, tpack_t tpack)
			: m_source(source), m_session(session),
			  m_type(MAKE_MESSAGE_TYPE(msg_type, TPACK)) { m_data.m_tpack = tpack; }

	message_t(uint32_t source, int32_t session, uint32_t msg_type, binary_t binary)
			: m_source(source), m_session(session),
			  m_type(MAKE_MESSAGE_TYPE(msg_type, BINARY)) { m_data.m_binary = binary; }

	message_t(uint32_t source, int32_t session, uint32_t msg_type, message_array_t* array)
			: m_source(source), m_session(session),
			  m_type(MAKE_MESSAGE_TYPE(msg_type, ARRAY)) { m_data.m_array = array; }
public:
	uint32_t m_source;
	int32_t  m_session;
	uint32_t m_type;
	data_t	 m_data;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//message array api
struct message_array_t {
	uint32_t m_count;
	message_t m_array[1];
};

extern message_array_t* message_array_create(uint32_t count);
extern  void message_array_release(message_array_t* array);

#endif
