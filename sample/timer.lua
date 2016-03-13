local port = ...

for i = 1, 2 do
	timer.sleep(1)
end

local function timeout(...)
	print("timeout", ...)
	timer.sleep(1)
end

timer.timeout(1, 3, 4, timeout)
timer.loop(1, 0.5, "hello", function(...) timer.sleep(1) end)


local result, listen_socket = tcp.listen("127.0.0.1", port or 80)
local result, accept_socket = listen_socket:accept(6)
if result then
	listen_socket:close()
	accept_socket:setopt({ read_head_bytes = 2, write_head_bytes = 2, })
	local result, buffer = accept_socket:read(4)
	if result then
		print("accept_socket:read success", result, buffer:tostring())
	else
		print("accept_socket:read failed", result, buffer)
	end
else
	print("listen_socket:accept failed", result, accept_socket)
end

print("context.recv 2", context.recv(0, 10))
print("context.recv 1", context.recv(0))
context.recv(handle, function(result, data, recv_handle, session, handle) print("context.recv 3", result, data, recv_handle, session, handle) end)


