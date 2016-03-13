math.randomseed(os.time())
local port = 8000
local services = {}
for i = 1, 100 do
	services[i] = context.create("timer.lua", port + i)
	context.wait(services[i], 2.1, function(result, error, handle)
		print("context.wait", i, result, error, handle)
		timer.sleep(math.random(1, 10))
		local result, socket = tcp.connect("127.0.0.1", port + i, 2)
		if result then
			socket:setopt({ read_head_bytes = 2, write_head_bytes = 2, })
			socket:write("hello world : " .. i)
		else
			print("tcp.connect fail:", result, socket)
		end
	end)
	context.send(services[i], "hihi"..i)
end

timer.sleep(15)
for i = 1, 100 do
	context.destroy(services[i])
end