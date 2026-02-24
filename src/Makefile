# # makefile for lua

# LIB= $(LUA)/lib
# INC= $(LUA)/include

# CC= gcc
# CFLAGS= -g -Wall -O2 -I$(INC) $(DEFS)
# DEFS= -DMAXCODE=64000 -DMAXCONSTANT=1024 -DMAXSYMBOL=1024 -DMAXARRAY=1024

# OBJS= hash.o inout.o lex.o opcode.o table.o y.tab.o
# SLIB= $(LIB)/liblua.a
# DLIB= $(LIB)/liblua.so.1.1

# libs: $(SLIB) $(DLIB)

# $(SLIB): $(OBJS)
# 	ar ruvl $@ $(OBJS)
# 	ranlib $(SLIB)

# $(DLIB): $(OBJS)
# 	ld -o $@ $(OBJS)

# clean:
# 	rm -f $(OBJS) $(SLIB) $(DLIB)





LUA= ..
LIB= $(LUA)/lib
INC= $(LUA)/include

CC= /usr/bin/gcc
# 编译动态库必须加上 -fPIC
CFLAGS= -g -Wall -O2 -I$(INC) -DMAXCODE=64000 -std=gnu89 -fPIC

OBJS= hash.o inout.o lex.o opcode.o table.o y.tab.o
SLIB= $(LIB)/liblua.a
DLIB= $(LIB)/liblua.so.1.1

all: $(SLIB) $(DLIB)

$(SLIB): $(OBJS)#静态库
	/usr/bin/ar rcu $@ $(OBJS)
	/usr/bin/ranlib $@

$(DLIB): $(OBJS)#动态库
	$(CC) -shared -o $@ $(OBJS)

clean:
	rm -f $(OBJS) $(SLIB) $(DLIB)