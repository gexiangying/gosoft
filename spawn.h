#ifndef _SPAWN_H_
#define _SPAWN_H_
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
typedef struct _spawn_pipe_* SPAWN_PIPE;
int spawn_child(const char* cmd,SPAWN_PIPE pipes);
SPAWN_PIPE create_spawnpipe();
void free_spawnpipe(SPAWN_PIPE pipes);
int pipe_printf(SPAWN_PIPE pipes,const char* format,...);
char* pipe_getline(SPAWN_PIPE pipes,char* buf,int max_size);
void pipe_closein(SPAWN_PIPE pipes);
void pipe_closeout(SPAWN_PIPE pipes);
int luaopen_luaext_pipe(lua_State* L);
#endif
