/*
** strlib.c
** String library to LUA
*/

char *rcs_strlib="$Id: strlib.c,v 1.2 1994/03/28 15:14:02 celes Exp $";

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "mm.h"


#include "lua.h"

/*
** Return the position of the first caracter of a substring into a string
** LUA interface:
**			n = strfind (string, substring)
*/
static void str_find (void)//返回子字符串在字符串中的第一个字符的位置，Lua接口：n = strfind (string, substring)，其中n表示子字符串在字符串中的位置，如果找不到则返回nil
{
 char *s1, *s2, *f;
 lua_Object o1 = lua_getparam (1);
 lua_Object o2 = lua_getparam (2);
 if (!lua_isstring(o1) || !lua_isstring(o2))
 { lua_error ("incorrect arguments to function `strfind'"); return; }
 s1 = lua_getstring(o1);
 s2 = lua_getstring(o2);
 f = strstr(s1,s2);//使用strstr函数在字符串s1中查找子字符串s2，如果找到则返回指向子字符串在字符串中的位置的指针，否则返回NULL
 if (f != NULL)
  lua_pushnumber (f-s1+1);//如果找到子字符串，则将其位置（以1为起始索引）推入Lua栈顶，否则推入nil
 else
  lua_pushnil();
}

/*
** Return the string length
** LUA interface:
**			n = strlen (string)
*/
static void str_len (void)//返回字符串的长度，Lua接口：n = strlen (string)，其中n表示字符串的长度，如果参数不是字符串则抛出错误
{
 lua_Object o = lua_getparam (1);
 if (!lua_isstring(o))
 { lua_error ("incorrect arguments to function `strlen'"); return; }
 lua_pushnumber(strlen(lua_getstring(o)));
}


/*
** Return the substring of a string, from start to end
** LUA interface:
**			substring = strsub (string, start, end)
*/
static void str_sub (void)//返回字符串的子串，从start位置到end位置，Lua接口：substring = strsub (string, start, end)，其中substring表示子串，如果参数不合法则抛出错误
{
 int start, end;
 char *s;
 lua_Object o1 = lua_getparam (1);
 lua_Object o2 = lua_getparam (2);
 lua_Object o3 = lua_getparam (3);
 if (!lua_isstring(o1) || !lua_isnumber(o2))
 { lua_error ("incorrect arguments to function `strsub'"); return; }
 if (o3 != NULL && !lua_isnumber(o3))
 { lua_error ("incorrect third argument to function `strsub'"); return; }
 s = lua_copystring(o1);
 start = lua_getnumber (o2);
 end = o3 == NULL ? strlen(s) : lua_getnumber (o3);
 if (end < start || start < 1 || end > strlen(s))
  lua_pushstring("");
 else
 {
  s[end] = 0;
  lua_pushstring (&s[start-1]);
 }
 free (s);
}

/*
** Convert a string to lower case.
** LUA interface:
**			lowercase = strlower (string)
*/
static void str_lower (void)//将字符串转换为小写，Lua接口：lowercase = strlower (string)，其中lowercase表示转换后的字符串，如果参数不是字符串则抛出错误
{
 char *s, *c;
 lua_Object o = lua_getparam (1);
 if (!lua_isstring(o))
 { lua_error ("incorrect arguments to function `strlower'"); return; }
 c = s = strdup(lua_getstring(o));
 while (*c != 0)
 {
  *c = tolower(*c);
  c++;
 }
 lua_pushstring(s);
 free(s);
} 


/*
** Convert a string to upper case.
** LUA interface:
**			uppercase = strupper (string)
*/
static void str_upper (void)//将字符串转换为大写，Lua接口：uppercase = strupper (string)，其中uppercase表示转换后的字符串，如果参数不是字符串则抛出错误
{
 char *s, *c;
 lua_Object o = lua_getparam (1);
 if (!lua_isstring(o))
 { lua_error ("incorrect arguments to function `strlower'"); return; }
 c = s = strdup(lua_getstring(o));
 while (*c != 0)
 {
  *c = toupper(*c);
  c++;
 }
 lua_pushstring(s);
 free(s);
} 


/*
** Open string library
*/
void strlib_open (void)//字符串库的初始化函数，提供字符串操作功能，如字符串连接、子串提取等
{
 lua_register ("strfind", str_find);
 lua_register ("strlen", str_len);
 lua_register ("strsub", str_sub);
 lua_register ("strlower", str_lower);
 lua_register ("strupper", str_upper);
}
