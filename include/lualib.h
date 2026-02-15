/*
** Libraries to be used in LUA programs
** Grupo de Tecnologia em Computacao Grafica
** TeCGraf - PUC-Rio
** $Id: lualib.h,v 1.1 1993/12/17 19:01:46 celes Exp $
*/


//定义了Lua标准库的接口函数，这些函数提供了Lua程序常用的功能，如输入输出、字符串处理和数学运算等
#ifndef lualib_h
#define lualib_h

void iolib_open   (void);//输入输出库的初始化函数
void strlib_open  (void);//字符串库的初始化函数，提供字符串操作功能，如字符串连接、子串提取等
void mathlib_open (void);//数学函数库的初始化函数，提供基本的数学运算功能，如三角函数、指数函数等

#endif

