# # makefile for lua interpreter

# BIN= $(LUA)/bin
# INC= $(LUA)/include
# LIB= $(LUA)/lib

# CC= gcc
# CFLAGS= -g -Wall -O2 -I$(INC)

# OBJS= lua.o
# T=$(BIN)/lua

# $T: $(OBJS)
# 	$(CC) -o $@ $(OBJS) -L$(LIB) -llua -llualib -lm

# clean:
# 	rm -f $T $(OBJS)




LUA= ../..
BIN= $(LUA)/bin
INC= $(LUA)/include
LIB= $(LUA)/lib

CC= /usr/bin/gcc
CFLAGS= -g -Wall -O2 -I$(INC) -std=gnu89 -Wno-implicit-function-declaration -Wno-int-conversion

OBJS= lua.o
T=$(BIN)/lua

# 核心链接规则：注意顺序，lualib 依赖 lua，所以 -llualib 要在 -llua 前面
$T: $(OBJS)
	$(CC) -o $@ $(OBJS) -L$(LIB) -llualib -llua -lm

clean:
	rm -f $T $(OBJS)