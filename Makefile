# # makefile for lua hierarchy

# all:
# 	(cd src; make)
# 	(cd clients/lib; make)
# 	(cd clients/lua; make)

# clean:
# 	(cd src; make clean)
# 	(cd clients/lib; make clean)
# 	(cd clients/lua; make clean)


LUA ?= $(PWD)
export LUA

all:
	(cd src; $(MAKE))
	(cd clients/lib; $(MAKE))
	(cd clients/lua; $(MAKE))

clean:
	(cd src; $(MAKE) clean)
	(cd clients/lib; $(MAKE) clean)
	(cd clients/lua; $(MAKE) clean)