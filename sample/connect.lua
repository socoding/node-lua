--[[print("this is connect handle")
local result, socket = tcp.connect("127.0.0.1", 8081)
if not result then
	context.destroy(2)
	return
end

socket:setopt({ read_head_bytes = 2, write_head_bytes = 2, })
for i = 1, 10 do
	socket:write(tostring(i))
end]]


local port = ...
local result, socket = tcp.connect("127.0.0.1", port)
if not result then
	context.destroy(port - 8080 + 1)
	return
end
socket:setopt({ read_head_bytes = 2, write_head_bytes = 2, })
math.randomseed(os.time())

local index = port - 8080
local loop_count = index * 10
for i = 1, loop_count do
	socket:write(tostring(i))
end
