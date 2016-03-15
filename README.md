# node-lua

## description
1.	A node complementation of lua which supports sync and rpc, and task-multiplexing support in multi-threads without harmless wakeup.
2.	It can be used as a simple script engine or complex server engine which supports a massive of independent lua contexts (or named services) running on multi-threads which restricted to the cpu core count.
3.	The lua context will suspend itself when it calls a sync and async rpc using lua coroutine inside the core c codes.
4.	The rpc can be called within the lua coroutine where the user creates and it won't impact the normal coroutine procedure.
5.	A context starts with a lua file as the entry. The process will exit automaticly when all contexts ends or terminates and a context will exit automaticly when all its sync and async remote procedure call ends or terminates.
6.	A optimized task scheduling is used with a thread based context queue which reduces the thread race condition and work-stealing algorithm is used in the task scheduling.
7.  A more friendly tcp api is embed in this engine with sync and async implementation which is much more convenient to build a tcp server.

## build & install

For windows, just open node-lua.sln and build the whole solution. For linux or other unix-like system

```
git clone https://github.com/socoding/node-lua.git
cd node-lua
make && make install
make release && make install

## usefull api
### tcp api
1.	result, listen_socket = tcp.listen(addr, port[, backlog, [void (*listen_callback)(result, listen_socket, addr, port[, backlog])]])
	--listen_callback is a once callback, blocking if callback is nil.
	
2.	result, listen_socket = tcp.listens(sock_name[, void (*listen_callback)(result, listen_socket, sock_name)])
	--listen_callback is a once callback, blocking if callback is nil.

3.	result, listen_socket = tcp.listen6(addr, port[, backlog, [void (*listen_callback)(result, listen_socket, addr, port[, backlog])]])
	--listen_callback is a once callback, blocking if callback is nil.
	
4.	result, accept_socket = tcp.accept(listen_socket[, timeout])
	--accept_callback is a continues callback, blocking if callback is nil.

5.	result, accept_socket = tcp.accept(listen_socket[, void (*accept_callback)(result, accept_socket, listen_socket)])
	--accept_callback is a continues callback, blocking if callback is nil.
	
6. 	result, connect_socket = tcp.connect(host_addr, host_port[, timeout [, void (*connect_callback)(result, connect_socket, host_addr, host_port[, timeout])]])
	--connect_callback is a once callback, blocking if callback is nil.
	
7. 	result, connect_socket = tcp.connects(sock_name[, timeout [, void (*connect_callback)(result, connect_socket, host_addr, host_port[, timeout])]])
	--connect_callback is a once callback, blocking if callback is nil.
	
8. 	result, connect_socket = tcp.connect6(host_addr, host_port[, timeout [, void (*connect_callback)(result, connect_socket, host_addr, host_port[, timeout])]])
	--connect_callback is a once callback, blocking if callback is nil.
	
9.  result, buffer = tcp.read(socket[, timeout])
	--recv_callback is a continues callback, blocking if callback is nil.
	
10. result, buffer = tcp.read(socket[, void (*recv_callback)(result, buffer, socket)])
	--recv_callback is a continues callback, blocking if callback is nil.
	
11. result, error = tcp.write(socket, buffer_or_lstring[, void (*send_callback)(result, error, socket, buffer_or_lstring)/bool safety])
	--send_callback is a once callback and is safety assurance, blocking until buffer_or_lstring is sent only if safety is true.
	
12. tcp.set_rwopt(socket, option_table)
	--set tcp_socket read and write options --e.g. { "read_head_endian" =  "L", "read_head_bytes" = 2, "read_head_max" = 65535, }
	
13. tcp.get_rwopt(socket)
	--get tcp_socket read and write options
	
14. tcp.set_nodelay(socket, enable)

### context api
1.	result, error = context.send(handle, data) --QUERY

2.	result, query_data = context.query(handle, data[, timeout [, query_callback(result, query_data, handle, data[, timeout])]]) --QUERY(real query)
	--query_callback is a once callback, blocking if callback is nil.
	
3.	result, error = context.reply(handle, session, data) --REPLY

4.	result, data, recv_handle, session = context.recv(handle[, timeout])
	--recv_callback is a continues callback, blocking if callback is nil.
	
5.	result, data, recv_handle, session = context.recv(handle[, recv_callback(result, data, recv_handle, session, handle)])
	--recv_callback is a continues callback, blocking if callback is nil.
	
6.	result, error = context.wait(handle[, timeout[, callback(result, error, handle[, timeout])]])
	--connect_callback is a once callback, blocking if callback is nil.

### timer api	
1.	timer.sleep(seconds)

2.	timer.timeout(seconds, ..., void (*callback)(...))

3.	timer.loop(interval, repeat_time, ..., void (*callback)(...))

### buffer api
