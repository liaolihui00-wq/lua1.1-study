/*
** Module to control static tables
** TeCGraf - PUC-Rio
** $Id: table.h,v 2.1 1994/04/20 22:07:57 celes Exp $
*/

#ifndef table_h
#define table_h

extern Symbol *lua_table;//符号表，记录程序里的变量名
extern Word    lua_ntable;//当前符号表里有多少个符号

extern char  **lua_constant;//常量表，记录程序里的常量
extern Word    lua_nconstant;//常量总个数

extern char  **lua_string;//字符串表，记录程序里的字符串
extern Word    lua_nstring;//字符串总个数，同样的字符串只记录一次

extern Hash  **lua_array;//hash表数组，记录程序里的数组
extern Word    lua_narray;//hash表总个数

extern char   *lua_file[];//文件名表，记录程序里用到的.lua源码文件名
extern int     lua_nfile;//当前文件名表里有多少个文件名

extern Word    lua_block;//内存块管理
extern Word    lua_nentity;



int   lua_findsymbol      (char *s);//在符号表里查找符号s对应的索引
int   lua_findconstant    (char *s);//在常量表里查找常量s对应的索引

void  lua_travsymbol      (void (*fn)(Object *));//遍历符号表里所有的对象，并对每个对象调用函数fn
void  lua_markobject      (Object *o);//mark对象还在使用
void  lua_pack            (void);//pack整理内存空间
void  lua_stringcollector (void);//stringcollecor回收字符串表里不再使用的字符串

char *lua_createstring    (char *s);//创建字符串s，如果s已经存在于字符串表里，返回字符串表里对应的字符串
int   lua_addfile         (char *fn);//源码文件栈管理，在文件名表里添加文件名fn
int   lua_delfile 	  (void);//从文件名表里删除最后一个文件名
char *lua_filename        (void);//返回当前正在处理的文件名,主要用于报错

void  lua_nextvar         (void);//变量迭代器，在符号表里返回下一个符号和对应的对象

#endif
