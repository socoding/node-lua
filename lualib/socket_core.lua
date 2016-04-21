local tcp = tcp
local context = context
local error = error
local type = type
local pcall = pcall
local select = select
local unpack = table.unpack
local setmetatable = setmetatable

local socket = {
	_VERSION = "LuaSocket 2.0.2",
}

function socket.skip(count, ...)
	return unpack({ ... }, count + 1)
end
	
function socket.newtry(finalize)
	return function(...)
		if select(1, ...) then
			return ...
		end
		local length = select("#", ...)
		local status = pcall(finalize)
		if length >= 2 then
			error({ select(2, ...) })
		else
			error({ status })
		end
	end
end

function socket.protect(func)
	return function(...)
		local result = { pcall(func, ...) }
		if result[1] then
			return unpack(result, 2)
		end
		local ret = result[2]
		if type(ret) == "table" then
			return nil, ret
		else
			return error(ret)
		end
	end
end

--tcp module
local lua_tcp_meta = {}
local lua_tcp_mode = {}

lua_tcp_meta.__call  = function()
	local tcp_socket = {
		_sock = nil,
		_is_connecting = false,
		_received_buffer = nil,
		_timeout = nil,
	}
	return setmetatable(tcp_socket, lua_tcp_mode)
end

lua_tcp_mode.__index = lua_tcp_mode
setmetatable(lua_tcp_mode, lua_tcp_meta)

function lua_tcp_mode:getfd()
	assert(false, "function getfd() not implemented")
end

function lua_tcp_mode:dirty()
	assert(false, "function dirty() not implemented")
end

function lua_tcp_mode:connect(address, port)
	if self._sock then
		return false, "socket already binded"
	end
	if self._is_connecting then
		return false, "socket already connecting"
	end
	self._is_connecting = true
	local result, sock
	if port then
		if self._timeout and self._timeout > 0 then
			result, sock = tcp.connect(address, port, self._timeout)
		else
			result, sock = tcp.connect(address, port)
		end
	else
		if self._timeout and self._timeout > 0 then
			result, sock = tcp.connects(address, self._timeout)
		else
			result, sock = tcp.connects(address)
		end
	end
	self._is_connecting = false
	if result then
		self._sock = sock
		return true
	else
		return false, context.strerror(sock)
	end
end

function lua_tcp_mode:settimeout(timeout)
	self._timeout = timeout
	return true
end

function lua_tcp_mode:setoption(field, value)
	local sock = self._sock
	if not sock or sock:is_closed() then
		return false, "socket not invalid or closed"
	end
	if field == 'tcp-nodelay' then
		sock:set_nodelay(value)
		return true
	else
		assert(false, "set option: "..tostring(field).." not implemented")
	end
end

function lua_tcp_mode:send(buffer)
	local sock = self._sock
	if not sock or sock:is_closed() then
		return false, "socket not invalid or closed"
	end
	sock:write(buffer)
	return true
end

local function try_read(tcp_socket)
	local result, buffer = tcp_socket._sock:read()
	if result then
		if not tcp_socket._received_buffer then
			tcp_socket._received_buffer = buffer
		else
			tcp_socket._received_buffer:append(buffer)
		end
	else
		--to be fix : half close http???
		tcp_socket._sock:close()
		tcp_socket._sock = nil
		tcp_socket._received_buffer = nil
		return context.strerror(buffer)
	end
end

	
function lua_tcp_mode:receive(len, header)
	local sock = self._sock
	if not sock or sock:is_closed() then
		return "", "socket not invalid or closed"
	end
	
	if not len or len == '*l' then
		if not self._received_buffer or self._received_buffer:length() == 0 then
			local error = try_read(self)
			if error then
				return "", error
			end
		end
		while true do
			local len = self._received_buffer:find("\n")
			if len then
				local received = self._received_buffer:split(len):tostring()
				return header and header..received or received
			end
			local error = try_read(self)
			if error then
				return "", error
			end
		end
	end
	
	if type(len) == "number" then
		if len <= 0 then
			return "", "receive length can't be less than 0"
		end
		while not self._received_buffer or self._received_buffer:length() < len do
			local error = try_read(self)
			if error then
				return "", error
			end
		end
		local received = self._received_buffer:split(len):tostring()
		return header and header..received or received
	end
	
	return "", "unsupported receive format"
end

socket.tcp = lua_tcp_mode

return socket
