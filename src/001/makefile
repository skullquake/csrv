#CC=x86_64-linux-gnu-gcc-9
#CC=x86_64-w64-mingw32-gcc-9.3-posix
ifeq ($(CC),)
	CC=x86_64-linux-gnu-gcc-9
endif
COMPILERVERSION=$(shell $(CC) -dumpversion)
TGT_MACHINE=$(shell $(CC) -dumpmachine)
PORT=1212
APPNAM=a
BINDIR=./bin/
LIBAPPNAM=app
LIBAPPDIR=./libsrc/$(LIBAPPNAM)/
LIBAPP=$(LIBAPPDIR)bin/$(TGT_MACHINE)/lib$(LIBAPPNAM)
CFLAGS=
LDFLAGS=
CFLAGS+=-Wall -I./src -I./src/mongoose
LDFLAGS+=-L$(BINDIR)$(TGT_MACHINE)
LDFLAGS+=-l$(LIBAPPNAM)
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
	#multiple rpaths???
	LDFLAGS+=-Wl,-rpath,./:./lib:$(BINDIR)$(TGT_MACHINE)/
	#--------------------------------------------------------------------------------
	#required for ./libsrc/ artifacts (loaded vy qjs import) to obtain symbols
	#--------------------------------------------------------------------------------
	LDFLAGS+=-Wl,--export-dynamic 
endif
ifneq (,$(findstring mingw,$(TGT_MACHINE)))
	BIN=$(BINDIR)$(TGT_MACHINE)/$(APPNAM).exe
	APPLIB=$(BINDIR)$(TGT_MACHINE)/$(APPNAM).dll
else
	BIN=$(BINDIR)$(TGT_MACHINE)/$(APPNAM).out
	APPLIB=$(BINDIR)$(TGT_MACHINE)/$(APPNAM).so
endif
SRC=\
    ./src/a.c
$(BIN):$(SRC) $(LIBAPP)
	-mkdir -p $(BINDIR)$(TGT_MACHINE)/app
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS) -o $(BIN)
$(LIBAPP):
	make -C $(LIBAPPDIR)
run:$(BIN)
	echo running $(BIN) $(PORT)
	@$(BIN) $(PORT)
run_centos:LD_LIBRARY_PATH=/usr/local/lib64
run_centos:$(BIN)
	echo running $(BIN) $(PORT)
	@$(BIN) $(PORT)
proxy:$(BIN)
	ngrok http 8000
openport:
	firewall-cmd --zone=public --permanent --add-port=$(PORT)/tcp
	firewall-cmd --zone=public --permanent --add-port=$(PORT)/udp
	firewall-cmd --reload
test:$(BIN)
	curl http://localhost:8000/lib/a.so
	curl http://localhost:8000/lib/a.dll
	curl http://localhost:8000/lib/b.so
	curl http://localhost:8000/lib/b.dll
	curl http://localhost:8000/lib/a
	curl http://localhost:8000/lib/b
.phony:clean
clean:
	@rm $(BIN)

