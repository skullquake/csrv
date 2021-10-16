#CC=x86_64-linux-gnu-gcc-9
#CC=x86_64-w64-mingw32-gcc-9.3-posix
ifeq ($(CC),)
	CC=x86_64-linux-gnu-gcc-9
endif
SRC=./src/a.c
COMPILERVERSION=$(shell $(CC) -dumpversion)
TGT_MACHINE=$(shell $(CC) -dumpmachine)
LIBNAM=$(shell basename `pwd`)
LIBAPPNAM=app
LIBAPPDIR=../../bin/$(TGT_MACHINE)/
LIBAPP=../../bin/$(TGT_MACHINE)/lib$(LIBAPPNAM)
CFLAGS=
LDFLAGS=
#DEPLOYDIR=../../lib
DEPLOYDIR=../../www
CFLAGS+=-Wall -I./src -I../app/src/mongoose
LDFLAGS+=-shared -fpic
ifneq (,$(findstring mingw,$(TGT_MACHINE)))
	LDFLAGS+=-L$(LIBAPPDIR)
	LDFLAGS+=-l$(LIBAPPNAM)
	LDFLAGS+=-Wl,--export-all-symbols
	ifneq (,$(findstring posix,$(TGT_MACHINE)))
	else
	endif
else
	DFLAGS+=-Wl,--export-dynamic 
endif
ifneq (,$(findstring mingw,$(TGT_MACHINE)))
	DSOEXT=.dll
else
	DSOEXT=.so
endif
BIN=./bin/$(TGT_MACHINE)/$(LIBNAM)$(DSOEXT)
all:deploy
$(BIN):$(SRC) $(LIBAPP)
	@-mkdir -p ./bin/$(TGT_MACHINE)
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS) -o $(BIN)
$(LIBAPP):
	make -C ../app
deploy:$(BIN)
	@-mkdir -p $(DEPLOYDIR)
	@cp $(BIN) $(DEPLOYDIR)
.phony:clean
clean:
	@-echo removing $(BIN)
	@rm $(BIN)
	@-echo removing $(DEPLOYDIR)/$(LIBNAM)$(DSOEXT)
	@-rm $(DEPLOYDIR)/$(LIBNAM)$(DSOEXT)
