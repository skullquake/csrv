#CC=x86_64-linux-gnu-gcc-9
#CC=x86_64-w64-mingw32-gcc-9.3-posix
#CC=x86_64-w64-mingw32-gcc-posix
#CXX=x86_64-w64-mingw32-g++-posix
ifeq ($(CC),)
	CC=gcc
endif
ifeq ($(CXX),)
	CXX=g++
endif
UPX=upx
COMPILERVERSION=$(shell $(CC) -dumpversion)
TGT_MACHINE=$(shell $(CC) -dumpmachine)
SRCDIR=./src
BINDIR=./bin/$(TGT_MACHINE)
SRC:=$(shell find $(SRCDIR) -name '*.c')
OBJS:=$(addprefix $(BINDIR)/,$(SRC:%.c=%.o))
SRCCPP:=$(shell find $(SRCDIR) -name '*.cpp')
OBJSCPP:=$(addprefix $(BINDIR)/,$(SRCCPP:%.cpp=%.o))
LIBNAM=$(shell basename `pwd`)
LIBAPPNAM=app
LIBAPPDIR=../../bin/$(TGT_MACHINE)/
LIBAPP=../../bin/$(TGT_MACHINE)/lib$(LIBAPPNAM)
DEPLOYDIR=../../www
CFLAGS+=-Wall
CFLAGS+=-I./src
CFLAGS+=-I./src/quickjs
CFLAGS+=-I../app/src/mongoose
CFLAGS+=-D_GNU_SOURCE
CFLAGS+=-DCONFIG_VERSION=\"2020-11-08\"
CFLAGS+=-DHAVE_STRUCT_TIMEVAL
CXXFLAGS+=$(CFLAGS)
CXXFLAGS+=-std=c++17
CFLAGS+=-fpermissive
#CFLAGS+=-DCONFIG_ATOMICS
#CFLAGS+=-DENABLE_TCC
#CFLAGS+=-DCS_ENABLE_DEBUG
CFLAGS+=-DCS_P_UNIX
LDFLAGS+=-shared
LDFLAGS+=-fpic
LDFLAGS+=-fvisibility=hidden
ifneq (,$(findstring mingw,$(TGT_MACHINE)))
	LDFLAGS+=-L$(LIBAPPDIR)
	LDFLAGS+=-static
	LDFLAGS+=-Wl,-Bdynamic
	#dynamically linked libs
	LDFLAGS+=-l$(LIBAPPNAM)
	LDFLAGS+=-Wl,-Bstatic
	#statically linked libs
	LDFLAGS+=-ldl
	LDFLAGS+=-lpthread
	LDFLAGS+=-Wl,--export-all-symbols
	ifneq (,$(findstring posix,$(TGT_MACHINE)))
	else
	endif
else
	LDFLAGS+=-Wl,--export-dynamic 
	LDFLAGS+=-ldl
	LDFLAGS+=-lpthread
endif
ifneq (,$(findstring mingw,$(TGT_MACHINE)))
	DSOEXT=.dll
else
	DSOEXT=.so
endif
BIN=$(BINDIR)/$(LIBNAM)$(DSOEXT)
all:deploy
$(BIN):$(OBJS) $(OBJSCPP) $(LIBAPP)
	@-mkdir -p $(BINDIR)/$(TGT_MACHINE)
	$(CXX)\
		$(OBJS) $(OBJSCPP)\
		$(LDFLAGS)\
		-o $(BIN)
	#@printf "compressing $@...\n"
	#@-$(UPX) $(BIN)
$(BINDIR)/%.o: %.c
	@printf "generating [c]   $@...\n"
	@mkdir -p $(@D)
	@$(CC)\
		$(CFLAGS) \
		-c $< \
		$(LDFLAGS) \
		-o $@
$(BINDIR)/%.o: %.cpp
	@printf "generating [cpp] $@...\n"
	@mkdir -p $(@D)
	@$(CXX)\
		$(CXXFLAGS) \
		-c $< \
		$(LDFLAGS) \
		-o $@
$(LIBAPP):
	@-make -C ../app
deploy:$(BIN)
	@-mkdir -p $(DEPLOYDIR)
	@cp $(BIN) $(DEPLOYDIR)
.phony:clean
clean:
	@-echo removing $(BIN)
	@-rm $(BIN) $(OBJS) $(OBJSCPP)
	@-echo removing $(DEPLOYDIR)/$(LIBNAM)$(DSOEXT)
	@-rm $(DEPLOYDIR)/$(LIBNAM)$(DSOEXT)

