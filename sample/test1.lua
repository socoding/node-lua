local index = ...
local port = 8080 + index
local result, listen_socket = tcp.listen("127.0.0.1", port)
print("listening: ", result, port, listen_socket, context.thread(), context.self())

listen_socket:accept(function(result, socket)
						print("socket accept ok: ", result, socket, port)
						socket:setopt({ read_head_bytes = 2, write_head_bytes = 2, })
						print("socket send result1: ", socket, socket:write("hello world1!", true), port)
						print("socket send result2: ", socket, socket:write("hello world2!", true), port)
						print("socket send result3: ", socket, socket:write("hello world3!", true), port)
						print("socket send result4: ", socket, socket:write("hello world4!", true), port)
						print("socket send result5: ", socket, socket:write("hello world5!", true), port)
						print("socket send result6: ", socket, socket:write("hello world6!", true), port)
						--socket:close() --to be checked
					 end)



