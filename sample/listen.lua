--[[print("this is listen handle")
local result, listen_socket = tcp.listen("127.0.0.1", 8081)
local result, accept_socket = listen_socket:accept()
listen_socket:close()
accept_socket:setopt({ read_head_bytes = 2, write_head_bytes = 2, })

for i = 1, 10 do
	local result, buffer = accept_socket:read()
	print("read: ", result and buffer:tostring() or buffer)
end]]

print("context.recv", context.recv(0))

do return end

local port = ...
local result, listen_socket = tcp.listen("127.0.0.1", port)
local result, accept_socket = listen_socket:accept()
listen_socket:close()
accept_socket:setopt({ read_head_bytes = 2, write_head_bytes = 2, })

local function pressure_test(loop_count)
	local a = 0
	for i = 1, loop_count do
		a = a + 1
	end
	return a
end

local index = port - 8080
local loop_count = index * 10
for i = 1, loop_count do
	local result, buffer = accept_socket:read()
	--print("read: ", port, result and buffer:tostring() or buffer)
	pressure_test(10)
end






