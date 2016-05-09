--[[
local redis = require 'redis'
local client = redis.connect("192.168.0.106", 6379)
client:set('usr:nrk', string.rep("1", 66000))
client:set('usr:nobody', 5)
--local value1 = client:get('usr:nrk')
local value2 = client:get('usr:nobody')
print("value", value1, value2)]]

-- local repeatid = nil
-- local repeatcount = 0
-- local function test_repeat(arg1, arg2)
	-- repeatcount = repeatcount + 1
	-- print("in repeat: ", arg1, arg2, repeatcount, os.time(), os.clock())
	-- if repeatcount >= 5 then
		-- timer.close(repeatid)
	-- end
-- end

--timer.sleep(2)
-- timer.timeout(1, 1, 2, 3, 4, function(a, b, c, d) print("this is timeout", a, b, c, d) end)
--repeatid = timer.loop(1, 0.050, "this is repeat arg", test_repeat)


-- local result, socket = tcp.connect("61.135.169.125", 443, 0.030)
-- if not result then
	-- print("tcp connect fail", context.strerror(socket))
-- else
	-- print("tcp connect success", socket)
-- end

-- local result, listen_socket = tcp.listen("127.0.0.1", 7710)
-- if result then
	-- local result, socket = listen_socket:accept(5)
	-- if result then
		-- print("tcp accept success", socket)
	-- else
		-- print("tcp accept fail", context.strerror(socket))
	-- end
-- else
	-- print("tcp listen fail", context.strerror(listen_socket))
-- end
--print("hello world")

--local service = context.create("listen.lua", 8000)
--context.query(service, "hello", 1, function(result, query_data, handle, data) print("context.query",result, query_data, handle, data) end) 
--print("context.query", context.query(service, "hello", 1))
--print("context.recv/10", context.recv(0, 4))
--print("context.wait", context.wait(service, 5))
--context.wait(service, 2, function(result, error, handle) print(result, context.strerror(error), handle) end)
--timer.sleep(3)
--context.wait(service, nil)
--context.destroy(service)

-- timer.sleep(10)
-- timer.timeout(5, 1, 2, 3, 4, function(a, b, c, d) print("this is timeout", a, b, c, d) end)

-- local ret, con_sock = tcp.connect("10.6.10.59", 8801)
-- print(ret, con_sock)
-- -- local ret, buffer = con_sock:read()
-- -- print("tcp.connect", ret, ret and buffer:tostring())

-- do return end

-- #ifdef _WIN32
-- # define TEST_PIPENAME "\\\\.\\pipe\\uv-test"
-- # define TEST_PIPENAME_2 "\\\\.\\pipe\\uv-test2"
-- #else
-- # define TEST_PIPENAME "/tmp/uv-test-sock"
-- # define TEST_PIPENAME_2 "/tmp/uv-test-sock2"
-- #endif

context.log(1, {})

do return end

local result, server = udp.open("127.0.0.1", 8081)
server:read(function(result, buffer, remote_addr, remote_port, remote_ipv6, server)
	print("server read callback: ", result, buffer, remote_addr, remote_port, remote_ipv6, server)
	server:write("hello, world server send!", remote_addr, remote_port, true)
end)

local result, client1 = udp.open()
local result, client2 = udp.open()

client1:read(function(result, buffer, remote_addr, remote_port, remote_ipv6, client)
	print("client1 read callback: ", result, buffer, remote_addr, remote_port, remote_ipv6, client)
	client1:close()
end)
client2:read(function(result, buffer, remote_addr, remote_port, remote_ipv6, client)
	print("client2 read callback: ", result, buffer, remote_addr, remote_port, remote_ipv6, client)
	client2:close()
end)

print("client1:write ", client1:write("hello, world1!", "127.0.0.1", 8081, true))
print("client2:write ", client2:write("hello, world2!", "127.0.0.1", 8081, true))

print("client1:addr ", client1:local_addr(), client1:local_port(), client1:remote_addr(), client1:remote_port())
print("client2:addr ", client2:local_addr(), client2:local_port(), client2:remote_addr(), client2:remote_port())

timer.sleep(0.1)
server:close()

--timer.sleep(3600)

do return end

--context send and recv test, memery leak test

local handle = context.create("sample/test.lua")

print("main service create sub service:", handle)

context.send(handle, 1, 2, 3, 4, nil, true, false, "hello", { 1, 2, 3, 4, nil, true, false, ["main"] = 1, })
context.send(handle, 1, 2, 3, 4, nil, true, false, "hello", { 1, 2, 3, 4, nil, true, false, ["main"] = 1, })
print("context.query", context.timed_query(handle, 0.5, "hello world"))
context.destroy(handle)

do return end

-- print(context.self, context.parent, handle)

if context.winos then
	package.cpath = package.cpath ..";".."..\\clib\\?.dll"
	package.path = package.path ..";".."..\\lualib\\?.lua"
else
	package.cpath = package.cpath ..";".."../clib/?.so"
	package.path = package.path ..";".."../lualib/?.lua"
end

-- for i = 1, 32 do
	-- context.create("test.lua", 1, 2, 3)
-- end
-- do return end

acceptSockets = {}
connecSockets = {}

local ret, sock = tcp.listen("0.0.0.0", 8801)
if not ret then
	print("tcp.listens failed: ", context.strerror(sock))
else
	print("tcp.listens success: ", sock)
	sock:accept(function(result, accept_sock)
		if result then
			acceptSockets[#acceptSockets+1] = accept_sock
			print("sock:accept success", #acceptSockets, accept_sock:fd(), accept_sock:local_port())
		else
			print("sock:accept fail", ret, context.strerror(accept_sock))
		end
		--sock:close()
		--accept_sock:set_nodelay(false)
		-- print("accept_sock local addr:", accept_sock:local_addr(), accept_sock:local_port())
		-- print("accept_sock remote addr:", accept_sock:remote_addr(), accept_sock:remote_port())
		-- accept_sock:set_rwopt({ read_head_endian =  "L", read_head_bytes = 2, read_head_max = 65535, write_head_endian =  "L", write_head_bytes = 2, write_head_max = 65535,})
		-- accept_sock:set_wshared(true)
		-- print("accept_sock:fd", accept_sock:fd())
		-- tcp.write(accept_sock:fd(), "hello world!", true)
		-- accept_sock:write("hello world!", function(result, error) print("tcp.write1", result, error) end)
		-- tcp.write(accept_sock:fd(), "hello world!", function(result, error) print("tcp.write2", result, error) end)
		-- accept_sock:close()
		-- tcp.write(accept_sock:fd(), "hello world!", function(result, error) print("tcp.write3", result, error) end)
	end)
	for i = 1, 100 do
		local ret, con_sock = tcp.connect("127.0.0.1", 8801)
		if ret then
			connecSockets[#connecSockets+1] = con_sock
			print("tcp.connect success", #connecSockets, con_sock:fd(), con_sock:remote_port(), con_sock:local_port())
			-- print("tcp.connect", ret, con_sock)
			-- print("con_sock local addr:", con_sock:local_addr(), con_sock:local_port())
			-- print("con_sock remote addr:", con_sock:remote_addr(), con_sock:remote_port())
			-- con_sock:set_rwopt({ read_head_endian =  "L", read_head_bytes = 2, read_head_max = 65535, write_head_endian =  "L", write_head_bytes = 2, write_head_max = 65535,})
			-- con_sock:set_wshared(true)
			-- local ret, buffer = con_sock:read()
			-- print("tcp read", ret, ret and buffer:tostring())
			-- local ret, buffer = con_sock:read()
			-- print("tcp read", ret, ret and buffer:tostring())
			-- local ret, buffer = con_sock:read()
			-- print("tcp read", ret, ret and buffer:tostring())
			-- local ret, buffer = con_sock:read()
			-- print("tcp read", ret, ret and buffer:tostring())
		else
			print("tcp.connect fail", ret, context.strerror(con_sock), #connecSockets)
			break
		end
	end
	sock:close()
end

--timer.sleep(1)


do return end

local port = 8080

local pair_count = 10
local listen_services = {}
for i = 1, pair_count do
	listen_services[i] = context.create("listen.lua", port + i)
end
local connect_services = {}
for i = 1, pair_count do
	connect_services[i] = context.create("connect.lua", port + i)
end



print("main service exiting!")
