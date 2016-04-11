
local fd, write_count = ...
fd = tonumber(fd)
write_count = tonumber(write_count)

local counter = 0
context.recv(0, function(result, recv_handle, session, type, data, data2)
	counter = counter + 1
	--print("agent["..(context.self).."] recved: ", result, recv_handle, session, type, data, data2)
	tcp.write(fd, data)
	if counter >= write_count then
		context.recv(0, nil)
	end
end)
