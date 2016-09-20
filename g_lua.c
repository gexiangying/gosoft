#include "g_lua.h"
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include "trace.h"
int trace_out(lua_State* L)
{
	char buf[1024]={0};
	size_t len = 0;
	const char* str = lua_tolstring(L,1,&len);
	int num = len /1023;
	int left = len % 1023;
	int i=0;
	for (i=0;i < num;i++){
		strncpy(buf,str,1023);
		str = str + 1023;
		buf[1023] = '\0';
		TRACE_OUT(buf);
	}
  strncpy(buf,str,left);
	buf[left] = '\0';
	TRACE_OUT(buf);
	return 0;
}
void error(lua_State* L,const char* fmt,...)
{
	char buf[1024] = {0};
	va_list argp;
	va_start(argp,fmt);
	vsprintf(buf,fmt,argp);
	va_end(argp);
	TRACE_OUT(buf);
	//lua_close(L);
}
//call_lua(L,"f","dd>d",x,y,&z)
void call_lua(lua_State* L,const char* func,const char* sig,...){
	if(L == NULL) return;
	va_list vl;
	int narg,nres;
	lua_getglobal(L,func);
	if(!lua_isfunction(L,-1)){
		lua_pop(L,1);
		error(L,"%s function is nil\n",func);
		return;
	}
	va_start(vl,sig);
	//push param
	for(narg = 0; *sig; narg++){
		lua_checkstack(L,1);
		switch(*sig++){
			case 'd':
				lua_pushnumber(L,va_arg(vl,double));
				break;
			case 'i':
				lua_pushinteger(L,va_arg(vl,int));
				break;
			case 's':
				lua_pushstring(L,va_arg(vl,char*));
				break;
			case 'l':
				{
					const char* str = va_arg(vl,const char*);
					int len = va_arg(vl,int);
					lua_pushlstring(L,str,len);
				}
				break;
			case 'u':
				lua_pushlightuserdata(L,va_arg(vl,void*));
				break;
			case '>':
				goto endargs;
			default:
				error(L,"invalid option (%c)",*(sig-1));
				break;
		}
	}
endargs:
	nres = strlen(sig);
	int npop = nres;
	if(lua_pcall(L,narg,nres,0) != 0){
		error(L,"*error calling '%s' : %s",func,lua_tostring(L,-1));
		lua_pop(L,1);
		va_end(vl);
		return;
	}
	//return param
	nres = - nres;
	while(*sig){
		switch(*sig++){
			case 'd':
				if(!lua_isnumber(L,nres))
					error(L,"wrong result type");
				*va_arg(vl,double*) = lua_tonumber(L,nres);
				break;
			case 'u':
				if(!lua_isuserdata(L,nres))
					error(L,"wrong result type");
				*va_arg(vl,void**) = lua_touserdata(L,nres);
				break;
			case 'i':
				if(!lua_isnumber(L,nres))
					error(L,"wrong result type");
				*va_arg(vl,int*) = lua_tointeger(L,nres);
				break;
			case 's':
				if(!lua_isstring(L,nres))
					error(L,"wrong result type");
				const char* str = lua_tostring(L,nres);
				char* buf = (char*)calloc(strlen(str) + 1,sizeof(char));
				strncpy(buf,str,strlen(str));
				*va_arg(vl,char**) = buf;
				break;
			default:
				error(L,"invlid option (%c)",*(sig-1));
		}
		nres++;
	}
	lua_pop(L,npop);
	va_end(vl);
}
