/*
** opcode.c
** TecCGraf - PUC-Rio
*/

char *rcs_opcode="$Id: opcode.c,v 2.1 1994/04/20 22:07:57 celes Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* stdlib.h does not have this in SunOS */
extern double strtod(const char *, char **);//声明strtod函数，用于将字符串转换为双精度浮点数

#include "mm.h"

#include "opcode.h"
#include "hash.h"
#include "inout.h"
#include "table.h"
#include "lua.h"

#define tonumber(o) ((tag(o) != T_NUMBER) && (lua_tonumber(o) != 0))//宏定义tonumber，用于将一个对象o转换为数字类型，0表示成功，非零表示转换失败
#define tostring(o) ((tag(o) != T_STRING) && (lua_tostring(o) != 0))//宏定义tostring，用于将一个对象o转换为字符串类型，0表示成功，非零表示转换失败

#ifndef MAXSTACK
#define MAXSTACK 256//运行栈的最大容量，表示Lua虚拟机中运行栈的大小，最多可以存储256个对象
#endif
static Object stack[MAXSTACK] = {{T_MARK, {NULL}}};//运行栈的静态数组，表示Lua虚拟机中运行栈的实际存储空间，初始值为一个标记对象，表示栈底，Lua虚拟机通过操作这个栈来执行指令和管理函数调用等操作
static Object *top=stack+1, *base=stack+1;//栈顶指针和基址指针，top指向当前栈顶位置，base指向当前函数调用的基址位置，初始值都指向stack数组的第二个元素，因为第一个元素是栈底的标记对象


/*
** Concatenate two given string, creating a mark space at the beginning.
** Return the new string pointer.
*/
static char *lua_strconc (char *l, char *r)//将两个字符串l和r连接起来，并在连接后的字符串前面创建一个标记空间，返回新的字符串指针
{
 char *s = calloc (strlen(l)+strlen(r)+2, sizeof(char));
 if (s == NULL)
 {
  lua_error ("not enough memory");
  return NULL;
 }
 *s++ = 0; 			/* create mark space *///在新字符串的开头创建一个标记空间，标记空间是一个字符，值为0
 return strcat(strcpy(s,l),r);
}

/*
** Duplicate a string,  creating a mark space at the beginning.
** Return the new string pointer.
*/
char *lua_strdup (char *l)//复制一个字符串l，并在复制后的字符串前面创建一个标记空间，返回新的字符串指针
{
 char *s = calloc (strlen(l)+2, sizeof(char));
 if (s == NULL)
 {
  lua_error ("not enough memory");
  return NULL;
 }
 *s++ = 0; 			/* create mark space */
 return strcpy(s,l);
}

/*
** Convert, if possible, to a number tag.
** Return 0 in success or not 0 on error.
*/ 
static int lua_tonumber (Object *obj)//将一个对象obj转换为数字类型，如果对象的类型已经是数字类型，则直接返回0表示成功；如果对象的类型不是数字类型但可以转换为数字类型，则进行转换并返回0表示成功；如果对象的类型无法转换为数字类型，则返回非零值表示错误
{
 char *ptr;
 if (tag(obj) != T_STRING)
 {
  lua_reportbug ("unexpected type at conversion to number");
  return 1;
 }
 nvalue(obj) = strtod(svalue(obj), &ptr);//使用strtod函数将对象obj的字符串值转换为双精度浮点数，并将转换后的数字存储在对象的数值字段中，ptr指向字符串中第一个无法转换为数字的字符
 if (*ptr)//如果ptr指向的字符不是字符串的结尾，说明字符串中存在无法转换为数字的部分，表示转换失败
 {
  lua_reportbug ("string to number convertion failed");
  return 2;
 }
 tag(obj) = T_NUMBER;
 return 0;
}

/*
** Test if is possible to convert an object to a number one.
** If possible, return the converted object, otherwise return nil object.
*/ 
static Object *lua_convtonumber (Object *obj)//测试是否可以将一个对象obj转换为数字类型，如果可以转换则返回转换后的Object对象指针，否则返回一个nil对象指针
{
 static Object cvt;
 
 if (tag(obj) == T_NUMBER)
 {
  cvt = *obj;
  return &cvt;
 }
  
 tag(&cvt) = T_NIL;
 if (tag(obj) == T_STRING)
 {
  char *ptr;
  nvalue(&cvt) = strtod(svalue(obj), &ptr);
  if (*ptr == 0)//如果ptr指向的字符是字符串的结尾，说明字符串完全成功地转换为数字，表示转换成功
   tag(&cvt) = T_NUMBER;
 }
 return &cvt;
}



/*
** Convert, if possible, to a string tag
** Return 0 in success or not 0 on error.
*/ 
static int lua_tostring (Object *obj)//将一个对象obj转换为字符串类型，如果对象的类型已经是字符串类型或者能转换成字符串类型，则直接返回0表示成功；如果对象的类型无法转换为字符串类型，则返回非零值表示错误
{
 static char s[256];
 if (tag(obj) != T_NUMBER)
 {
  lua_reportbug ("unexpected type at conversion to string");
  return 1;
 }
 if ((int) nvalue(obj) == nvalue(obj))//如果对象obj的数值部分是一个整数，则将其格式化为整数字符串；否则，将其格式化为浮点数字符串
  sprintf (s, "%d", (int) nvalue(obj));
 else
  sprintf (s, "%g", nvalue(obj));
 svalue(obj) = lua_createstring(lua_strdup(s));//转换为字符串后，将字符串存储在对象的字符串值字段中，并将对象的类型标签设置为T_STRING，表示对象现在是一个字符串类型
 if (svalue(obj) == NULL)
  return 1;
 tag(obj) = T_STRING;
 return 0;
}


/*
** Execute the given opcode. Return 0 in success or 1 on error.
*/
int lua_execute (Byte *pc)//执行给定的操作码pc，返回0表示成功，返回1表示错误
{
 Object *oldbase = base;//保存当前基址指针的值，以便在执行过程中需要重置Lua虚拟机状态时使用
 base = top;//将基址指针设置为当前栈顶位置，表示新的函数调用的基址位置，Lua虚拟机通过操作这个基址指针来管理函数调用和局部变量等操作
 while (1)
 {
  OpCode opcode;
  switch (opcode = (OpCode)*pc++)
  {
   case PUSHNIL: tag(top++) = T_NIL; break;
   
   case PUSH0: tag(top) = T_NUMBER; nvalue(top++) = 0; break;
   case PUSH1: tag(top) = T_NUMBER; nvalue(top++) = 1; break;
   case PUSH2: tag(top) = T_NUMBER; nvalue(top++) = 2; break;

   case PUSHBYTE: tag(top) = T_NUMBER; nvalue(top++) = *pc++; break;//将一个字节（0-255）推入Lua栈顶，适用于需要推入较小整数的情况
   
   case PUSHWORD: //将一个字（0-65535）推入Lua栈顶，适用于需要推入较大整数的情况
   {
    CodeWord code;
    get_word(code,pc);
    tag(top) = T_NUMBER; nvalue(top++) = code.w;
   }
   break;
   
   case PUSHFLOAT://将一个浮点数推入Lua栈顶，适用于需要推入浮点数的情况
   {
    CodeFloat code;
    get_float(code,pc);
    tag(top) = T_NUMBER; nvalue(top++) = code.f;
   }
   break;

   case PUSHSTRING://将一个字符串推入Lua栈顶，适用于需要推入字符串的情况
   {
    CodeWord code;
    get_word(code,pc);
    tag(top) = T_STRING; svalue(top++) = lua_constant[code.w];
   }
   break;
   
   case PUSHLOCAL0: case PUSHLOCAL1: case PUSHLOCAL2:
   case PUSHLOCAL3: case PUSHLOCAL4: case PUSHLOCAL5:
   case PUSHLOCAL6: case PUSHLOCAL7: case PUSHLOCAL8:
   case PUSHLOCAL9: *top++ = *(base + (int)(opcode-PUSHLOCAL0)); break;//将局部变量0到9推入Lua栈顶，局部变量的硬编码优化
   
   case PUSHLOCAL: *top++ = *(base + (*pc++)); break;//将一个局部变量推入Lua栈顶，局部变量的索引由紧跟在操作码后的一个字节指定
   
   case PUSHGLOBAL: //将一个全局变量推入Lua栈顶
   {
    CodeWord code;
    get_word(code,pc);
    *top++ = s_object(code.w);
   }
   break;
   
   case PUSHINDEXED://将一个索引的值推入Lua栈顶，适用于访问表中的元素的情况
    --top;
    if (tag(top-1) != T_ARRAY)//如果栈顶的值不是一个数组类型，说明无法进行索引操作，报告错误并返回1
    {
     lua_reportbug ("indexed expression not a table");
     return 1;
    }
    {
     Object *h = lua_hashdefine (avalue(top-1), top);
     if (h == NULL) return 1;
     *(top-1) = *h;//将表替换成索引的值，完成索引操作
    }
   break;
   
   case PUSHMARK: tag(top++) = T_MARK; break;//将一个标记对象推入Lua栈顶，标记函数调用的开始
   
   case PUSHOBJECT: *top = *(top-3); top++; break;//将一个Lua对象推入Lua栈顶，适用于需要推入任意类型对象的情况，这里直接将栈顶的值复制到下一个位置，完成对象的推入
   
   case STORELOCAL0: case STORELOCAL1: case STORELOCAL2://
   case STORELOCAL3: case STORELOCAL4: case STORELOCAL5:
   case STORELOCAL6: case STORELOCAL7: case STORELOCAL8:
   case STORELOCAL9: *(base + (int)(opcode-STORELOCAL0)) = *(--top); break;//将Lua栈顶的值存储到局部变量0到9中，局部变量的硬编码优化，存储前先将栈顶的值弹出
    
   case STORELOCAL: *(base + (*pc++)) = *(--top); break;//将Lua栈顶的值存储到一个局部变量中，局部变量的索引由紧跟在操作码后的一个字节指定，存储前先将栈顶的值弹出
   
   case STOREGLOBAL://将Lua栈顶的值存储到一个全局变量中，适用于需要修改全局环境中的变量的情况
   {
    CodeWord code;
    get_word(code,pc);
    s_object(code.w) = *(--top);
   }
   break;

   case STOREINDEXED0://将Lua栈顶的值存储到一个索引位置中，适用于修改表中的元素的情况
    if (tag(top-3) != T_ARRAY)
    {
     lua_reportbug ("indexed expression not a table");
     return 1;
    }
    {
     Object *h = lua_hashdefine (avalue(top-3), top-2);
     if (h == NULL) return 1;
     *h = *(top-1);
    }
    top -= 3;//调整栈顶指针，覆盖已经使用的栈
   break;
   
   case STOREINDEXED://将Lua栈顶的值存储到一个索引位置中，适用于修改表中的元素的情况
   {
    int n = *pc++;
    if (tag(top-3-n) != T_ARRAY)
    {
     lua_reportbug ("indexed expression not a table");
     return 1;
    }
    {
     Object *h = lua_hashdefine (avalue(top-3-n), top-2-n);
     if (h == NULL) return 1;
     *h = *(top-1);
    }
    top--;
   }
   break;
   
   case STORELIST0:
   case STORELIST://将Lua栈顶的值存储到一个列表位置中，适用于构造表时将元素存储到列表中的情况
   {
    int m, n;
    Object *arr;
    if (opcode == STORELIST0) m = 0;//如果操作码是STORELIST0，表示存储的元素数量为0，m设置为0
    else m = *(pc++) * FIELDS_PER_FLUSH;//如果操作码是STORELIST，表示存储的元素数量由紧跟在操作码后的一个字节指定，m设置为这个值乘以每次刷新多少个变量的常量FIELDS_PER_FLUSH
    n = *(pc++);
    arr = top-n-1;
    if (tag(arr) != T_ARRAY)
    {
     lua_reportbug ("internal error - table expected");
     return 1;
    }
    while (n)//循环将栈顶的值存储到表arr中，索引从n+m开始递减，直到存储完所有元素，完成列表元素的存储
    {
     tag(top) = T_NUMBER; nvalue(top) = n+m;
     *(lua_hashdefine (avalue(arr), top)) = *(top-1);//将栈顶的值存储到表arr中，索引为n+m，完成列表元素的存储
     top--;
     n--;
    }
   }
   break;
   
   case STORERECORD://字典初始化操作，处理形如 {x=1, y=2} 的语法
   {
    int n = *(pc++);
    Object *arr = top-n-1;
    if (tag(arr) != T_ARRAY)
    {
     lua_reportbug ("internal error - table expected");
     return 1;
    }
    while (n)
    {
     CodeWord code;
     get_word(code,pc);
     tag(top) = T_STRING; svalue(top) = lua_constant[code.w];//将一个字符串常量推入栈顶，字符串常量的索引由紧跟在操作码后的一个字指定，完成记录键的准备
     *(lua_hashdefine (avalue(arr), top)) = *(top-1);//将栈顶的值存储到表arr中，索引为刚才推入的字符串常量，完成记录元素的存储
     top--;
     n--;
    }
   }
   break;
   
   case ADJUST://调整Lua栈的大小，适用于函数调用前调整栈空间以容纳参数和局部变量的情况
   {
    Object *newtop = base + *(pc++);//计算新的栈顶位置，新的栈顶位置由基址加上紧跟在操作码后的一个字节指定，表示需要调整的栈空间大小
    while (top < newtop) tag(top++) = T_NIL;
    top = newtop;  /* top could be bigger than newtop */
   }
   break;
   
   case CREATEARRAY://创建一个新的table表对象
    if (tag(top-1) == T_NIL) //如果栈顶的值是nil，表示没有指定表的初始大小，默认设置为101，适用于大多数情况，避免频繁调整表的大小
     nvalue(top-1) = 101;
    else 
    {
     if (tonumber(top-1)) return 1;//如果栈顶的值无法转换为数字，报告错误并返回1
     if (nvalue(top-1) <= 0) nvalue(top-1) = 101;//如果栈顶的值是一个非正数，默认设置为101
    }
    avalue(top-1) = lua_createarray(nvalue(top-1));//创建一个新的table表对象
    if (avalue(top-1) == NULL)
     return 1;//如果创建表对象失败，报告错误并返回1
    tag(top-1) = T_ARRAY;
   break;
   
   case EQOP://比较栈顶的两个值是否相等，并将结果作为一个数字（1相等，0不相等）存储在栈顶位置
   {
    Object *l = top-2;
    Object *r = top-1;
    --top;
    if (tag(l) != tag(r)) 
     tag(top-1) = T_NIL;//如果两个值的类型不同，直接认为它们不相等，结果为nil
    else
    {
     switch (tag(l))
     {
      case T_NIL:       tag(top-1) = T_NUMBER; break;//如果两个值都是nil，认为它们相等，结果为数字1，类型为T_NUMBER
      case T_NUMBER:    tag(top-1) = (nvalue(l) == nvalue(r)) ? T_NUMBER : T_NIL; break;
      case T_ARRAY:     tag(top-1) = (avalue(l) == avalue(r)) ? T_NUMBER : T_NIL; break;
      case T_FUNCTION:  tag(top-1) = (bvalue(l) == bvalue(r)) ? T_NUMBER : T_NIL; break;
      case T_CFUNCTION: tag(top-1) = (fvalue(l) == fvalue(r)) ? T_NUMBER : T_NIL; break;
      case T_USERDATA:  tag(top-1) = (uvalue(l) == uvalue(r)) ? T_NUMBER : T_NIL; break;
      case T_STRING:    tag(top-1) = (strcmp (svalue(l), svalue(r)) == 0) ? T_NUMBER : T_NIL; break;
      case T_MARK:      return 1;//如果两个值都是标记对象，认为它们不相等，直接返回1表示错误，标记对象不应该参与比较
     }
    }
    nvalue(top-1) = 1;//将比较结果设置为数字1，表示相等，类型为T_NUMBER；如果不相等，类型为T_NIL，数值部分不使用
   }
   break;
    
   case LTOP://比较栈顶的两个值的是否小于，并将结果作为一个数字（1小于，0不小于）存储在栈顶位置
   {
    Object *l = top-2;
    Object *r = top-1;
    --top;
    if (tag(l) == T_NUMBER && tag(r) == T_NUMBER)
     tag(top-1) = (nvalue(l) < nvalue(r)) ? T_NUMBER : T_NIL;
    else
    {
     if (tostring(l) || tostring(r))//如果两个值中有一个无法转换为字符串，报告错误并返回1，因为无法进行大小比较
      return 1;
     tag(top-1) = (strcmp (svalue(l), svalue(r)) < 0) ? T_NUMBER : T_NIL;
    }
    nvalue(top-1) = 1; 
   }
   break;
   
   case LEOP://比较栈顶的两个值的是否小于或等于，并将结果作为一个数字（1小于或等于，0不小于或等于）存储在栈顶位置
   {
    Object *l = top-2;
    Object *r = top-1;
    --top;
    if (tag(l) == T_NUMBER && tag(r) == T_NUMBER)
     tag(top-1) = (nvalue(l) <= nvalue(r)) ? T_NUMBER : T_NIL;
    else
    {
     if (tostring(l) || tostring(r))
      return 1;
     tag(top-1) = (strcmp (svalue(l), svalue(r)) <= 0) ? T_NUMBER : T_NIL;
    }
    nvalue(top-1) = 1; 
   }
   break;
   
   case ADDOP://将栈顶的两个值相加，并将结果存储在栈顶位置
   {
    Object *l = top-2;
    Object *r = top-1;
    if (tonumber(r) || tonumber(l))
     return 1;
    nvalue(l) += nvalue(r);
    --top;
   }
   break; 
   
   case SUBOP://将栈顶的两个值相减，并将结果存储在栈顶位置
   {
    Object *l = top-2;
    Object *r = top-1;
    if (tonumber(r) || tonumber(l))
     return 1;
    nvalue(l) -= nvalue(r);
    --top;
   }
   break; 
   
   case MULTOP://将栈顶的两个值相乘，并将结果存储在栈顶位置
   {
    Object *l = top-2;
    Object *r = top-1;
    if (tonumber(r) || tonumber(l))
     return 1;
    nvalue(l) *= nvalue(r);
    --top;
   }
   break; 
   
   case DIVOP://将栈顶的两个值相除，并将结果存储在栈顶位置
   {
    Object *l = top-2;
    Object *r = top-1;
    if (tonumber(r) || tonumber(l))
     return 1;
    nvalue(l) /= nvalue(r);
    --top;
   }
   break; 
   
   case CONCOP://将栈顶的两个值连接成一个字符串，并将结果存储在栈顶位置
   {
    Object *l = top-2;
    Object *r = top-1;
    if (tostring(r) || tostring(l))
     return 1;
    svalue(l) = lua_createstring (lua_strconc(svalue(l),svalue(r)));
    if (svalue(l) == NULL)//如果字符串连接失败，报告错误并返回1
     return 1;
    --top;
   }
   break; 
   
   case MINUSOP://将栈顶的值取负，并将结果存储在栈顶位置
    if (tonumber(top-1))
     return 1;
    nvalue(top-1) = - nvalue(top-1);
   break; 
   
   case NOTOP://将栈顶的值取反，并将结果存储在栈顶位置，nil和false被认为是false，其他值被认为是true
    tag(top-1) = tag(top-1) == T_NIL ? T_NUMBER : T_NIL;
   break; 
   
   case ONTJMP://条件跳转，如果Lua栈顶的值为真，则跳转到指定位置，否则继续执行下一条指令
   {
    CodeWord code;
    get_word(code,pc);
    if (tag(top-1) != T_NIL) pc += code.w;
   }
   break;
   
   case ONFJMP:	 //条件跳转，如果Lua栈顶的值为假，则跳转到指定位置，否则继续执行下一条指令  
   {
    CodeWord code;
    get_word(code,pc);
    if (tag(top-1) == T_NIL) pc += code.w;
   }
   break;
   
   case JMP://默认（向下）无条件跳转，直接跳转到指定位置
   {
    CodeWord code;
    get_word(code,pc);
    pc += code.w;
   }
   break;
    
   case UPJMP://向上无条件跳转，直接跳转到指定位置
   {
    CodeWord code;
    get_word(code,pc);
    pc -= code.w;
   }
   break;
   
   case IFFJMP://条件跳转，如果Lua栈顶的值为假，则跳转到指定位置，否则继续执行下一条指令
   {
    CodeWord code;
    get_word(code,pc);
    top--;//弹出栈顶的值，因为条件跳转后这个值不再需要了
    if (tag(top) == T_NIL) pc += code.w;
   }
   break;

   case IFFUPJMP://条件跳转，如果Lua栈顶的值为假，则向上跳转到指定位置继续执行，否则继续执行下一条指令
   {
    CodeWord code;
    get_word(code,pc);
    top--;//弹出栈顶的值
    if (tag(top) == T_NIL) pc -= code.w;
   }
   break;

   case POP: --top; break;//从Lua栈顶弹出一个值，丢弃它，适用于不再需要某个值的情况
   
   case CALLFUNC://调用一个函数，适用于函数调用的情况
   {
    Byte *newpc;//保存函数的入口地址，准备跳转执行函数的代码
    Object *b = top-1;//从栈顶开始向下查找，找到第一个标记对象，标记对象之前的值应该是一个函数对象，准备调用这个函数
    while (tag(b) != T_MARK) b--;
    if (tag(b-1) == T_FUNCTION)
    {
     lua_debugline = 0;			/* always reset debug flag */
     newpc = bvalue(b-1);//获取函数对象的入口地址，准备跳转执行函数的代码
     bvalue(b-1) = pc;		        /* store return code *///将当前指令的地址存储在函数对象中，以便函数执行完后能够返回到正确的位置继续执行
     nvalue(b) = (base-stack);		/* store base value *///将当前基址指针与栈底的距离存储在标记对象中，以便函数执行过程中能够正确访问参数和局部变量
     base = b+1;
     pc = newpc;
     if (MAXSTACK-(base-stack) < STACKGAP)//检查栈空间是否足够，如果不足够，报告错误并返回1，避免栈溢出
     {
      lua_error ("stack overflow");
      return 1;
     }
    }
    else if (tag(b-1) == T_CFUNCTION)
    {
     int nparam; //计算函数参数的数量，准备调用C函数
     lua_debugline = 0;			/* always reset debug flag */
     nvalue(b) = (base-stack);		/* store base value */
     base = b+1;
     nparam = top-base;			/* number of parameters */
     (fvalue(b-1))();			/* call C function *///调用C函数，C函数通过访问Lua栈来获取参数和返回值，C函数执行完后需要将返回值放在Lua栈上
     
     /* shift returned values */
     { 
      int i;
      int nretval = top - base - nparam;//返回值的数量
      top = base - 2;//调整栈顶指针，准备将返回值从Lua栈上移到正确的位置，返回值应该放在基址之前的位置，以便调用者能够访问到
      base = stack + (int) nvalue(base-1);
      for (i=0; i<nretval; i++)
      {
       *top = *(top+nparam+2);
       ++top;
      }
     }
    }
    else
    {
     lua_reportbug ("call expression not a function");
     return 1;
    }
   }
   break;
   
   case RETCODE://函数返回操作，适用于函数执行完毕后返回到调用者继续执行的情况
   {
    int i;
    int shift = *pc++;//获取返回值的数量
    int nretval = top - base - shift;
    top = base - 2;
    pc = bvalue(base-2);//获取返回地址，准备跳转回调用者继续执行
    base = stack + (int) nvalue(base-1);
    for (i=0; i<nretval; i++)
    {
     *top = *(top+shift+2);
     ++top;
    }
   }
   break;
   
   case HALT://程序执行结束，适用于程序正常结束的情况
    base = oldbase;
   return 0;		/* success */
   
   case SETFUNCTION://将一个函数对象推入Lua栈顶，适用于函数定义的情况
   {
    CodeWord file, func;
    get_word(file,pc);
    get_word(func,pc);
    if (lua_pushfunction (file.w, func.w))
     return 1;
   }
   break;
   
   case SETLINE://设置当前行号，适用于调试信息的更新
   {
    CodeWord code;
    get_word(code,pc);
    lua_debugline = code.w;
   }
   break;
   
   case RESET://重置Lua虚拟机的状态，适用于错误处理或异常情况的恢复
    lua_popfunction ();
   break;
   
   default:
    lua_error ("internal error - opcode didn't match");
   return 1;
  }
 }
}


/*
** Traverse all objects on stack
*/
void lua_travstack (void (*fn)(Object *))//遍历Lua栈上的所有对象，fn是一个函数指针，遍历过程中会调用这个函数来处理每个对象
{
 Object *o;
 for (o = top-1; o >= stack; o--)
  fn (o);
}

/*
** Open file, generate opcode and execute global statement. Return 0 on
** success or 1 on error.
*/
int lua_dofile (char *filename)//打开一个文件，生成操作码并执行全局语句，返回0表示成功，返回1表示错误
{
 if (lua_openfile (filename)) return 1;
 if (lua_parse ()) { lua_closefile (); return 1; }
 lua_closefile ();
 return 0;
}

/*
** Generate opcode stored on string and execute global statement. Return 0 on
** success or 1 on error.
*/
int lua_dostring (char *string)//生成存储在字符串上的操作码并执行全局语句，返回0表示成功，返回1表示错误
{
 if (lua_openstring (string)) return 1;
 if (lua_parse ()) return 1;
 lua_closestring();
 return 0;
}

/*
** Execute the given function. Return 0 on success or 1 on error.
*/
int lua_call (char *functionname, int nparam)//执行给定的函数，函数名由字符串functionname指定，参数数量由整数nparam指定，返回0表示成功，返回1表示错误
{
 static Byte startcode[] = {CALLFUNC, HALT};
 int i; 
 Object func = s_object(lua_findsymbol(functionname));//在全局环境符号表中查找函数名对应的对象，如果找不到，返回一个nil对象
 if (tag(&func) != T_FUNCTION) return 1;
 for (i=1; i<=nparam; i++)
  *(top-i+2) = *(top-i);
 top += 2;
 tag(top-nparam-1) = T_MARK;
 *(top-nparam-2) = func;
 return (lua_execute (startcode));
}

/*
** Get a parameter, returning the object handle or NULL on error.
** 'number' must be 1 to get the first parameter.
*/
Object *lua_getparam (int number)//获取一个参数，返回对象句柄或在错误时返回NULL
{
 if (number <= 0 || number > top-base) return NULL;//如果参数编号小于等于0或者大于当前基址和栈顶之间的距离，说明参数编号无效，返回NULL表示错误
 return (base+number-1);
}

/*
** Given an object handle, return its number value. On error, return 0.0.
*/
real lua_getnumber (Object *object)//给定一个对象句柄，返回它的数值，如果发生错误，返回0.0
{
 if (object == NULL || tag(object) == T_NIL) return 0.0;
 if (tonumber (object)) return 0.0;
 else                   return (nvalue(object));
}

/*
** Given an object handle, return its string pointer. On error, return NULL.
*/
char *lua_getstring (Object *object)//给定一个对象句柄，返回它的字符串指针，如果发生错误，返回NULL
{
 if (object == NULL || tag(object) == T_NIL) return NULL;
 if (tostring (object)) return NULL;
 else                   return (svalue(object));
}

/*
** Given an object handle, return a copy of its string. On error, return NULL.
*/
char *lua_copystring (Object *object)//给定一个对象句柄，返回它的字符串的副本，如果发生错误，返回NULL
{
 if (object == NULL || tag(object) == T_NIL) return NULL;
 if (tostring (object)) return NULL;
 else                   return (strdup(svalue(object)));
}

/*
** Given an object handle, return its cfuntion pointer. On error, return NULL.
*/
lua_CFunction lua_getcfunction (Object *object)//给定一个对象句柄，返回它的C函数指针，如果发生错误，返回NULL
{
 if (object == NULL) return NULL;
 if (tag(object) != T_CFUNCTION) return NULL;
 else                            return (fvalue(object));
}

/*
** Given an object handle, return its user data. On error, return NULL.
*/
void *lua_getuserdata (Object *object)//给定一个对象句柄，返回它的用户数据，如果发生错误，返回NULL
{
 if (object == NULL) return NULL;
 if (tag(object) != T_USERDATA) return NULL;
 else                           return (uvalue(object));
}

/*
** Given an object handle and a field name, return its field object.
** On error, return NULL.
*/
Object *lua_getfield (Object *object, char *field)//给定一个对象句柄和一个字段名，返回它的字段对象，如果发生错误，返回NULL
{
 if (object == NULL) return NULL;
 if (tag(object) != T_ARRAY)
  return NULL;
 else
 {
  Object ref;
  tag(&ref) = T_STRING;
  svalue(&ref) = lua_createstring(lua_strdup(field));
  return (lua_hashdefine(avalue(object), &ref));
 }
}

/*
** Given an object handle and an index, return its indexed object.
** On error, return NULL.
*/
Object *lua_getindexed (Object *object, float index)//给定一个对象句柄和一个索引，返回它的索引对象，如果发生错误，返回NULL
{
 if (object == NULL) return NULL;
 if (tag(object) != T_ARRAY)
  return NULL;
 else
 {
  Object ref;
  tag(&ref) = T_NUMBER;
  nvalue(&ref) = index;
  return (lua_hashdefine(avalue(object), &ref));
 }
}

/*
** Get a global object. Return the object handle or NULL on error.
*/
Object *lua_getglobal (char *name)//获取一个全局对象，返回对象句柄或在错误时返回NULL
{
 int n = lua_findsymbol(name);
 if (n < 0) return NULL;
 return &s_object(n);
}

/*
** Pop and return an object
*/
Object *lua_pop (void)//从Lua栈顶弹出一个对象并返回它的句柄，如果栈已经空了，返回NULL表示错误
{
 if (top <= base) return NULL;
 top--;
 return top;
}

/*
** Push a nil object
*/
int lua_pushnil (void)//将一个nil对象推入Lua栈顶，返回0表示成功，返回1表示错误
{
 if ((top-stack) >= MAXSTACK-1)
 {
  lua_error ("stack overflow");
  return 1;
 }
 tag(top) = T_NIL;
 return 0;
}

/*
** Push an object (tag=number) to stack. Return 0 on success or 1 on error.
*/
int lua_pushnumber (real n)//将一个数字对象推入Lua栈顶，返回0表示成功，返回1表示错误
{
 if ((top-stack) >= MAXSTACK-1)
 {
  lua_error ("stack overflow");
  return 1;
 }
 tag(top) = T_NUMBER; nvalue(top++) = n;
 return 0;
}

/*
** Push an object (tag=string) to stack. Return 0 on success or 1 on error.
*/
int lua_pushstring (char *s)//将一个字符串对象推入Lua栈顶，返回0表示成功，返回1表示错误
{
 if ((top-stack) >= MAXSTACK-1)
 {
  lua_error ("stack overflow");
  return 1;
 }
 tag(top) = T_STRING; 
 svalue(top++) = lua_createstring(lua_strdup(s));
 return 0;
}

/*
** Push an object (tag=cfunction) to stack. Return 0 on success or 1 on error.
*/
int lua_pushcfunction (lua_CFunction fn)//将一个C函数对象推入Lua栈顶，返回0表示成功，返回1表示错误
{
 if ((top-stack) >= MAXSTACK-1)
 {
  lua_error ("stack overflow");
  return 1;
 }
 tag(top) = T_CFUNCTION; fvalue(top++) = fn;
 return 0;
}

/*
** Push an object (tag=userdata) to stack. Return 0 on success or 1 on error.
*/
int lua_pushuserdata (void *u)//将一个用户数据对象推入Lua栈顶，返回0表示成功，返回1表示错误
{
 if ((top-stack) >= MAXSTACK-1)
 {
  lua_error ("stack overflow");
  return 1;
 }
 tag(top) = T_USERDATA; uvalue(top++) = u;
 return 0;
}

/*
** Push an object to stack.
*/
int lua_pushobject (Object *o)//将一个对象推入Lua栈顶，返回0表示成功，返回1表示错误
{
 if ((top-stack) >= MAXSTACK-1)
 {
  lua_error ("stack overflow");
  return 1;
 }
 *top++ = *o;
 return 0;
}

/*
** Store top of the stack at a global variable array field. 
** Return 1 on error, 0 on success.
*/
int lua_storeglobal (char *name)//将Lua栈顶的值存储到一个全局变量中，返回1表示错误，返回0表示成功
{
 int n = lua_findsymbol (name);
 if (n < 0) return 1;
 if (tag(top-1) == T_MARK) return 1;//如果栈顶的值是一个标记对象，说明无法存储到全局变量中，报告错误并返回1
 s_object(n) = *(--top);
 return 0;
}

/*
** Store top of the stack at an array field. Return 1 on error, 0 on success.
*/
int lua_storefield (lua_Object object, char *field)//将Lua栈顶的值存储到一个数组字段中，返回1表示错误，返回0表示成功
{
 if (tag(object) != T_ARRAY)
  return 1;
 else
 {
  Object ref, *h;
  tag(&ref) = T_STRING;
  svalue(&ref) = lua_createstring(lua_strdup(field));
  h = lua_hashdefine(avalue(object), &ref);
  if (h == NULL) return 1;
  if (tag(top-1) == T_MARK) return 1;
  *h = *(--top);
 }
 return 0;
}


/*
** Store top of the stack at an array index. Return 1 on error, 0 on success.
*/
int lua_storeindexed (lua_Object object, float index)//将Lua栈顶的值存储到一个数组索引位置中，返回1表示错误，返回0表示成功
{
 if (tag(object) != T_ARRAY)
  return 1;
 else
 {
  Object ref, *h;
  tag(&ref) = T_NUMBER;
  nvalue(&ref) = index;
  h = lua_hashdefine(avalue(object), &ref);
  if (h == NULL) return 1;
  if (tag(top-1) == T_MARK) return 1;
  *h = *(--top);
 }
 return 0;
}


/*
** Given an object handle, return if it is nil.
*/
int lua_isnil (Object *object)//给定一个对象句柄，返回它是否是nil
{
 return (object != NULL && tag(object) == T_NIL);
}

/*
** Given an object handle, return if it is a number one.
*/
int lua_isnumber (Object *object)//给定一个对象句柄，返回它是否是一个数字对象
{
 return (object != NULL && tag(object) == T_NUMBER);
}

/*
** Given an object handle, return if it is a string one.
*/
int lua_isstring (Object *object)//给定一个对象句柄，返回它是否是一个字符串对象
{
 return (object != NULL && tag(object) == T_STRING);
}

/*
** Given an object handle, return if it is an array one.
*/
int lua_istable (Object *object)//给定一个对象句柄，返回它是否是一个数组对象（table表）
{
 return (object != NULL && tag(object) == T_ARRAY);
}

/*
** Given an object handle, return if it is a cfunction one.
*/
int lua_iscfunction (Object *object)//给定一个对象句柄，返回它是否是一个C函数对象
{
 return (object != NULL && tag(object) == T_CFUNCTION);
}

/*
** Given an object handle, return if it is an user data one.
*/
int lua_isuserdata (Object *object)//给定一个对象句柄，返回它是否是一个用户数据对象
{
 return (object != NULL && tag(object) == T_USERDATA);
}

/*
** Internal function: return an object type. 
*/
void lua_type (void)//内部函数：返回一个对象的类型，适用于需要获取对象类型信息的情况
{
 Object *o = lua_getparam(1);
 lua_pushstring (lua_constant[tag(o)]);
}

/*
** Internal function: convert an object to a number
*/
void lua_obj2number (void)//内部函数：将一个对象转换为数字，适用于需要将对象转换为数值进行计算的情况
{
 Object *o = lua_getparam(1);
 lua_pushobject (lua_convtonumber(o));
}

/*
** Internal function: print object values
*/
void lua_print (void)//内部函数：打印对象的值，适用于调试或输出对象信息的情况
{
 int i=1;
 void *obj;
 while ((obj=lua_getparam (i++)) != NULL)//循环获取参数，直到没有更多参数为止，适用于打印多个对象的情况
 {
  if      (lua_isnumber(obj))    printf("%g\n",lua_getnumber (obj));
  else if (lua_isstring(obj))    printf("%s\n",lua_getstring (obj));
  else if (lua_iscfunction(obj)) printf("cfunction: %p\n",lua_getcfunction (obj));
  else if (lua_isuserdata(obj))  printf("userdata: %p\n",lua_getuserdata (obj));
  else if (lua_istable(obj))     printf("table: %p\n",obj);
  else if (lua_isnil(obj))       printf("nil\n");
  else			         printf("invalid value to print\n");
 }
}

/*
** Internal function: do a file
*/
void lua_internaldofile (void)//内部函数：执行一个文件，适用于需要在Lua环境中执行一个外部文件的情况
{
 lua_Object obj = lua_getparam (1);
 if (lua_isstring(obj) && !lua_dofile(lua_getstring(obj)))//如果参数是一个字符串对象，并且成功执行了这个文件
  lua_pushnumber(1);//如果成功执行了这个文件，将数字1推入Lua栈顶，表示成功
 else
  lua_pushnil();
}

/*
** Internal function: do a string
*/
void lua_internaldostring (void)//内部函数：执行一个字符串，适用于需要在Lua环境中执行一段代码字符串的情况
{
 lua_Object obj = lua_getparam (1);
 if (lua_isstring(obj) && !lua_dostring(lua_getstring(obj)))//如果参数是一个字符串对象，并且成功执行了这个字符串
  lua_pushnumber(1);
 else
  lua_pushnil();
}


