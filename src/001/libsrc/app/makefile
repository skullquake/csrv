#CC=x86_64-linux-gnu-gcc-9
#CC=x86_64-w64-mingw32-gcc-9.3-posix
ifeq ($(CC),)
	CC=x86_64-linux-gnu-gcc-9
endif
SRC=\
	./src/a.c\
	./src/mongoose/mongoose.c\
	./src/mjson/mjson.c\
	./src/dlfcn/dlfcn.c\
	./src/map/map.c
COMPILERVERSION=$(shell $(CC) -dumpversion)
TGT_MACHINE=$(shell $(CC) -dumpmachine)
LIBNAM=lib$(shell basename `pwd`)
CFLAGS=
LDFLAGS=
DEPLOYDIR=../../bin/$(TGT_MACHINE)
CFLAGS+=-Wall
CFLAGS+=-I./src
CFLAGS+=-I./src/mongoose
CFLAGS+=-I./src/mjson
CFLAGS+=-I./src/map
CFLAGS+=-DMG_ENABLE_SSI=1
LDFLAGS+=-shared -fpic
ifneq (,$(findstring mingw,$(TGT_MACHINE)))
	LDFLAGS+=-lwsock32
	LDFLAGS+=-lws2_32
	#--------------------------------------------------------------------------------
	#required for ./libsrc/ artifacts (loaded vy qjs import) to obtain symbols
	#--------------------------------------------------------------------------------
	LDFLAGS+=-Wl,--export-all-symbols
	ifneq (,$(findstring posix,$(TGT_MACHINE)))
	else
	endif
else
	LDFLAGS+=-ldl
	#--------------------------------------------------------------------------------
	#required for ./libsrc/ artifacts (loaded vy qjs import) to obtain symbols
	#--------------------------------------------------------------------------------
	LDFLAGS+=-Wl,--export-dynamic 
	LDFLAGS+=-rdynamic
endif
ifneq (,$(findstring mingw,$(TGT_MACHINE)))
	DSOEXT=.dll
else
	DSOEXT=.so
endif
LIB=./bin/$(TGT_MACHINE)/$(LIBNAM)$(DSOEXT)
all:deploy
$(LIB):$(SRC)
	@-mkdir -p ./bin/$(TGT_MACHINE)
	@$(CC) $(CFLAGS) $(SRC) $(LDFLAGS) -o $(LIB)
deploy:$(LIB)
	@-mkdir -p $(DEPLOYDIR)
	@cp $(LIB) $(DEPLOYDIR)
.phony:clean
clean:
	@-echo removing $(LIB)
	@-rm $(LIB)
	@-echo removing $(DEPLOYDIR)/$(LIBNAM)$(DSOEXT)
	@-rm $(DEPLOYDIR)/$(LIBNAM)$(DSOEXT)
