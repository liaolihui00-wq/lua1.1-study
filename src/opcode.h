/*
** TeCGraf - PUC-Rio
** $Id: opcode.h,v 2.1 1994/04/20 22:07:57 celes Exp $
*/

//定义了 Lua 虚拟机（VM）能理解的所有底层指令（OpCode），以及 Lua 最核心的数据结构 Object
#ifndef opcode_h
#define opcode_h

#ifndef STACKGAP//栈保护区间，为虚拟机执行预留的缓冲区，防止计算过程中栈溢出
#define STACKGAP	128  
#endif 

#ifndef real//Lua中用来表示数字的类型，默认是float
#define real float
#endif

#define FIELDS_PER_FLUSH 40//当Lua在执行过程中需要将局部变量从寄存器刷新到内存中时，每次刷新多少个变量，这个值是为了优化性能

typedef unsigned char  Byte;//Lua虚拟机指令的基本单位，表示一个字节

typedef unsigned short Word;//Lua虚拟机指令的另一个基本单位，表示一个字，通常是两个字节，用于存储指令的操作码和操作数

typedef union
{
 struct {char c1; char c2;} m;
 Word w;
} CodeWord;//CodeWord联合体，用于在Lua虚拟机中表示指令的操作码和操作数

typedef union
{
 struct {char c1; char c2; char c3; char c4;} m;
 float f;
} CodeFloat;//CodeFloat联合体，用于在Lua虚拟机中表示一个浮点数

typedef enum 
{ 
 PUSHNIL,//将一个nil对象推入Lua栈顶
 PUSH0, PUSH1, PUSH2,//将数字0到2推入Lua栈顶，常用小整数的硬编码优化，省去读取参数的时间
 PUSHBYTE,//将一个字节（0-255）推入Lua栈顶，适用于需要推入较小整数的情况
 PUSHWORD,//将一个字（0-65535）推入Lua栈顶，适用于需要推入较大整数的情况
 PUSHFLOAT,//将一个浮点数推入Lua栈顶，适用于需要推入浮点数的情况
 PUSHSTRING,//将一个字符串推入Lua栈顶，适用于需要推入字符串的情况
 PUSHLOCAL0, PUSHLOCAL1, PUSHLOCAL2, PUSHLOCAL3, PUSHLOCAL4,
 PUSHLOCAL5, PUSHLOCAL6, PUSHLOCAL7, PUSHLOCAL8, PUSHLOCAL9,//将局部变量0到9推入Lua栈顶，常用局部变量的硬编码优化
 PUSHLOCAL,//将一个局部变量推入Lua栈顶，适用于需要推入较多局部变量的情况
 PUSHGLOBAL,//将一个全局变量推入Lua栈顶，适用于需要访问全局环境中的变量的情况
 PUSHINDEXED,//将一个索引的值推入Lua栈顶，适用于访问表中的元素的情况
 PUSHMARK,//将一个标记对象推入Lua栈顶，标记函数调用的开始
 PUSHOBJECT,//将一个Lua对象推入Lua栈顶，适用于需要推入任意类型对象的情况
 STORELOCAL0, STORELOCAL1, STORELOCAL2, STORELOCAL3, STORELOCAL4,
 STORELOCAL5, STORELOCAL6, STORELOCAL7, STORELOCAL8, STORELOCAL9,//将Lua栈顶的值存储到局部变量0到9中，常用局部变量的硬编码优化
 STORELOCAL,//将Lua栈顶的值存储到一个局部变量中，适用于需要存储较多局部变量的情况
 STOREGLOBAL,//将Lua栈顶的值存储到一个全局变量中，适用于需要修改全局环境中的变量的情况
 STOREINDEXED0,//将Lua栈顶的值存储到一个索引位置中，适用于修改表中的元素的情况
 STOREINDEXED,
 STORELIST0,//将Lua栈顶的值存储到一个列表位置中，适用于构造表时将元素存储到列表中的情况
 STORELIST,
 STORERECORD,//将Lua栈顶的值存储到一个记录位置中，适用于构造表时将元素存储到记录中的情况
 ADJUST,//调整Lua栈的大小，适用于函数调用前调整栈空间以容纳参数和局部变量的情况
 CREATEARRAY,//创建一个新的table表对象
 EQOP,//等于
 LTOP,//小于
 LEOP,//小于等于
 ADDOP,//加法
 SUBOP,//减法
 MULTOP,//乘法
 DIVOP,//除法
 CONCOP,//连接字符串
 MINUSOP,//取负
 NOTOP,//逻辑非
 ONTJMP,//条件跳转，如果Lua栈顶的值为真，则跳转到指定位置，否则继续执行下一条指令
 ONFJMP,//条件跳转，如果Lua栈顶的值为假，则跳转到指定位置，否则继续执行下一条指令
 JMP,//无条件跳转，跳转到指定位置继续执行
 UPJMP,//向上跳转，跳转到指定位置继续执行，通常用于循环结构中
 IFFJMP,//条件跳转，如果Lua栈顶的值为假，则跳转到指定位置，否则继续执行下一条指令，适用于if语句的情况
 IFFUPJMP,//条件跳转，如果Lua栈顶的值为假，则向上跳转到指定位置继续执行，否则继续执行下一条指令，适用于if语句的情况
 POP,//从Lua栈顶弹出一个值，丢弃它，适用于不再需要某个值的情况
 CALLFUNC,//调用一个函数，适用于函数调用的情况
 RETCODE,//从函数返回，适用于函数返回的情况
 HALT,//停止Lua虚拟机的执行，适用于程序结束或遇到不可恢复错误的情况
 SETFUNCTION,//将一个函数定义推入Lua栈顶，适用于函数定义的情况
 SETLINE,//设置当前行号，适用于调试信息和错误报告中显示正确的行号的情况
 RESET//重置Lua虚拟机的状态，适用于重新初始化虚拟机或清理资源的情况
} OpCode;//Lua虚拟机支持的所有指令的枚举类型，每个指令对应一个操作码（OpCode），Lua虚拟机通过执行这些指令来实现Lua代码的功能

typedef enum
{
 T_MARK,//mark对象
 T_NIL,//nil对象，表示Lua中的空值
 T_NUMBER,//数字对象
 T_STRING,//字符串对象
 T_ARRAY,//表对象，Lua中的核心数据结构，可以用来表示数组、字典等各种数据结构
 T_FUNCTION,//函数对象
 T_CFUNCTION,//c函数对象，表示一个C语言函数，可以被Lua调用
 T_USERDATA//用户数据对象，表示一个由C语言定义的任意数据结构，可以被Lua操作，但Lua本身不关心它的内部结构
} Type; //Lua对象的类型枚举，表示Lua中的不同数据类型，如nil、数字、字符串、表、函数、C函数和用户数据等

typedef void (*Cfunction) (void);
typedef int  (*Input) (void);

typedef union
{
 Cfunction 	 f;//函数指针
 real    	 n;//数字值
 char      	*s;//字符串值
 Byte      	*b;//字节值，通常用于存储小整数或操作码等
 struct Hash    *a;//表对象指针
 void           *u;//用户数据指针
} Value;//Lua对象的值联合体，根据对象的类型不同，value可以存储不同类型的数据，如数字、字符串、表、函数等

typedef struct Object
{
 Type  tag;
 Value value;
} Object;//Lua对象结构体，包含一个类型标签（tag）和一个值（value），tag表示对象的类型，value根据tag的不同可以存储不同类型的数据

typedef struct
{
 char   *name;//变量名，符号表项的名字，用于在Lua代码中引用这个变量
 Object  object;//变量值，符号表项的对象，记录这个变量对应的Lua对象信息，如类型和值等，用于在Lua代码中访问和操作这个变量
} Symbol;//符号表项结构体，包含一个名字（name）和一个对象（object），用于在Lua的符号表中记录变量名和对应的对象信息

/* Macros to access structure members */
#define tag(o)		((o)->tag)//获取Lua对象的类型标签
#define nvalue(o)	((o)->value.n)//获取Lua对象的数字值
#define svalue(o)	((o)->value.s)//获取Lua对象的字符串值
#define bvalue(o)	((o)->value.b)//字节值
#define avalue(o)	((o)->value.a)//lua对象的指针值
#define fvalue(o)	((o)->value.f)//函数指针值
#define uvalue(o)	((o)->value.u)//用户数据表

/* Macros to access symbol table */
#define s_name(i)	(lua_table[i].name)//
#define s_object(i)	(lua_table[i].object)
#define s_tag(i)	(tag(&s_object(i)))
#define s_nvalue(i)	(nvalue(&s_object(i)))
#define s_svalue(i)	(svalue(&s_object(i)))
#define s_bvalue(i)	(bvalue(&s_object(i)))
#define s_avalue(i)	(avalue(&s_object(i)))
#define s_fvalue(i)	(fvalue(&s_object(i)))
#define s_uvalue(i)	(uvalue(&s_object(i)))

#define get_word(code,pc)    {code.m.c1 = *pc++; code.m.c2 = *pc++;}
#define get_float(code,pc)   {code.m.c1 = *pc++; code.m.c2 = *pc++;\
                              code.m.c3 = *pc++; code.m.c4 = *pc++;}
 


/* Exported functions */
int     lua_execute   (Byte *pc);
void    lua_markstack (void);
char   *lua_strdup (char *l);

void    lua_setinput   (Input fn);	/* from "lex.c" module */
char   *lua_lasttext   (void);		/* from "lex.c" module */
int     lua_parse      (void); 		/* from "lua.stx" module */
void    lua_type       (void);
void 	lua_obj2number (void);
void 	lua_print      (void);
void 	lua_internaldofile (void);
void 	lua_internaldostring (void);
void    lua_travstack (void (*fn)(Object *));

#endif
