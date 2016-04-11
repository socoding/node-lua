
local host, port, write_count = ...
write_count = tonumber(write_count)
local result, socket = tcp.connect(host, tonumber(port))
if result then
	socket:set_rwopt({ read_head_endian =  "L", read_head_bytes = 2, read_head_max = 65535, write_head_endian =  "L", write_head_bytes = 2, write_head_max = 65535, })
	for i = 1, write_count do
		socket:write("client wrote: "..(context.self).."_"..i)
	end
	for i = 1, write_count do
		local result, data = socket:read()
		if result then
			--print("client readed "..(context.self).."_"..i .." :", data)
		end
	end
else
	print("tcp.connect connect failed: ", context.strerror(socket))
end