#BIN = /cygdrive/d/MinGWStudio/MinGw/bin/
#BIN = /cygdrive/c/mingw/bin/
CC = $(BIN)gcc 
TITLE = gsoft
#VERSION = $(shell git describe --tags)
#ifeq ($(version),1)
#PRJ = gcad-$(VERSION)
#else
PRJ = gsoft
#endif
#CFLAGS = -mno-cygwin
CFLAGS = 
ifeq ($(notrace),1)
C_PROC= 
else
C_PROC = -DENABLE_TRACE
endif
ifeq ($(test),1)
C_PROC += -DENABLE_TEST
endif
INCPATH = -I/usr/include
LIBPATH = -L/usr/lib
LIBS = -lws2_32 -lAdvapi32 -llua5.1 -ltrace
#LDFLAGS = -mwindows -mno-cygwin
LDFLAGS = -mwindows
DEPS = $(TITLE).dep
SRCS :=$(wildcard *.c)
HPPS :=$(wildcard *.h)
OBJS :=$(patsubst %.c,%.o,$(SRCS))
#RRCS :=$(wildcard *.rc)
#LUASRCS :=$(wildcard *.lua)
#LUAOBJS :=$(patsubst %.lua,%.lob,$(LUASRCS))
#RRCS_OBJS :=$(patsubst %.rc,%.res,$(RRCS))
#RRCS_OBJS :=$(PRJ).res
#WINDRES = $(BIN)windres
all:$(PRJ).exe $(DEPS) tags 
tags:$(SRCS) $(HPPS)
	ctags *.c *.h
$(PRJ).exe:$(OBJS) 
	$(CC) -o $@ $(OBJS) $(LIBPATH) $(LIBS) $(LDFLAGS)
%.o:%.c	
	$(CC) $(CFLAGS) $(C_PROC) $(INCPATH) -c $< 
$(DEPS):$(SRCS) $(HPPS) 
	$(CC) -MM $(INCPATH) *.c >$(DEPS)
#-@if test ! -r "$(DEPS)";then echo>$(DEPS);fi
#	makedepend  -f$(DEPS)>&/dev/null *.c
-include $(DEPS)
install:
	cp $(PRJ).exe bin/
clean:
	-@rm *.o *.rc *.exe *.dep *.res *.lob tags 
