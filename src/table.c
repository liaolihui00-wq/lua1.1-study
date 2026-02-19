/*
** table.c
** Module to control static tables
*/

char *rcs_table="$Id: table.c,v 2.1 1994/04/20 22:07:57 celes Exp $";

#include <stdlib.h>
#include <string.h>

#include "mm.h"

#include "opcode.h"
#include "hash.h"
#include "inout.h"
#include "table.h"
#include "lua.h"

#define streq(s1,s2)	(s1[0]==s2[0]&&strcmp(s1+1,s2+1)==0)//字符串比较宏，比较两个字符串s1和s2是否相等，首先比较它们的第一个字符，如果第一个字符相同，则使用strcmp函数比较它们的剩余部分（从第二个字符开始），如果strcmp返回0表示剩余部分也相同，则整个字符串相等，返回true，否则返回false

#ifndef MAXSYMBOL
#define MAXSYMBOL	512//符号表的最大容量，表示符号表中最多可以存储512个符号
#endif
static Symbol  		tablebuffer[MAXSYMBOL] = {
                                    {"type",{T_CFUNCTION,{lua_type}}},
                                    {"tonumber",{T_CFUNCTION,{lua_obj2number}}},
                                    {"next",{T_CFUNCTION,{lua_next}}},
                                    {"nextvar",{T_CFUNCTION,{lua_nextvar}}},
                                    {"print",{T_CFUNCTION,{lua_print}}},
                                    {"dofile",{T_CFUNCTION,{lua_internaldofile}}},
                                    {"dostring",{T_CFUNCTION,{lua_internaldostring}}}
                                                 };//符号表的静态数组，包含了预定义的符号和它们对应的函数指针，这些符号包括"type"、"tonumber"、"next"、"nextvar"、"print"、"dofile"和"dostring"，每个符号都关联一个C函数，这些函数实现了Lua中的基本功能，如类型检查、字符串转换、迭代器等
Symbol	       	       *lua_table=tablebuffer;//符号表的指针，指向符号表的静态数组tablebuffer，lua_table用于在Lua虚拟机中访问和操作符号表中的符号信息
Word   	 		lua_ntable=7;//符号表中当前已使用的符号数量，初始值为7，因为tablebuffer中已经预定义了7个符号，lua_ntable用于跟踪符号表中已使用的符号数量，以便在添加新符号时进行检查和管理

struct List
{
 Symbol *s;
 struct List *next;
};//符号列表结构体，用于在符号表中维护一个链表，s是指向符号表中某个符号的指针，next是指向下一个符号列表节点的指针，这个链表用于实现符号表的快速查找和管理

static struct List o6={ tablebuffer+6, 0};
static struct List o5={ tablebuffer+5, &o6 };
static struct List o4={ tablebuffer+4, &o5 };
static struct List o3={ tablebuffer+3, &o4 };
static struct List o2={ tablebuffer+2, &o3 };
static struct List o1={ tablebuffer+1, &o2 };
static struct List o0={ tablebuffer+0, &o1 };
static struct List *searchlist=&o0;//符号列表的静态链表，这些节点按照预定义符号的顺序链接在一起，searchlist指向链表的头部，用于在符号表中进行快速查找和管理

#ifndef MAXCONSTANT
#define MAXCONSTANT	256//常量表的最大容量，表示常量表中最多可以存储256个常量
#endif
/* pre-defined constants need garbage collection extra byte */ 
static char tm[] = " mark";
static char ti[] = " nil";
static char tn[] = " number";
static char ts[] = " string";
static char tt[] = " table";
static char tf[] = " function";
static char tc[] = " cfunction";
static char tu[] = " userdata";
static char  	       *constantbuffer[MAXCONSTANT] = {tm+1, ti+1,
						       tn+1, ts+1,
						       tt+1, tf+1,
						       tc+1, tu+1
                                                      };//常量表的静态数组，包含了预定义的常量字符串，这些常量字符串表示Lua中的基本类型，每个常量字符串前面都有一个额外的字节用于标记，这些常量字符串在Lua虚拟机中用于表示对象的类型信息
char  	      	      **lua_constant = constantbuffer;
Word    		lua_nconstant=T_USERDATA+1;//常量表中当前已使用的常量数量，初始值为T_USERDATA+1=8，lua_nconstant用于跟踪常量表中已使用的常量数量

#ifndef MAXSTRING
#define MAXSTRING	512//字符串表的最大容量，表示字符串表中最多可以存储512个字符串
#endif
static char 	       *stringbuffer[MAXSTRING];
char  		      **lua_string = stringbuffer;
Word    		lua_nstring=0;

#define MAXFILE 	20//文件表的最大容量，表示文件表中最多可以存储20个文件
char  		       *lua_file[MAXFILE];
int      		lua_nfile;


#define markstring(s)   (*((s)-1))//标记字符串的宏，给定一个字符串s，markstring(s)访问s前面的一个字节，这个字节用于标记字符串是否被使用或需要垃圾收集，在Lua虚拟机中，当一个字符串被创建或使用时，会设置这个标记位，以便在垃圾收集过程中识别和处理未使用的字符串


/* Variables to controll garbage collection */
Word lua_block=10; /* to check when garbage collector will be called，lua_block是一个变量 ，用于控制垃圾收集的触发时机，当lua_nentity（新实体的数量）达到lua_block时，垃圾收集器将被调用来清理未使用的字符串和表 */
Word lua_nentity;   /* counter of new entities (strings and arrays) ，新实体的数量*/


/*
** Given a name, search it at symbol table and return its index. If not
** found, allocate at end of table, checking oveflow and return its index.
** On error, return -1.
*/
int lua_findsymbol (char *s)//根据给定的名字s，在符号表中搜索对应的符号，如果找到则返回符号的索引，如果没有找到则在符号表的末尾分配一个新的符号，检查是否溢出，并返回新符号的索引，如果发生错误则返回-1
{
 struct List *l, *p;
 for (p=NULL, l=searchlist; l!=NULL; p=l, l=l->next)
  if (streq(s,l->s->name))
  {
   if (p!=NULL)
   {
    p->next = l->next;
    l->next = searchlist;
    searchlist = l;//将找到的符号移动到链表的头部，以提高后续搜索的效率
   }
   return (l->s-lua_table);
  }

 if (lua_ntable >= MAXSYMBOL-1)
 {
  lua_error ("symbol table overflow");
  return -1;
 }
 s_name(lua_ntable) = strdup(s);
 if (s_name(lua_ntable) == NULL)
 {
  lua_error ("not enough memory");
  return -1;
 }
 s_tag(lua_ntable) = T_NIL;
 p = malloc(sizeof(*p)); 
 p->s = lua_table+lua_ntable;//将新分配的符号添加到链表中，p->s指向新符号在符号表中的位置
 p->next = searchlist;
 searchlist = p;//将新符号插入到链表的头部，以便快速访问

 return lua_ntable++;
}

/*
** Given a constant string, search it at constant table and return its index.
** If not found, allocate at end of the table, checking oveflow and return 
** its index.
**
** For each allocation, the function allocate a extra char to be used to
** mark used string (it's necessary to deal with constant and string 
** uniformily). The function store at the table the second position allocated,
** that represents the beginning of the real string. On error, return -1.
** 
*/
int lua_findconstant (char *s)//根据给定的常量字符串s，在常量表中搜索对应的常量，如果找到则返回常量的索引，如果没有找到则在常量表的末尾分配一个新的常量，检查是否溢出，并返回新常量的索引，如果发生错误则返回-1
{
 int i;
 for (i=0; i<lua_nconstant; i++)
  if (streq(s,lua_constant[i]))
   return i;
 if (lua_nconstant >= MAXCONSTANT-1)
 {
  lua_error ("lua: constant string table overflow"); 
  return -1;
 }
 {
  char *c = calloc(strlen(s)+2,sizeof(char));//为新常量分配内存，分配的大小是字符串s的长度加上2个字节，其中一个字节用于标记，另一个字节用于存储字符串的结束符'\0'
  c++;		/* create mark space */
  lua_constant[lua_nconstant++] = strcpy(c,s);//将新常量复制到常量表中，并返回新常量的索引
 }
 return (lua_nconstant-1);
}


/*
** Traverse symbol table objects
*/
void lua_travsymbol (void (*fn)(Object *))//遍历符号表对象，对符号表中的每个对象调用fn函数，通常用于垃圾回收等需要访问符号表中对象的操作
{
 int i;
 for (i=0; i<lua_ntable; i++)
  fn(&s_object(i));
}


/*
** Mark an object if it is a string or a unmarked array.
*/
void lua_markobject (Object *o)//标记一个对象，如果它是一个字符串或者一个未标记的数组
{
 if (tag(o) == T_STRING)
  markstring (svalue(o)) = 1;//如果对象是一个字符串，则将该字符串的标记位设置为1，表示该字符串正在使用中
 else if (tag(o) == T_ARRAY)
   lua_hashmark (avalue(o));
}


/*
** Garbage collection. 
** Delete all unused strings and arrays.
*/
void lua_pack (void)//垃圾收集函数，删除所有未使用的字符串和数组
{
 /* mark stack strings */
 lua_travstack(lua_markobject);//遍历运行栈上的对象，对每个对象调用lua_markobject函数进行标记，如果对象是一个字符串或者一个未标记的数组，则将其标记为正在使用中
 
 /* mark symbol table strings */
 lua_travsymbol(lua_markobject);//遍历符号表中的对象，对每个对象调用lua_markobject函数进行标记，如果对象是一个字符串或者一个未标记的数组，则将其标记为正在使用中

 lua_stringcollector();//垃圾收集字符串，删除所有未被标记为正在使用的字符串
 lua_hashcollector();//垃圾收集数组，删除所有未被标记为正在使用的数组

 lua_nentity = 0;      /* reset counter */
} 

/*
** Garbage collection to atrings.
** Delete all unmarked strings
*/
void lua_stringcollector (void)//垃圾收集字符串，删除所有未被标记为正在使用的字符串
{
 int i, j;
 for (i=j=0; i<lua_nstring; i++)
  if (markstring(lua_string[i]) == 1)
  {
   lua_string[j++] = lua_string[i];//如果字符串被标记为正在使用中，则将其保留在字符串表中，并将其移动到字符串表的前面，以便在后续的垃圾收集过程中更容易访问和管理
   markstring(lua_string[i]) = 0;//将字符串的标记位重置为0，表示该字符串已经被处理过了，下次垃圾收集时如果没有被重新标记为1，则会被删除
  }
  else
  {
   free (lua_string[i]-1);
  }
 lua_nstring = j;
}

/*
** Allocate a new string at string table. The given string is already 
** allocated with mark space and the function puts it at the end of the
** table, checking overflow, and returns its own pointer, or NULL on error.
*/
char *lua_createstring (char *s)//在字符串表中分配一个新的字符串，给定的字符串s已经分配了标记空间，函数将其放在字符串表的末尾，检查是否溢出，并返回新字符串的指针，如果发生错误则返回NULL
{
 int i;
 if (s == NULL) return NULL;
 
 for (i=0; i<lua_nstring; i++)
  if (streq(s,lua_string[i]))//如果字符串s已经存在于字符串表中，则返回该字符串的指针，避免重复存储相同的字符串
  {
   free(s-1);
   return lua_string[i];
  }

 if (lua_nentity == lua_block || lua_nstring >= MAXSTRING-1)
 {
  lua_pack ();//如果新实体的数量达到垃圾收集的触发时机，或者字符串表已经满了，则调用lua_pack函数进行垃圾收集，清理未使用的字符串和数组，以释放内存空间
  if (lua_nstring >= MAXSTRING-1)
  {
   lua_error ("string table overflow");
   return NULL;
  }
 } 
 lua_string[lua_nstring++] = s;
 lua_nentity++;
 return s;
}

/*
** Add a file name at file table, checking overflow. This function also set
** the external variable "lua_filename" with the function filename set.
** Return 0 on success or 1 on error.
*/
int lua_addfile (char *fn)//在文件表中添加一个文件名，检查是否溢出，这个函数还会设置外部变量"lua_filename"为当前设置的文件名，成功返回0，失败返回1
{
 if (lua_nfile >= MAXFILE-1)
 {
  lua_error ("too many files");
  return 1;
 }
 if ((lua_file[lua_nfile++] = strdup (fn)) == NULL)//将文件名fn复制到文件表中，如果复制失败则返回错误，成功则将文件名添加到文件表中，并更新lua_nfile的数量
 {
  lua_error ("not enough memory");
  return 1;
 }
 return 0;
}

/*
** Delete a file from file stack
*/
int lua_delfile (void)//从文件栈中删除一个文件，如果文件栈已经空了，则报错或者直接返回错误码，而不是盲目递减lua_nfile，成功返回1，失败返回0
{
  if (lua_nfile <= 0) 
  {
    lua_error("file stack underflow");
    return 0; 
  }
  lua_nfile--; 
  return 1;
}

/*
** Return the last file name set.
*/
char *lua_filename (void)//返回当前正在处理的文件名，主要用于报错，如果文件栈为空，则返回NULL或者一个默认的文件名，而不是直接访问lua_file[lua_nfile-1]，以避免访问越界
{
 if (lua_nfile <= 0) return NULL;//如果文件栈为空，则返回NULL，表示没有当前文件名可用
 return lua_file[lua_nfile-1];
}

/*
** Internal function: return next global variable
*/
void lua_nextvar (void)//内部函数：返回下一个全局变量，作为迭代器使用，根据当前符号表中的符号索引返回下一个全局变量的名称和值，如果没有更多的全局变量，则返回nil
{
 int index;
 Object *o = lua_getparam (1);//
 if (o == NULL)
 { lua_error ("too few arguments to function `nextvar'"); return; }
 if (lua_getparam (2) != NULL)
 { lua_error ("too many arguments to function `nextvar'"); return; }
 if (tag(o) == T_NIL)
 {
  index = 0;
 }
 else if (tag(o) != T_STRING) 
 { 
  lua_error ("incorrect argument to function `nextvar'"); 
  return;
 }
 else
 {
  for (index=0; index<lua_ntable; index++)
   if (streq(s_name(index),svalue(o))) break;
  if (index == lua_ntable) 
  {
   lua_error ("name not found in function `nextvar'");
   return;
  }
  index++;
  while (index < lua_ntable && tag(&s_object(index)) == T_NIL) index++;
  
  if (index == lua_ntable)
  {
   lua_pushnil();
   lua_pushnil();//如果没有更多的全局变量，则返回nil
   return;
  }
 }
 {
  Object name;
  tag(&name) = T_STRING;
  svalue(&name) = lua_createstring(lua_strdup(s_name(index)));
  if (lua_pushobject (&name)) return;//将下一个全局变量的名称推入栈顶，如果发生错误则返回
  if (lua_pushobject (&s_object(index))) return;//将下一个全局变量的值推入栈顶，如果发生错误则返回
 }
}
