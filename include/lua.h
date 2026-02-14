/*
** LUA - Linguagem para Usuarios de Aplicacao
** Grupo de Tecnologia em Computacao Grafica
** TeCGraf - PUC-Rio
** $Id: lua.h,v 1.1 1993/12/17 18:41:19 celes Exp $
*/


#ifndef lua_h
#define lua_h

typedef void (*lua_CFunction) (void);//c函数指针，用于让lua调用c代码
typedef struct Object *lua_Object;//lua对象指针，lua中的所有对象都用这个类型表示，保持封装性

#define lua_register(n,f)	(lua_pushcfunction(f), lua_storeglobal(n))


void           lua_errorfunction    	(void (*fn) (char *s));//自定义错误处理函数，lua调用这个函数来处理错误
void           lua_error		(char *s);//lua调用这个函数来抛出错误，s是错误信息
int            lua_dofile 		(char *filename);//解析并运行一个lua文件，filename是文件名
int            lua_dostring 		(char *string);//解析并运行一段lua代码，string是代码字符串
int            lua_call 		(char *functionname, int nparam);//调用一个lua函数，functionname是函数名，nparam是参数个数

lua_Object     lua_getparam 		(int number);//获取函数参数，number是参数的索引，从1开始
float          lua_getnumber 		(lua_Object object);//从lua对象中获取一个数字，object是lua对象
char          *lua_getstring 		(lua_Object object);//从lua对象中获取一个字符串
char 	      *lua_copystring 		(lua_Object object);//从lua对象中获取一个字符串的副本，返回一个新的字符串指针
lua_CFunction  lua_getcfunction 	(lua_Object object);//从lua对象中获取一个c函数指针
void          *lua_getuserdata  	(lua_Object object);//从lua对象中获取一个用户数据指针
lua_Object     lua_getfield         	(lua_Object object, char *field);//从lua对象中获取一个字段的值，field是字段名
lua_Object     lua_getindexed         	(lua_Object object, float index);//从lua对象中获取一个索引的值，index是索引值
lua_Object     lua_getglobal 		(char *name);//从全局环境中获取一个变量的值，name是变量名

lua_Object     lua_pop 			(void);//从lua栈顶弹出一个对象，返回这个对象

//栈与推送
int 	       lua_pushnil 		(void);//向lua栈顶推入一个nil对象
int            lua_pushnumber 		(float n);//向lua栈顶推入一个数字对象，n是数字值
int            lua_pushstring 		(char *s);
int            lua_pushcfunction	(lua_CFunction fn);
int            lua_pushuserdata     	(void *u);
int            lua_pushobject       	(lua_Object object);

//储存数据
int            lua_storeglobal		(char *name);//将lua栈顶的对象储存在全局环境中，name是变量名
int            lua_storefield		(lua_Object object, char *field);//将lua栈顶的对象储存在lua对象的一个字段中，field是字段名
int            lua_storeindexed		(lua_Object object, float index);//将lua栈顶的对象储存在lua对象的一个索引位置，index是索引值


//类型检查
int            lua_isnil 		(lua_Object object);//是否为nil对象
int            lua_isnumber 		(lua_Object object);//是否为数字对象
int            lua_isstring 		(lua_Object object);//是否为字符串对象
int            lua_istable          	(lua_Object object);//是否为表对象
int            lua_iscfunction 		(lua_Object object);//是否为c函数对象
int            lua_isuserdata 		(lua_Object object);//是否为用户数据对象

#endif
