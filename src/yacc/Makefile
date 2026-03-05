# makefile for lua

LIB= $(LUA)/lib
INC= $(LUA)/inc

CC= gcc
CFLAGS= -g -Wall -O2 -I$(INC) $(DEFS)
DEFS= -DMAXCODE=64000 -DMAXCONSTANT=1024 -DMAXSYMBOL=1024 -DMAXARRAY=1024

OBJS= hash.o inout.o lex.o opcode.o table.o y.tab.o
SLIB= $(LIB)/lua.a
DLIB= $(LIB)/liblua.so.1.1

libs: $(SLIB) $(DLIB)

$(SLIB): y.tab.c $(OBJS)
	ar ruvl $@ $(OBJS)
	ranlib $(SLIB)

$(DLIB): $(OBJS)
	ld -o $@ $(OBJS)

y.tab.c: lua.stx exscript
	# 1. 运行 yacc，产物先在当前目录
	yacc -d lua.stx
	# 2. 立即将产物移动到上一级目录 (src)
	mv y.tab.c y.tab.h ../
	# 3. 在上一级目录执行 ex 脚本（注意路径也要变）
	ex ../y.tab.c <exscript

clean:
	rm -f $(OBJS) $(SLIB) $(DLIB)
