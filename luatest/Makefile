CC = clang
CFLAGS = -Wall -O2 `pkg-config --cflags sqlite3 lua-5.4`
LDFLAGS = `pkg-config --libs sqlite3 lua-5.4` -lm -ldl
SRC = luatest.c
all: luatest

myinit: $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
