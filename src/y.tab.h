
typedef union  
{
 int   vInt;
 long  vLong;
 float vFloat;
 char *pChar;
 Word  vWord;
 Byte *pByte;
} YYSTYPE;//定义了一个联合体类型YYSTYPE，用于存储不同类型的值，如整数、长整数、浮点数、字符串指针、字等，这个联合体在语法分析过程中用于存储词法单元的值，根据不同的词法单元类型，可以使用不同的成员来访问存储的值
extern YYSTYPE yylval;//全局变量，yylval会被赋值为当前识别的词法单元的值，以供语法分析器使用
# define WRONGTOKEN 257

# define NIL 258
# define IF 259
# define THEN 260
# define ELSE 261
# define ELSEIF 262
# define WHILE 263
# define DO 264
# define REPEAT 265
# define UNTIL 266
# define END 267
# define RETURN 268
# define LOCAL 269
# define NUMBER 270
# define FUNCTION 271
# define STRING 272

# define NAME 273
# define DEBUG 274

# define AND 275
# define OR 276
# define NE 277
# define LE 278
# define GE 279
# define CONC 280//连接操作符，表示字符串连接，在Lua中使用两个点（..）来连接字符串，例如 "Hello" .. " " .. "World" 会得到 "Hello World"
# define UNARY 281//一元操作符，一元减号
# define NOT 282//逻辑非操作符，表示取反