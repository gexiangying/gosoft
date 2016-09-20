ip = "192.168.5.224"
port = 25
trace = true 
--trace_out
--tcp_connect
--send_msg
--close_socket

--local s  = tcp_connect(ip,port)

--
--local cmds = {"tasklist","systeminfo","whoami","cmd.exe /C dir","cmd.exe /C echo %username%"}
local cmds = {"whoami","cmd.exe /C dir","cmd.exe /C echo %username%"}
--local dirs = os.execute("dir")
--trace_out(dirs)
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
function on_time()
	trace_out("on_time\n")
	for i,v in ipairs(cmds) do
		local rs = shell_exec(v)
		trace_out(table.concat(rs))
	end
end

function on_quit()
	close_socket(s)
end
