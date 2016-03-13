# node-lua
A node completation of lua which support sync and async remote procedure call, and task-multiplexing support(in muti-thread mode with no useless wakeup)

# usefull api
## tcp api
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

15. result, error = context.send(handle, data) --QUERY

16. result, query_data = context.query(handle, data[, timeout [, query_callback(result, query_data, handle, data[, timeout])]]) --QUERY(real query)
	--query_callback is a once callback, blocking if callback is nil.
	
17. result, error = context.reply(handle, session, data) --REPLY

18. result, data, recv_handle, session = context.recv(handle[, timeout])
	--recv_callback is a continues callback, blocking if callback is nil.
	
19. result, data, recv_handle, session = context.recv(handle[, recv_callback(result, data, recv_handle, session, handle)])
	--recv_callback is a continues callback, blocking if callback is nil.
	
20. result, error = context.wait(handle[, timeout[, callback(result, error, handle[, timeout])]])
	--connect_callback is a once callback, blocking if callback is nil.
	
21. timer.sleep(seconds)

22.	timer.timeout(seconds, ..., void (*callback)(...))

23.	timer.loop(interval, repeat_time, ..., void (*callback)(...))
