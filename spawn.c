#include "spawn.h"
#include <windows.h>
#include <stdarg.h>
#include <stdio.h>

#define MAX_PIPE_BUF 4096
struct _spawn_pipe_
{
	HANDLE in;
	HANDLE out;
	int cur_;
	int size_;
	char buf[MAX_PIPE_BUF];
};

static int get_imp(HANDLE in,char* buf,int max,int* cur)
{
	*cur = 0;
	DWORD count = 0;
	if(!ReadFile(in,buf,max,&count, 
				NULL) || count == 0) {
		return -1; 
	}
	return count; 
}
static int get_char(SPAWN_PIPE pipes)
{
	int c ;
	if(pipes->size_ == -1)
		return EOF;
	if(pipes->size_ == 0 && (pipes->size_ = get_imp(pipes->in,pipes->buf,
					MAX_PIPE_BUF,&pipes->cur_)) == -1 ){
		return EOF;
	}
	c = pipes->buf[pipes->cur_++];
	pipes->size_--;
	return c;
}
static char* get_line(SPAWN_PIPE pipes,char* buf,int lim)
{
	int c;
	char* org = buf;
	while(--lim && (c = get_char(pipes)) != EOF )
		if((*buf++ = c) == '\n')
			break;
	*buf = '\0';
	return (c == EOF && org == buf)?NULL:org;
}

void pipe_closein(SPAWN_PIPE pipes)
{
	CloseHandle(pipes->in);
}
void pipe_closeout(SPAWN_PIPE pipes)
{
	CloseHandle(pipes->out);
}
int pipe_printf(SPAWN_PIPE pipes,const char* format,...)
{
	char buf[MAX_PIPE_BUF] = {0};
	unsigned long count = 0;
	va_list argList;
  va_start(argList, format);
  vsprintf(buf,format,argList);
  va_end(argList);
	WriteFile(pipes->out,buf,strlen(buf),&count,NULL);
	return count;
}
char* pipe_getline(SPAWN_PIPE pipes,char* buf,int max_size)
{
	return get_line(pipes,buf,max_size);
}
SPAWN_PIPE create_spawnpipe()
{
	SPAWN_PIPE pipes = (SPAWN_PIPE)malloc(sizeof(struct _spawn_pipe_));
	if(!pipes) return 0;
	memset(pipes,0,sizeof(struct _spawn_pipe_));
	return pipes;
}
void free_spawnpipe(SPAWN_PIPE pipes)
{
	free(pipes);
}
static int create_child(const char* cmd,SPAWN_PIPE pipes)
{
	PROCESS_INFORMATION piProcInfo; 
	STARTUPINFO siStartInfo; 
	// Set up members of the PROCESS_INFORMATION structure. 
	ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );
	// Set up members of the STARTUPINFO structure. 
	ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
	siStartInfo.cb = sizeof(STARTUPINFO); 
	siStartInfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	siStartInfo.wShowWindow = SW_HIDE;
	siStartInfo.hStdInput = pipes->in;
	siStartInfo.hStdOutput = pipes->out;
	siStartInfo.hStdError = pipes->out;
	// Create the child process. 
	return CreateProcess(NULL, 
			(LPTSTR)cmd,       // command line 
			NULL,          // process security attributes 
			NULL,          // primary thread security attributes 
			TRUE,          // handles are inherited 
			0,             // creation flags 
			NULL,          // use parent's environment 
			NULL,          // use parent's current directory 
			&siStartInfo,  // STARTUPINFO pointer 
			&piProcInfo);  // receives PROCESS_INFORMATION 
}
static int dup_pipes(SPAWN_PIPE pipes)
{
	int result = 0;
	HANDLE temp;
	result = DuplicateHandle(GetCurrentProcess(), pipes->in,
        GetCurrentProcess(), &temp, 0,
        FALSE,
        DUPLICATE_SAME_ACCESS);
	CloseHandle(pipes->in);
	pipes->in = temp;
	result = DuplicateHandle(GetCurrentProcess(), pipes->out,
        GetCurrentProcess(), &temp, 0,
        FALSE,
        DUPLICATE_SAME_ACCESS);
	CloseHandle(pipes->out);
	pipes->out = temp;
	return result;
}
int spawn_child(const char* cmd,SPAWN_PIPE pipes)
{
	SECURITY_ATTRIBUTES saAttr; 
	struct _spawn_pipe_ child = {0};
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
	saAttr.bInheritHandle = TRUE; 
	saAttr.lpSecurityDescriptor = NULL; 
	if(!CreatePipe(&child.in,&pipes->out,&saAttr,0))
		return 0;
	if(!CreatePipe(&pipes->in,&child.out,&saAttr,0)){
		CloseHandle(child.in);
		CloseHandle(pipes->out);
		return 0;
	}
	dup_pipes(pipes);
	if(!create_child(cmd,&child))
		return 0;
	CloseHandle(child.in);
	CloseHandle(child.out);
	return 1;
}
static int lua_new_pipe(lua_State* L)
{
	const char* cmd = lua_tostring(L,1);
	//TRACE_OUT("lua_new pipe %s\n",cmd);
	SPAWN_PIPE pipes = (SPAWN_PIPE)lua_newuserdata(L,sizeof(struct _spawn_pipe_));
	if(!pipes){
		//error(L,"*error* : create pipes !\n");
		return 0;
	}
	memset(pipes,0,sizeof(struct _spawn_pipe_));
	if(spawn_child(cmd,pipes) == 0){
		//error(L,"*error* : spawn_child %s !\n",cmd);
		lua_pop(L,1);
		free(pipes);
		lua_pushnil(L);
		return 1;
	}
	luaL_getmetatable(L,"ge.pipe");
	lua_setmetatable(L,-2);
	return 1;
}
static int lua_pipe_print(lua_State* L)
{
	SPAWN_PIPE pipes = (SPAWN_PIPE)lua_touserdata(L,1);
	const char* str = lua_tostring(L,2);
	unsigned long count = 0;
	BOOL flag = WriteFile(pipes->out,str,strlen(str),&count,NULL);
	if(flag)
		lua_pushinteger(L,flag);
	else 
		lua_pushnil(L);
	return 1;
}
static int lua_pipe_closein(lua_State* L)
{
	SPAWN_PIPE pipes = (SPAWN_PIPE)lua_touserdata(L,1);
	pipe_closein(pipes);
	return 0;
}
static int lua_pipe_closeout(lua_State* L)
{
	SPAWN_PIPE pipes = (SPAWN_PIPE)lua_touserdata(L,1);
	pipe_closeout(pipes);
	return 0;
}
static int lua_pipe_getline(lua_State* L)
{
	char* buf = (char*)calloc(MAX_PIPE_BUF,sizeof(char));
	SPAWN_PIPE pipes = (SPAWN_PIPE)lua_touserdata(L,1);
	if(pipe_getline(pipes,buf,MAX_PIPE_BUF)){
		lua_pushstring(L,buf);
	}
	else{
		lua_pushnil(L);
	}
	free(buf);
	return 1;
}
static const struct luaL_Reg luapipe_f[] = {
	{"new",lua_new_pipe},
	{NULL,NULL}
};
static const struct luaL_Reg luapipe_m[] = {
	{"print",lua_pipe_print},
	{"closeout",lua_pipe_closeout},
	{"closein",lua_pipe_closein},
	{"getline",lua_pipe_getline},
	{NULL,NULL}
};
int luaopen_luaext_pipe(lua_State* L){
	luaL_newmetatable(L,"ge.pipe");
	lua_pushvalue(L,-1);
	lua_setfield(L,-2,"__index");
	luaL_register(L,NULL,luapipe_m);
	luaL_register(L,"pipe",luapipe_f);
	return 1;
}
