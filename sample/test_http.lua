-- if context.winos then
	-- package.cpath = package.cpath ..";".."..\\clib\\?.dll"
	-- package.path = package.path ..";".."..\\lualib\\?.lua"
-- else
	-- package.cpath = package.cpath ..";".."../clib/?.so"
	-- package.path = package.path ..";".."../lualib/?.lua"
-- end

if context.winos then
	package.cpath = package.cpath ..";".."clib\\?.dll"
	package.path = package.path ..";".."lualib\\?.lua"
else
	package.cpath = package.cpath ..";".."clib/?.so"
	package.path = package.path ..";".."lualib/?.lua"
end

local http = require "http"
local a, b = http.request("http://www.baidu.com")
for k, v in pairs(b) do
	print(k, v)
end