/*
** lua.c
** Linguagem para Usuarios de Aplicacao
*/

char *rcs_lua="$Id: lua.c,v 1.1 1993/12/17 18:41:19 celes Exp $";

#include <stdio.h>

#include "lua.h"
#include "lualib.h"

int main (int argc, char *argv[])//程序入口函数，接受命令行参数argc和argv，初始化Lua标准库，并根据命令行参数执行相应的Lua代码，如果没有参数则进入交互模式，读取用户输入的Lua代码并执行
{
 int i;
 iolib_open ();//初始化输入输出库，提供文件操作和标准输入输出功能
 strlib_open ();//初始化字符串库，提供字符串操作功能，如字符串连接、子串提取等
 mathlib_open ();//初始化数学函数库，提供基本的数学运算功能，如三角函数、指数函数等
 if (argc < 2)//如果没有命令行参数，进入交互模式，读取用户输入的Lua代码并执行
 {
   char buffer[2048];
   while (fgets(buffer,sizeof(buffer),stdin) != NULL)
     lua_dostring(buffer);
 }
 else//如果有命令行参数，依次执行每个参数指定的Lua文件
   for (i=1; i<argc; i++)
    lua_dofile (argv[i]);
  return 0;
}
