#ifndef CONTEXT_H_
#define CONTEXT_H_
#include "message.h"
#include "message_queue.h"

class worker_t;

class context_t {
public:
	context_t();
	FORCE_INLINE uint32_t get_parent() const { return m_parent; }
	FORCE_INLINE uint32_t get_handle() const { return m_handle; }
	FORCE_INLINE void grab() { atomic_inc(m_ref); }
	FORCE_INLINE void release() { if (atomic_dec(m_ref) == 1) delete this; }
	FORCE_INLINE void set_inited(bool init) { m_inited = init; }
	FORCE_INLINE bool is_inited() const { return m_inited; }
	FORCE_INLINE bool is_valid() const { return m_inited && m_handle != 0; }
	FORCE_INLINE void link(context_t* next) { m_next_processing = next; }
	FORCE_INLINE void unlink() { m_next_processing = NULL; }
	FORCE_INLINE context_t* next() const { return m_next_processing; }
	FORCE_INLINE uint32_t get_message_length() const { return m_message_queue.get_length(); };
	
	void attach_worker(worker_t* worker);
	void detach_worker();

	void on_registered(uint32_t parent, uint32_t handle); /* thread safe */
	void on_retired(); /* thread safe */
	bool push_message(message_t& message, bool& processing);
	bool pop_message(message_t& message);
	virtual void on_received(message_t& message);
	virtual void on_dropped(message_t& message);
	virtual bool init(int32_t argc, char* argv[], char* env[]) = 0;
	virtual bool deinit(const char *arg) = 0;
	virtual void on_worker_attached() {};
	virtual void on_worker_detached() {};
protected:
	virtual ~context_t();

protected:
	context_t(const context_t& ctx);
	context_t& operator=(const context_t& ctx);

	uint32_t m_parent;  /* parent handle */
	uint32_t m_handle;  /* allocated handle */
	worker_t* m_worker; /* attached worker */
private:
	bool m_inited;			  /* whether context has been inited */
	atomic_t m_ref;			  /* context reference */
	bool m_processing;				 /* whether context is being processed(in context queue) */
	context_t *m_next_processing;	 /* next context to be processed(in context queue) */
	atomic_t m_message_queue_lock;   /* message queue lock */
	message_queue_t m_message_queue; /* message queue */
};

#endif
