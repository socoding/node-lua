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

15. result, error = context.send(handle, data1, data2, ...) --QUERY

16. result, query_data1, query_data2, ... = context.query(handle, data1, data2, ... [, query_callback(result, query_data1, query_data2, ...)]]) --QUERY(real query)
	--query_callback is a once callback, blocking if callback is nil.

17. result, query_data1, query_data2, ... = context.timed_query(handle, timeout, data1, data2, ... [, query_callback(result, query_data1, query_data2, ...)]]) --QUERY(real query)
	--query_callback is a once callback, blocking if callback is nil.
	
18. result, error = context.reply(handle, session, data1, data2, ...) --REPLY

19. result, recv_handle, session, data1, data2, ... = context.recv(handle[, timeout])
	--recv_callback is a continues callback, blocking if callback is nil.

20. result, recv_handle, session, data1, data2, ... = context.recv(handle[, recv_callback(result, recv_handle, session, data1, data2, ... )])
	--recv_callback is a continues callback, blocking if callback is nil.

21. result, error = context.wait(handle[, timeout[, callback(result, error, handle[, timeout])]])
	--connect_callback is a once callback, blocking if callback is nil.

22. timer.sleep(seconds)

23.	timer.timeout(seconds, ..., void (*callback)(...))

24.	timer.loop(interval, repeat_time, ..., void (*callback)(...))

----------------------------------------------------------------------------------------------------------------------------

14. 考虑lua_service的gc[考虑一直没有唤醒的服务的lua垃圾回收]

17. 研究lua hook，如何中断死循环和调试(输入)

22.to be implemented  / to be fixed

23.udp http https

24.lua socket tcp socket 迁移

io 阻塞非阻塞？ setvbuf
{"__tostring", f_tostring},

热更新

ｗａｒｎｉｎｇ　编译lua的共享dll库,需要在编译时开启 LUA_BUILD_AS_DLL 这个宏(以便在vc下边下编译时导出符号) (外部模块库需要同时开启 LUA_CORE 这个宏,以便使用 LUAMOD_API 这个宏来导出luaopen接口)
ｗａｒｎｉｎｇ　使用lua的dll扩展库,需要把lua整个打成dll动态库,主进程和其它扩展库都需要使用动态链接lua库的方式来编译或生成,不然lua的版本校验将不会通过(动态库实际使用lua.dll,编译和链接时使用lua.lib)
                如果是静态库，那么每个模块加载该静态库后，每个模块都分别有一个拷贝。如果是动态库，每个模块加载该动态库后，都只有一个拷贝。
http://blog.csdn.net/rheostat/article/details/18796911
				
Don't link your C module with liblua.a when you create a.so from it. 
For examples, see my page of Lua libraries: http://www.tecgraf.puc-rio.br/~lhf/ftp/lua/ .
You can link liblua.a statically into your main program but you have to export its symbols by adding -Wl,-E at link time.
That's how the Lua interpreter is built in Linux.

A node completation of lua which support sync and async remote procedure call, and task-multiplexing support(in muti-thread mode with no useless wakeup)