/*
** inout.c
** Provide function to realise the input/output function and debugger 
** facilities.
*/

char *rcs_inout="$Id: inout.c,v 1.2 1993/12/22 21:15:16 roberto Exp $";

#include <stdio.h>
#include <string.h>

#include "opcode.h"
#include "hash.h"
#include "inout.h"
#include "table.h"

/* Exported variables */
int lua_linenumber;//当前正在处理的行号
int lua_debug;//调试模式标志，表示是否启用调试功能，在解析过程中会根据这个标志来决定是否生成调试信息或者进行额外的错误检查
int lua_debugline;//调试行号，表示当前正在处理的行号

/* Internal variables */
#ifndef MAXFUNCSTACK
#define MAXFUNCSTACK 32//函数栈的最大容量，表示函数调用栈中最多可以存储32个函数调用信息
#endif
static struct { int file; int function; } funcstack[MAXFUNCSTACK];//函数栈的静态数组，用于存储函数调用信息，每个元素包含一个文件索引和一个函数索引，这些索引用于在调试信息中报告当前正在执行的函数和所在的文件
static int nfuncstack=0;//函数栈的当前数量，表示函数调用栈中当前已经存储了多少个函数调用信息

static FILE *fp;//输入文件指针，表示当前正在读取的输入文件，如果是从字符串输入，则这个指针为NULL
static char *st;//输入字符串指针，表示当前正在读取的输入字符串，如果是从文件输入，则这个指针为NULL
static void (*usererror) (char *s);//用户错误处理函数指针，表示一个函数，用于处理错误消息，如果用户注册了一个错误处理函数，那么在发生错误时会调用这个函数来处理错误消息，否则会使用默认的错误处理方式（如打印到标准错误输出）

/*
** Function to set user function to handle errors.
*/
void lua_errorfunction (void (*fn) (char *s))//设置用户错误处理函数，fn是一个函数指针，用于处理错误消息，这样用户可以自定义错误处理逻辑
{
 usererror = fn;
}

/*
** Function to get the next character from the input file
*/
static int fileinput (void)//从输入文件中获取下一个字符，如果已经到达文件末尾，则返回0，表示输入结束，否则返回下一个字符的ASCII码
{
 int c = fgetc (fp);
 return (c == EOF ? 0 : c);
}

/*
** Function to get the next character from the input string
*/
static int stringinput (void)//从输入字符串中获取下一个字符，如果已经到达字符串末尾，则返回0，表示输入结束，否则返回下一个字符的ASCII码
{
 st++;
 return (*(st-1));
}

/*
** Function to open a file to be input unit. 
** Return 0 on success or 1 on error.
*/
int lua_openfile (char *fn)//打开一个文件作为输入单元，fn是文件名，成功返回0，失败返回1，并使用fileinput函数作为输入函数
{
 lua_linenumber = 1;
 lua_setinput (fileinput);//设置输入函数为fileinput，这样在词法分析过程中就会从这个文件中读取字符
 fp = fopen (fn, "r");//尝试打开文件，如果打开失败则返回错误，成功则将文件指针存储在fp变量中
 if (fp == NULL) return 1;
 if (lua_addfile (fn)) return 1;//将文件名添加到文件表中，如果添加失败则返回错误，成功则继续进行输入设置
 return 0;
}

/*
** Function to close an opened file
*/
void lua_closefile (void)//关闭一个已经打开的文件，如果文件指针fp不为NULL，则关闭这个文件，并将fp设置为NULL，表示没有当前打开的文件
{
 if (fp != NULL)
 {
  lua_delfile();//从文件名表中删除当前文件名，更新文件栈的状态
  fclose (fp);//关闭文件，释放系统资源
  fp = NULL;
 }
}

/*
** Function to open a string to be input unit
*/
int lua_openstring (char *s)//打开一个字符串作为输入单元，s是输入字符串，成功返回0，失败返回1，并使用stringinput函数作为输入函数
{
 lua_linenumber = 1;
 lua_setinput (stringinput);//设置输入函数为stringinput
 st = s;
 {
  char sn[64];
  sprintf (sn, "String: %10.10s...", s);//构造一个表示输入字符串的文件名，格式为"String: xxxxx..."，其中xxxxx是输入字符串的前10个字符，这样在调试信息或者错误报告中就可以显示这个输入字符串的来源
  if (lua_addfile (sn)) return 1;//将这个表示输入字符串的文件名添加到文件表中，如果添加失败则返回错误，成功则继续进行输入设置
 }
 return 0;
}

/*
** Function to close an opened string
*/
void lua_closestring (void)//关闭打开的字符串，但是不需要真正关闭字符串，因为字符串是存储在内存中的，只需要更新文件栈的状态即可
{
 lua_delfile();//从文件名表中删除当前输入字符串的表示文件名，更新文件栈的状态
}

/*
** Call user function to handle error messages, if registred. Or report error
** using standard function (fprintf).
*/
void lua_error (char *s)//调用用户函数来处理错误消息，如果已经注册了一个错误处理函数，那么在发生错误时会调用这个函数来处理错误消息，否则会使用默认的错误处理方式（如打印到标准错误输出）
{
 if (usererror != NULL) usererror (s);
 else	fprintf (stderr, "lua: %s\n", s);//如果没有注册用户错误处理函数，则将错误消息打印到标准错误输出，格式为"lua: 错误消息"
}

/*
** Called to execute  SETFUNCTION opcode, this function pushs a function into
** function stack. Return 0 on success or 1 on error.
*/
int lua_pushfunction (int file, int function)//执行SETFUNCTION opcode时调用，这个函数将一个函数信息推入函数栈中，file是函数所在的文件索引，function是函数的索引，成功返回0，失败返回1
{
 if (nfuncstack >= MAXFUNCSTACK-1)//如果函数栈已经满了，则返回错误，表示函数调用栈溢出
 {
  lua_error ("function stack overflow");
  return 1;
 }
 funcstack[nfuncstack].file = file;
 funcstack[nfuncstack].function = function;
 nfuncstack++;
 return 0;
}

/*
** Called to execute  RESET opcode, this function pops a function from 
** function stack.
*/
void lua_popfunction (void)//执行RESET opcode时调用，表示当前函数调用结束，更新函数栈的状态
{
 nfuncstack--;
}

/*
** Report bug building a message and sending it to lua_error function.
*/
void lua_reportbug (char *s)//报告一个bug，构建一个错误消息并发送给lua_error函数，这个函数主要用于在Lua虚拟机内部发生不可恢复的错误时调用，构建一个包含错误信息和当前调试信息的错误消息，并调用lua_error函数来处理这个错误消息
{
 char msg[1024];
 strcpy (msg, s);
 if (lua_debugline != 0)
 {
  int i;
  if (nfuncstack > 0)
  {
   sprintf (strchr(msg,0), 
         "\n\tin statement begining at line %d in function \"%s\" of file \"%s\"",
         lua_debugline, s_name(funcstack[nfuncstack-1].function),
  	 lua_file[funcstack[nfuncstack-1].file]);//错误消息中添加当前正在执行的函数和所在的文件以及行号，这样可以帮助定位错误发生的位置
   sprintf (strchr(msg,0), "\n\tactive stack\n");
   for (i=nfuncstack-1; i>=0; i--)//错误消息中添加函数调用栈的状态，列出每个函数调用的信息
    sprintf (strchr(msg,0), "\t-> function \"%s\" of file \"%s\"\n", 
                            s_name(funcstack[i].function),
			    lua_file[funcstack[i].file]);//错误消息中添加当前函数调用栈的状态，列出每个函数调用的信息
  }
  else
  {
   sprintf (strchr(msg,0),
         "\n\tin statement begining at line %d of file \"%s\"", 
         lua_debugline, lua_filename());//如果函数栈为空，则错误消息中只添加当前正在处理的文件和行号
  }
 }
 lua_error (msg);
}

