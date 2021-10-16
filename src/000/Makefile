ifndef TOOLCHAINDIR
	TOOLCHAINDIR=$(shell dirname `which gcc`)/
endif
ifndef COMPILERPREFIX
	COMPILERPREFIX=
endif
ifndef COMPILERPOSTFIX
	COMPILERPOSTFIX=
endif
ifdef $(word 1, COMPILERPREFIX)
	SEP0=-
else
	SEP0=
endif
ifdef $(word 1, COMPILERPOSTFIX)
	SEP1=-
else
	SEP1=
endif
#ifeq ($(OS),Windows_NT)
#   HOST_OS := Windows
#   SHELL = cmd
#    ifeq '$(findstring ;,$(PATH))' ';'
#       HOST_OS := Windows
#    else
#        HOST_OS := $(shell uname 2>/dev/null || echo Unknown)
#        HOST_OS := $(patsubst CYGWIN%,Cygwin,$(HOST_OS))
#        HOST_OS := $(patsubst MSYS%,MSYS,$(HOST_OS))
#        HOST_OS := $(patsubst MINGW64%,MINGW,$(HOST_OS))
#        HOST_OS := $(patsubst MINGW%,MINGW64,$(HOST_OS))
#    endif
#    EXT := .exe
#else
#    UNAME_S := $(shell uname -s)
#    HOST_OS := $(UNAME_S)
#    ifeq ($(UNAME_S),Linux)
#    endif
#    ifeq ($(UNAME_S),Darwin)
#    endif
#    ifneq ($(filter arm%,$(UNAME_P)),)
#        HOST_OS=ARM
#    endif
#    EXT := .out
#endif
#ifeq ($(HOST_OS),$(filter $(HOST_OS),Windows_NT MINGW MINGW64 MSYS))
#        ifeq ($(PROCESSOR_ARCHITEW6432),AMD64)
#            HOST_PROC=AMD64
#        else
#        ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
#            HOST_PROC=AMD64
#        endif
#        ifeq ($(PROCESSOR_ARCHITECTURE),x86)
#            HOST_PROC=IA32
#        endif
#    endif
#else ifeq ($(HOST_OS),$(filter $(HOST_OS),Linux Darwin ARM))
#    UNAME_P := $(shell uname -p)
#    ifeq ($(UNAME_P),x86_64)
#        HOST_PROC=AMD64
#    endif
#    ifneq ($(filter %86,$(UNAME_P)),)
#        HOST_PROC=IA32
#    endif
#endif
MAKEFILEPATH:=$(abspath $(lastword $(MAKEFILE_LIST)))
CURRENTDIRNAM:=$(notdir $(patsubst %/,%,$(dir $(MAKEFILEPATH))))
CXX=$(TOOLCHAINDIR)/$(COMPILERPREFIX)$(SEP0)g++$(SEP1)$(POSTFIX)
CC=$(TOOLCHAINDIR)$(COMPILERPREFIX)$(SEP0)gcc$(SEP1)$(POSTFIX)
AR=$(TOOLCHAINDIR)$(COMPILERPREFIX)$(SEP0)ar$(SEP1)$(POSTFIX)
LD=$(TOOLCHAINDIR)$(COMPILERPREFIX)$(SEP0)ld
STRIP=$(TOOLCHAINDIR)$(COMPILERPREFIX)$(SEP0)strip
#CXX=$(TOOLCHAINDIR)/$(COMPILERPREFIX)$()g++$(SEP1)$(POSTFIX)
#CC=$(TOOLCHAINDIR)$(COMPILERPREFIX)$()gcc$(SEP1)$(POSTFIX)
#AR=$(TOOLCHAINDIR)$(COMPILERPREFIX)$()ar$(SEP1)$(POSTFIX)
#LD=$(TOOLCHAINDIR)$(COMPILERPREFIX)$()ld
#STRIP=$(TOOLCHAINDIR)$(COMPILERPREFIX)$()strip
MKDIR=/bin/mkdir
RM=/bin/rmdir
ECHO=/bin/echo
CP=/bin/cp
RM=/bin/rm
FIND=/bin/find
CPPCHECK=/usr/bin/cppcheck
COMPILERVERSION=$(shell $(CC) -dumpversion)
TGT_MACHINE=$(shell $(CC) -dumpmachine)$(SEP1)$(POSTFIX)
ifdef $(word 1, COMPILERVERSION)
	SEP2=-
else
	SEP2=
endif
SRCDIR=src
SRC:=$(shell find $(SRCDIR) -name '*.c' -o -name '*.cpp')
OBJROOT=./obj
OBJDIR=$(OBJROOT)/$(TGT_MACHINE)/
OBJS=$(addprefix $(OBJDIR),$(patsubst %.c,%.o ,$(patsubst %.cpp,%.o,$(SRC))))
ifneq (,$(findstring djgpp,$(TGT_MACHINE)))
	BINNAM=a
else
	BINNAM=$(CURRENTDIRNAM)
endif
RESDIR=src/res
APPRESDIR=./res
RES:=$(shell find $(RESDIR) -name '*.txt' -o -name '*.js'  -o -name '*.json')
RESOBJS=$(addprefix $(OBJDIR),$(patsubst %.js,%.o,$(patsubst %.txt,%.o,$(patsubst %.json,%.o,$(RES)))))
BUILDROOT=build
BINDIR=$(BUILDROOT)/$(TGT_MACHINE)/
BIN=$(BINDIR)$(BINNAM)$(BINEXT)
RUNSCRIPT=$(BINDIR)run$(SCREXT)
ifneq (,$(findstring mingw,$(TGT_MACHINE)))
	BINEXT=.exe
	SCREXT=.bat
else ifneq (,$(findstring djgpp,$(TGT_MACHINE)))
	BINEXT=.exe
	SCREXT=.bat
else ifneq (,$(findstring linux,$(TGT_MACHINE)))
	BINEXT=.out
	SCREXT=.sh
else
	BINEXT=.out
	SCREXT=.sh
endif
ifneq (,$(findstring mingw,$(TGT_MACHINE)))
	DSOEXT=.dll
else ifneq (,$(findstring djgpp,$(TGT_MACHINE)))
	DSOEXT=.dxe
else ifneq (,$(findstring linux,$(TGT_MACHINE)))
	DSOEXT=.so
else
	DSOEXT=.so
endif
LIBDIR=./build/$(TGT_MACHINE)/lib
LIBCOMPILERPREFIX=lib
ifneq (,$(findstring mingw,$(TGT_MACHINE)))
	UPX=upx
else ifneq (,$(findstring djgpp,$(TGT_MACHINE)))
	UPX=upx
else ifneq (,$(findstring linux,$(TGT_MACHINE)))
	UPX=echo skipping upx
else
	UPX=echo skipping upx
endif
#ifneq (,$(findstring ARM,$(HOST_PROC)))
#	UPX+= --android-shlib
#endif

CFLAGS+=-Wall
CFLAGS+=-O2
CFLAGS+=-I./src
CFLAGS+=-I./src/quickjs
CFLAGS+=-I./src/mongoose
CFLAGS+=-I./src/sqlite
CFLAGS+=-I./src/zlib
CFLAGS+=-I./src/pcre2
ifneq (,$(findstring djgpp,$(TGT_MACHINE)))
else ifneq (,$(findstring mingw,$(TGT_MACHINE)))
	#CFLAGS+=-fpermissive
else
endif
#CFLAGS+=-fvisibility=hidden
#ifneq (,$(findstring aarch64,$(TGT_MACHINE)))
#else
#	CFLAGS+=-fno-gnu-unique
#endif
CFLAGS+=-g
CFLAGS+=-MMD
CFLAGS+=-Wno-array-bounds
CFLAGS+=-Wno-format-truncation
CFLAGS+=-D_GNU_SOURCE
CFLAGS+=-DCONFIG_VERSION=\"2020-11-08\"
CFLAGS+=-DHAVE_STRUCT_TIMEVAL
#CFLAGS+=-DCONFIG_ATOMICS
CFLAGS+=-DENABLE_TCC
#CFLAGS+=-DCS_ENABLE_DEBUG
CFLAGS+=-DCS_P_UNIX
#dealing with leading underscores in embedded resources
ifneq (,$(findstring x86_64-w64-mingw32,$(TGT_MACHINE)))
	CFLAGS+=-DLEADING_UNDERSCORES
else
endif


#CFLAGS+=-DHAVE_BOOL_T
#CFLAGS+=-DHAVE_STDBOOL_H
CXXFLAGS+=$(CFLAGS)
ifneq (,$(findstring djgpp,$(TGT_MACHINE)))
	CXXFLAGS+=-DENTRYPOINT=\"_appmain\"
else
	CXXFLAGS+=-DENTRYPOINT=\"appmain\"
endif
#LDFLAGS+=-L$(LIBDIR)
# --------------------------------------------------------------------------------
#ifneq (,$(findstring djgpp,$(TGT_MACHINE)))
#	#import libs
#	LIBNAMS=$(shell (ls $(LIBDIR)|grep libi|cut -f1 -d".")|cut -c4- 2>/dev/null)
#else
#	#libs
#	LIBNAMS=$(shell (ls $(LIBDIR)|cut -f1 -d".")|cut -c4- 2>/dev/null)
#endif
#LIBFLAGS=$(addprefix -l,$(LIBNAMS))
# --------------------------------------------------------------------------------
ifneq (,$(findstring djgpp,$(TGT_MACHINE)))
else ifneq (,$(findstring mingw,$(TGT_MACHINE)))
else
	LDFLAGS+=-Wl,-rpath=./lib
	LDFLAGS+=-Wl,-rpath=$(LIBDIR)
endif
# --------------------------------------------------------------------------------
ifneq (,$(findstring mingw,$(TGT_MACHINE)))
	LDFLAGS+=-static
endif
ifneq (,$(findstring mingw,$(TGT_MACHINE)))
	# wrap dso dynspec: start
	ifneq (,$(findstring win32,$(TGT_MACHINE)))
		LDFLAGS+=-Wl,-Bstatic
	else
		LDFLAGS+=-Wl,-Bstatic
	endif
endif
#static libs here
#--static libstc++
ifneq (,$(findstring mingw,$(TGT_MACHINE)))
	LDFLAGS+=-lstdc++
endif

ifneq (,$(findstring mingw,$(TGT_MACHINE)))
 	LDFLAGS+=-lwsock32
	LDFLAGS+=-lws2_32
	ifneq (,$(findstring posix,$(TGT_MACHINE)))
		LDFLAGS+=-lpthread
	else
		LDFLAGS+=-lpthread
	endif
else
	LDFLAGS+=-ldl
	LDFLAGS+=-lpthread
endif
ifneq (,$(findstring mingw,$(TGT_MACHINE)))
	# wrap dso dynspec: start
	ifneq (,$(findstring win32,$(TGT_MACHINE)))
		LDFLAGS+=-Wl,-Bdynamic
	else
		LDFLAGS+=-Wl,-Bdynamic
	endif
endif
#dynamic libs here
# --------------------------------------------------------------------------------
ifeq (run,$(firstword $(MAKECMDGOALS)))
  RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  $(eval $(RUN_ARGS):;@:)
endif
all:$(RUNSCRIPT)
$(BIN):$(OBJS) $(RESOBJS)
	@echo "generating $@..."
	@-$(MKDIR) -p $(@D)
	@$(CXX) \
		$(OBJS) \
		$(RESOBJS) \
		$(LDFLAGS) \
		-o $(BIN) 
$(OBJDIR)%.o: %.c
	@echo "generating $@..."
	@-$(MKDIR) -p $(@D)
	@$(CC)\
		$(CFLAGS)\
		-c $<\
		-o $@
$(OBJDIR)%.o: %.cpp
	@echo "generating $@..."
	@-$(MKDIR) -p $(@D)
	@$(CXX)\
		$(CXXFLAGS)\
		-c $<\
		-o $@
#todo: make more generic
$(OBJDIR)%.o: %.txt
	@echo "generating $@..."
	@-$(MKDIR) $(@D)
	@$(LD)\
		-r\
		-b binary\
		-o $@\
		$<
#todo: make more generic
$(OBJDIR)%.o: %.json
	@echo "generating $@..."
	@-$(MKDIR) $(@D)
	@$(LD)\
		-r\
		-b binary\
		-o $@\
		$< 
strip:$(BIN)
	@echo "stripping $(BIN)..."
	@-$(STRIP) $(BIN) --strip-unneeded
compress:strip
	@echo "compressing $(BIN)..."
	@-$(UPX) $(BIN) 2>/dev/null
ifneq (,$(findstring mingw,$(COMPILERPREFIX)))
$(RUNSCRIPT):compress
	@echo "creating runscript for $(BIN)..."
	@echo "@echo off" > $(RUNSCRIPT)
	@echo "setlocal" >> $(RUNSCRIPT)
	@echo "set PATH=.\lib;%PATH%" >> $(RUNSCRIPT)
	@echo ".\\\$(BINNAM)$(BINEXT)" >> $(RUNSCRIPT)
else ifneq (,$(findstring djgpp,$(COMPILERPREFIX)))
$(RUNSCRIPT):compress
	@echo "creating runscript for $(BIN)..."
	@echo "@echo off" > $(RUNSCRIPT)
	@echo "set PATH=.\lib;%PATH%" >> $(RUNSCRIPT)
	@echo "set LD_LIBRARY_PATH=.\lib" >> $(RUNSCRIPT)
	@echo ".\\\$(BINNAM)$(BINEXT)" >> $(RUNSCRIPT)
else ifneq (,$(findstring linux,$(COMPILERPREFIX)))
$(RUNSCRIPT):compress
	@echo "creating runscript for $(BIN)..."
	@echo "#!/bin/bash" > $(RUNSCRIPT)
	@echo "./a.out" >> $(RUNSCRIPT)
else
$(RUNSCRIPT):compress
	@echo "creating runscript for $(BIN)..."
	@echo "#!/bin/bash" > $(RUNSCRIPT)
	@echo "./a.out" >> $(RUNSCRIPT)
endif
run:$(BIN)
	@echo "running $(BIN)..."
	@$(BIN) $(RUN_ARGS) 
.phony:clean sta
clean:
	@echo "cleaning ..."
	@echo "removing $(BIN)..."
	@-rm $(BIN)
	@echo "removing $(OBJDIR)..."
	@-rm -rf $(OBJDIR)
cleanall:
	@echo "removing $(BUILDROOT)..."
	@-$(RM) -rf $(BUILDROOT)
	@echo "removing $(OBJROOT)..."
	@-$(RM) -rf $(OBJROOT)
sta:
	@$(ECHO) "performing static analysis on $(SRC)..."
	@$(CPPCHECK) --enable=all $(SRC)
#--------------------------------------------------------------------------------
#todo: refine version control
#--------------------------------------------------------------------------------
statver:
	(cat ./src/res/version.json |jq '.')
incmajor:
	(cat ./src/res/version.json |jq '.major|=.+1'>/tmp/a.json)&&mv /tmp/a.json ./src/res/version.json
	(cat ./src/res/version.json |jq '.')
incminor:
	(cat ./src/res/version.json |jq '.minor|=.+1'>/tmp/a.json)&&mv /tmp/a.json ./src/res/version.json
	(cat ./src/res/version.json |jq '.')
incpatch:
	(cat ./src/res/version.json |jq '.patch|=.+1'>/tmp/a.json)&&mv /tmp/a.json ./src/res/version.json
	(cat ./src/res/version.json |jq '.')
decmajor:
	(cat ./src/res/version.json |jq '.major|=.-1'>/tmp/a.json)&&mv /tmp/a.json ./src/res/version.json
	(cat ./src/res/version.json |jq '.')
decminor:
	(cat ./src/res/version.json |jq '.minor|=.-1'>/tmp/a.json)&&mv /tmp/a.json ./src/res/version.json
	(cat ./src/res/version.json |jq '.')
decpatch:
	(cat ./src/res/version.json |jq '.patch|=.-1'>/tmp/a.json)&&mv /tmp/a.json ./src/res/version.json
	(cat ./src/res/version.json |jq '.')
ifeq (setmajor,$(firstword $(MAKECMDGOALS)))
  RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  $(eval $(RUN_ARGS):;@:)
endif
setmajor:
	(cat ./src/res/version.json |jq '.major|=$(RUN_ARGS)')&&mv /tmp/a.json ./src/res/version.json
	(cat ./src/res/version.json |jq '.')
ifeq (setminor,$(firstword $(MAKECMDGOALS)))
  RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  $(eval $(RUN_ARGS):;@:)
endif
setminor:
	(cat ./src/res/version.json |jq '.minor|=$(RUN_ARGS)'>/tmp/a.json)&&mv /tmp/a.json ./src/res/version.json
	(cat ./src/res/version.json |jq '.')
ifeq (setpatch,$(firstword $(MAKECMDGOALS)))
  RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  $(eval $(RUN_ARGS):;@:)
endif
setpatch:
	(cat ./src/res/version.json |jq '.patch|=$(RUN_ARGS)')&&mv /tmp/a.json ./src/res/version.json
	(cat ./src/res/version.json |jq '.')
#--------------------------------------------------------------------------------
test:
	echo $(SEP0)
	echo $(SEP1)
	echo $(COMPILERPREFIX)
	echo $(COMPILERPOSTFIX)
	echo $(CC)
	echo $(CXX)
