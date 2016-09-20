#ifndef _G_LUA_H_
#define _G_LUA_H_
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
void error(lua_State* L,const char* fmt,...);
void call_lua(lua_State* L,const char* func,const char* sig,...);
int trace_out(lua_State* L);
#endif
