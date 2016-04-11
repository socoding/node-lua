
local client_count = 100
local write_count = 100
local fd_clents = {}
local agent_clents = {}

local function socket_read_func(result, buffer, socket)
	local fd = socket:fd()
	local client = fd_clents[fd]
	if not result then
		if client then
			fd_clents[fd] = nil
			agent_clents[client.agent] = nil
			context.destroy(client.agent, "socket closed: " .. context.strerror(buffer))
		end
		socket:close()
		return
	end
	--print("socket_read_func(result, buffer, socket)", result, buffer, socket)
	context.send(client.agent, 1, buffer)
end

local result, listen_socket = tcp.listen("127.0.0.1", 9901)
if result then
	listen_socket:accept(function(result, accept_socket)
		if result then
			local fd = accept_socket:fd()
			accept_socket:set_rwopt({ read_head_endian =  "L", read_head_bytes = 2, read_head_max = 65535, write_head_endian =  "L", write_head_bytes = 2, write_head_max = 65535, })
			accept_socket:set_wshared(true)
			local agent = context.create("sample/agent.lua", fd, write_count)
			local client = { fd = fd, agent = agent, addr = accept_socket:remote_addr(), }
			fd_clents[fd] = client
			agent_clents[agent] = client
			accept_socket:read(socket_read_func)
		else
			print("server accept error: ", context.strerror(accept_socket))
		end
	end)
	local clients = {}
	for i = 1, client_count do --n clients will create n agent
		clients[i] = context.create("sample/client.lua", "127.0.0.1", 9901, write_count)
	end
	for i = 1, #clients do
		context.wait(clients[i])
		--print("client "..i .. "waited back!")
	end
	listen_socket:close()
else
	print("server listen error: ", context.strerror(listen_socket))
end

