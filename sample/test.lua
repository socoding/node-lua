--print(package.path)
print(context.parent)
package.path = package.path .. ";..\\lualib\\?.lua"
package.cpath = package.cpath .. ";..\\clib\\?.dll"

local redis = require 'redis'
local client = redis.connect("10.4.0.32", 6379, 2)
client:set('usr:nrk', "hello world")
client:set('usr:nobody', 5)
local value1 = client:get('usr:nrk')
local value2 = client:get('usr:nobody')
print("value", value1, value2)

local http = require 'http'
print(http.request("www.baidu.com"))

function gethttpsize(u)
	local r, c, h = http.request {method = "HEAD", url = u}
	return r, c, h
end

print(gethttpsize("www.baidu.com"))
