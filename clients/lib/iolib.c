/*
** iolib.c
** Input/output library to LUA
*/

char *rcs_iolib="$Id: iolib.c,v 1.4 1994/04/25 20:11:23 celes Exp $";

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/stat.h>
// #ifdef __GNUC__
// #include <floatingpoint.h>//在GNU C编译器中，可能需要包含这个头文件来处理浮点数相关的功能，但在其他编译器中可能不需要，因此被注释掉了
// #endif

#include "mm.h"

#include "lua.h"


// static FILE *in=stdin, *out=stdout;//在C语言中，不能直接初始化一个指向FILE类型的指针为一个文件流常量（如stdin和stdout）

static FILE *in, *out;
static FILE *in = NULL;
static FILE *out = NULL;

//在 iolib_open 里赋值
void iolib_init(void) {
    if (in == NULL) in = stdin;
    if (out == NULL) out = stdout;
}

/*
** Open a file to read.
** LUA interface:
**			status = readfrom (filename)
** where:
**			status = 1 -> success
**			status = 0 -> error
*/
static void io_readfrom (void)//打开一个文件进行读取，Lua接口：status = readfrom (filename)，其中status表示操作的结果，1表示成功，0表示错误，如果参数不是字符串则抛出错误
{
 lua_Object o = lua_getparam (1);
 if (o == NULL)			/* restore standart input */
 {
  if (in != stdin)
  {
   fclose (in);
   in = stdin;
  }
  lua_pushnumber (1);
 }
 else
 {
  if (!lua_isstring (o))
  {
   lua_error ("incorrect argument to function 'readfrom`");
   lua_pushnumber (0);
  }
  else
  {
   FILE *fp = fopen (lua_getstring(o),"r");
   if (fp == NULL)
   {
    lua_pushnumber (0);
   }
   else
   {
    if (in != stdin) fclose (in);
    in = fp;//将输入流设置为打开的文件，如果之前的输入流不是标准输入，则先关闭它
    lua_pushnumber (1);
   }
  }
 }
}


/*
** Open a file to write.
** LUA interface:
**			status = writeto (filename)
** where:
**			status = 1 -> success
**			status = 0 -> error
*/
static void io_writeto (void)//打开一个文件进行写入，Lua接口：status = writeto (filename)，其中status表示操作的结果，1表示成功，0表示错误
{
 lua_Object o = lua_getparam (1);
 if (o == NULL)			/* restore standart output */
 {
  if (out != stdout)
  {
   fclose (out);
   out = stdout;
  }
  lua_pushnumber (1);
 }
 else
 {
  if (!lua_isstring (o))
  {
   lua_error ("incorrect argument to function 'writeto`");
   lua_pushnumber (0);
  }
  else
  {
   FILE *fp = fopen (lua_getstring(o),"w");
   if (fp == NULL)
   {
    lua_pushnumber (0);
   }
   else
   {
    if (out != stdout) fclose (out);
    out = fp;//将输出流设置为打开的文件，如果之前的输出流不是标准输出，则先关闭它
    lua_pushnumber (1);
   }
  }
 }
}


/*
** Open a file to write appended.
** LUA interface:
**			status = appendto (filename)
** where:
**			status = 2 -> success (already exist)
**			status = 1 -> success (new file)
**			status = 0 -> error
*/
static void io_appendto (void)//打开一个文件进行追加写入，Lua接口：status = appendto (filename)，其中status表示操作的结果，2表示成功（文件已存在），1表示成功（新文件），0表示错误
{
 lua_Object o = lua_getparam (1);
 if (o == NULL)			/* restore standart output */
 {
  if (out != stdout)
  {
   fclose (out);
   out = stdout;
  }
  lua_pushnumber (1);
 }
 else
 {
  if (!lua_isstring (o))
  {
   lua_error ("incorrect argument to function 'appendto`");
   lua_pushnumber (0);
  }
  else
  {
   int r;
   FILE *fp;
   struct stat st;//在C语言中，struct stat是一个结构体类型，用于存储文件的属性信息，如大小、权限、修改时间等，stat函数可以用来获取文件的属性信息并存储在这个结构体中
   if (stat(lua_getstring(o), &st) == -1) r = 1;//使用stat函数检查文件是否存在，如果返回-1表示文件不存在，则设置r为1，表示成功（新文件），否则设置r为2，表示成功（文件已存在）
   else                                   r = 2;
   fp = fopen (lua_getstring(o),"a");
   if (fp == NULL)
   {
    lua_pushnumber (0);
   }
   else
   {
    if (out != stdout) fclose (out);
    out = fp;//将输出流设置为打开的文件，如果之前的输出流不是标准输出，则先关闭它
    lua_pushnumber (r);
   }
  }
 }
}



/*
** Read a variable. On error put nil on stack.
** LUA interface:
**			variable = read ([format])
**
** O formato pode ter um dos seguintes especificadores:
**
**	s ou S -> para string
**	f ou F, g ou G, e ou E -> para reais
**	i ou I -> para inteiros
**
**	Estes especificadores podem vir seguidos de numero que representa
**	o numero de campos a serem lidos.
*/
static void io_read (void)//从输入流中读取一个变量，Lua接口：variable = read ([format])，其中variable表示读取到的变量，如果发生错误则返回nil，format是一个可选的格式字符串，用于指定读取的格式
{
 lua_Object o = lua_getparam (1);
 if (o == NULL || !lua_isstring(o))	/* free format *///如果没有提供格式参数或者格式参数不是字符串，则使用自由格式读取，即跳过空白字符后直接读取一个字符串或数字
 {
  int c;
  char s[256];
  while (isspace(c=fgetc(in)));//跳过输入流中的空白字符，直到遇到第一个非空白字符为止，如果输入流已经结束，则返回nil表示错误
  if (c == '\"')//如果第一个非空白字符是双引号，说明要读取一个字符串，继续读取直到下一个双引号为止，如果在此过程中遇到输入流结束，则返回nil表示错误
  {
   int c, n=0;
   while((c = fgetc(in)) != '\"')
   {
    if (c == EOF)//如果在读取字符串的过程中遇到输入流结束，则返回nil表示错误
    {
     lua_pushnil ();
     return;
    }
    s[n++] = c;
   }
   s[n] = 0;
  }
  else if (c == '\'')//如果第一个非空白字符是单引号，说明要读取一个字符串，继续读取直到下一个单引号为止
  {
   int c, n=0;
   while((c = fgetc(in)) != '\'')
   {
    if (c == EOF)
    {
     lua_pushnil ();
     return;
    }
    s[n++] = c;
   }
   s[n] = 0;
  }
  else//如果第一个非空白字符既不是双引号也不是单引号，说明要读取一个数字或一个单词，继续读取直到下一个空白字符为止
  {
   char *ptr;
   double d;
   ungetc (c, in);//将第一个非空白字符放回输入流，以便后续的读取函数能够正确地读取它，如果输入流已经结束，则返回nil表示错误
   if (fscanf (in, "%s", s) != 1)//如果读取一个单词失败，则返回nil表示错误
   {
    lua_pushnil ();
    return;
   }
   d = strtod (s, &ptr);//尝试将读取到的字符串转换为一个数字，如果转换成功且字符串完全被转换了，则返回这个数字；否则，返回这个字符串作为一个字符串对象
   if (!(*ptr))//如果ptr指向的字符是字符串的结尾，说明字符串完全成功地转换为数字，表示转换成功
   {
    lua_pushnumber (d);
    return;
   }
  }
  lua_pushstring (s);//将读取到的字符串推入Lua栈顶，表示读取成功
  return;
 }
 else				/* formatted *///如果提供了格式参数且格式参数是一个字符串，则使用格式化读取，根据格式字符串中的指定格式来读取输入流中的数据
 {
  char *e = lua_getstring(o);//获取格式字符串，解析格式字符串中的格式说明符，并根据这些说明符来读取输入流中的数据
  char t;
  int  m=0;
  while (isspace(*e)) e++;//跳过格式字符串中的空白字符，直到遇到第一个非空白字符为止，如果格式字符串已经结束，则返回nil表示错误
  t = *e++;
  while (isdigit(*e))//如果格式字符串中有数字，说明指定了要读取的字段数，如果格式字符串已经结束或者没有数字，则m默认为0，表示不限制字段数
   m = m*10 + (*e++ - '0');//将格式字符串中的数字部分转换为一个整数，表示要读取的字段数
  
  if (m > 0)
  {
   char f[80];
   char s[256];
   sprintf (f, "%%%ds", m);//根据指定的字段数构造一个格式字符串，用于读取一个字符串
   if (fgets (s, m, in) == NULL)
   {
    lua_pushnil();
    return;
   }
   else
   {
    if (s[strlen(s)-1] == '\n')
     s[strlen(s)-1] = 0;
   }
   switch (tolower(t))//根据格式字符串中的类型说明符来处理读取到的数据
   {
    case 'i'://如果类型说明符是'i'，说明要读取一个整数，将读取到的字符串转换为一个长整数，并将其推入Lua栈顶作为一个数字对象
    {
     long int l;
     sscanf (s, "%ld", &l);
     lua_pushnumber(l);
    }
    break;
    case 'f': case 'g': case 'e'://如果类型说明符是'f'、'g'或'e'，说明要读取一个实数，将读取到的字符串转换为一个浮点数，并将其推入Lua栈顶作为一个数字对象
    {
     float f;
     sscanf (s, "%f", &f);
     lua_pushnumber(f);
    }
    break;
    default: //如果类型说明符是's'或其他字符，说明要读取一个字符串，将读取到的字符串直接推入Lua栈顶作为一个字符串对象
     lua_pushstring(s); 
    break;
   }
  }
  else
  {
   switch (tolower(t))
   {
    case 'i':
    {
     long int l;
     if (fscanf (in, "%ld", &l) == EOF)
       lua_pushnil();
       else lua_pushnumber(l);
    }
    break;
    case 'f': case 'g': case 'e':
    {
     float f;
     if (fscanf (in, "%f", &f) == EOF)
       lua_pushnil();
       else lua_pushnumber(f);
    }
    break;
    default: 
    {
     char s[256];
     if (fscanf (in, "%s", s) == EOF)
       lua_pushnil();
       else lua_pushstring(s);
    }
    break;
   }
  }
 }
}


/*
** Write a variable. On error put 0 on stack, otherwise put 1.
** LUA interface:
**			status = write (variable [,format])
**
** O formato pode ter um dos seguintes especificadores:
**
**	s ou S -> para string
**	f ou F, g ou G, e ou E -> para reais
**	i ou I -> para inteiros
**
**	Estes especificadores podem vir seguidos de:
**
**		[?][m][.n]
**
**	onde:
**		? -> indica justificacao
**			< = esquerda
**			| = centro
**			> = direita (default)
**		m -> numero maximo de campos (se exceder estoura)
**		n -> indica precisao para
**			reais -> numero de casas decimais
**			inteiros -> numero minimo de digitos
**			string -> nao se aplica
*/
static char *buildformat (char *e, lua_Object o)//根据格式字符串e和Lua对象o构造一个格式化字符串，并返回一个指向格式化结果的指针，格式字符串e可以包含一些格式说明符，用于指定如何格式化Lua对象o的值
{
 static char buffer[512];
 static char f[80];
 char *string = &buffer[255];
 char t, j='r';
 int  m=0, n=0, l;
 while (isspace(*e)) e++;
 t = *e++;
 if (*e == '<' || *e == '|' || *e == '>') j = *e++;
 while (isdigit(*e))
  m = m*10 + (*e++ - '0');
 e++;	/* skip point */
 while (isdigit(*e))
  n = n*10 + (*e++ - '0');

 sprintf(f,"%%");
 if (j == '<' || j == '|') sprintf(strchr(f,0),"-");
 if (m != 0)   sprintf(strchr(f,0),"%d", m);
 if (n != 0)   sprintf(strchr(f,0),".%d", n);
 sprintf(strchr(f,0), "%c", t);
 switch (tolower(t))
 {
  case 'i': t = 'i';
   sprintf (string, f, (long int)lua_getnumber(o));
  break;
  case 'f': case 'g': case 'e': t = 'f';
   sprintf (string, f, (float)lua_getnumber(o));
  break;
  case 's': t = 's';
   sprintf (string, f, lua_getstring(o));
  break;
  default: return "";
 }
 l = strlen(string);
 if (m!=0 && l>m)
 {
  int i;
  for (i=0; i<m; i++)
   string[i] = '*';
  string[i] = 0;
 }
 else if (m!=0 && j=='|')
 {
  int i=l-1;
  while (isspace(string[i])) i--;
  string -= (m-i) / 2;
  i=0;
  while (string[i]==0) string[i++] = ' ';
  string[l] = 0;
 }
 return string;
}
static void io_write (void)//将一个变量写入输出流，Lua接口：status = write (variable [,format])，其中status表示操作的结果，1表示成功，0表示错误，variable是要写入的变量，format是一个可选的格式字符串，用于指定写入的格式
{
 lua_Object o1 = lua_getparam (1);
 lua_Object o2 = lua_getparam (2);
 if (o1 == NULL)			/* new line */
 {
  fprintf (out, "\n");
  lua_pushnumber(1);
 }
 else if (o2 == NULL)   		/* free format */
 {
  int status=0;
  if (lua_isnumber(o1))
   status = fprintf (out, "%g", lua_getnumber(o1));
  else if (lua_isstring(o1))
   status = fprintf (out, "%s", lua_getstring(o1));
  lua_pushnumber(status);
 }
 else					/* formated */
 {
  if (!lua_isstring(o2))
  { 
   lua_error ("incorrect format to function `write'"); 
   lua_pushnumber(0);
   return;
  }
  lua_pushnumber(fprintf (out, "%s", buildformat(lua_getstring(o2),o1)));
 }
}

/*
** Execute a executable program using "system".
** Return the result of execution.
*/
void io_execute (void)//使用system函数执行一个可执行程序，Lua接口：status = execute (command)，其中status表示执行的结果，0表示错误，其他值表示成功，command是要执行的命令字符串
{
 lua_Object o = lua_getparam (1);
 if (o == NULL || !lua_isstring (o))
 {
  lua_error ("incorrect argument to function 'execute`");
  lua_pushnumber (0);
 }
 else
 {
  int res = system(lua_getstring(o));
  lua_pushnumber (res);
 }
 return;
}

/*
** Remove a file.
** On error put 0 on stack, otherwise put 1.
*/
void io_remove  (void)//删除一个文件，Lua接口：status = remove (filename)，其中status表示操作的结果，0表示错误，1表示成功，filename是要删除的文件名字符串
{
 lua_Object o = lua_getparam (1);
 if (o == NULL || !lua_isstring (o))
 {
  lua_error ("incorrect argument to function 'execute`");
  lua_pushnumber (0);
 }
 else
 {
  if (remove(lua_getstring(o)) == 0)
   lua_pushnumber (1);
  else
   lua_pushnumber (0);
 }
 return;
}

/*
** Open io library
*/
void iolib_open (void)
{
  iolib_init(); // 确保输入输出流被正确初始化
 lua_register ("readfrom", io_readfrom);
 lua_register ("writeto",  io_writeto);
 lua_register ("appendto", io_appendto);
 lua_register ("read",     io_read);
 lua_register ("write",    io_write);
 lua_register ("execute",  io_execute);
 lua_register ("remove",   io_remove);
}
