local function f(handle)
	local result, socket = tcp.connect("127.0.0.1", 8081)
	print("socket connect to 127.0.0.1:8081 result: ", result, socket)
	if not result then return end

	socket:setopt({ read_head_bytes = 2, write_head_bytes = 2, })

	local result, buffer = socket:read()
	print("socket blocking read result: ", socket, result, result and buffer:tostring() or buffer)

	socket:read(function(result, buffer)
					print("socket nonblocking read result: ", socket, result, result and buffer:tostring() or buffer)
					if not result then
						print("socket closed", socket)
						socket:close()
					end
				end)
	coroutine.yield("yielding up")
end

local servicen = 10
local services = {}
for i = 1, servicen do
	services[i] = context.create("test1.lua", i)
end
local co = coroutine.create(f)
print("coroutine.resume1", coroutine.resume(co, services[1]))
print("coroutine.resume2", coroutine.resume(co))
for i = 1, servicen do
	context.destroy(services[i], "I just want to kill you "..i)
end
