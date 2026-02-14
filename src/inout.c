/*
** $Id: inout.h,v 1.1 1993/12/17 18:41:19 celes Exp $
*/


#ifndef inout_h
#define inout_h

extern int lua_linenumber;//行号计数器，在解析lua代码时，记录当前正在解析的行号，用于错误报告和调试信息中显示正确的行号
extern int lua_debug;//调试标志，控制是否输出调试信息，当lua_debug为非零值时，lua会输出调试信息，如函数调用、变量值等，帮助开发者调试代码
extern int lua_debugline;//调试行号，记录当前正在执行的lua代码的行号，用于调试信息中显示当前执行的行号

int  lua_openfile     (char *fn);//打开一个lua文件，fn是文件名，返回一个文件标识符，用于后续操作
void lua_closefile    (void);//关闭当前打开的lua文件，释放相关资源
int  lua_openstring   (char *s);//允许Lua直接解析C语言中的字符串s，返回一个字符串标识符
void lua_closestring  (void);//关闭当前打开的字符串
int  lua_pushfunction (int file, int function);//将一个函数推入Lua栈顶，file是函数所在的文件标识符，function是函数在文件中的位置，返回一个函数标识符
void lua_popfunction  (void);//处理完该函数，回到上一层
void lua_reportbug    (char *s);//格式化输出错误信息s

#endif
