--print(package.path)
-- print(context.parent)
package.path = package.path .. ";..\\lualib\\?.lua"
package.cpath = package.cpath .. ";..\\luaclib\\?.dll"

print(require "cjson")
print(require "lfs")
print(require "lpeg")
print(require "mime")
print(require "protobuf.c")


do return end

-- local redis = require 'redis'
-- local client = redis.connect("10.4.0.32", 6379, 2)
-- client:set('usr:nrk', "hello world")
-- client:set('usr:nobody', 5)
-- local value1 = client:get('usr:nrk')
-- local value2 = client:get('usr:nobody')
-- print("value", value1, value2)

-- local http = require 'http'
-- print(http.request("www.baidu.com"))

-- function gethttpsize(u)
	-- local r, c, h = http.request {method = "HEAD", url = u}
	-- return r, c, h
-- end

-- print(gethttpsize("www.baidu.com"))

---------------------------------
print("this is the child service : ", context.self, context.parent)
print("context.recv blocking1: ", context.recv(0))
print("context.recv blocking2: ", context.recv(0))
local result = { context.recv(0) }
print("context.recv blocking3: ", unpack(result))
print("context.reply: ", result[2], result[3])
--timer.sleep(5)
context.reply(result[2], result[3], 1, 2, 3, "aaaa", { ["hello"] = true, })

print("context.recv blocking3: ", context.recv(0))
context.recv(0, function(result, recv_handle, session, ... )
		print("context.recv noblocking: ", result, recv_handle, session, ...)
	end
)

