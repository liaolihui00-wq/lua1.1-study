
/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 1 "lua.stx"


char *rcs_luastx = "$Id: lua.stx,v 2.4 1994/04/20 16:22:21 celes Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mm.h"

#include "opcode.h"
#include "hash.h"
#include "inout.h"
#include "table.h"
#include "lua.h"

#define LISTING 1 //是否生成汇编代码列表的标志，0表示不生成，1表示生成，以便在编译过程中输出详细的汇编代码信息，方便调试和分析Lua虚拟机的执行过程

#ifndef GAPCODE
#define GAPCODE 50//代码缓冲区的增长步长，表示当代码缓冲区满时，每次增加的字节数，以便在编译过程中动态扩展代码缓冲区的大小，避免内存溢出
#endif
static Word   maxcode;//代码缓冲区的最大容量
static Word   maxmain;//主代码的最大容量
static Word   maxcurr ;//当前代码的容量
static Byte  *code = NULL;//代码缓冲区的指针，指向存储生成的代码的内存区域
static Byte  *initcode;//初始代码的指针
static Byte  *basepc;//代码缓冲区的基地址
static Word   maincode;//主代码的起始位置
static Word   pc;//程序计数器

#define MAXVAR 32//变量缓冲区的最大容量，表示在编译过程中用于存储变量信息的缓冲区的大小
static long    varbuffer[MAXVAR];    //变量缓冲区的静态数组，用于在编译过程中存储变量信息，如局部变量、全局变量等
static int     nvarbuffer=0;	     /* number of variables at a list */

static Word    localvar[STACKGAP];  //变量表的静态数组，用于在编译过程中存储局部变量的名称
static int     nlocalvar=0;	     /* number of local variables */

#define MAXFIELDS FIELDS_PER_FLUSH*2
static Word    fields[MAXFIELDS];    //字段表的静态数组，用于在编译过程中存储构造表时需要刷新到内存中的字段名称
static int     nfields=0;
static int     ntemp;		    //临时变量的数量
static int     err;		     //错误标志，表示在编译过程中是否发生了错误，如果发生错误，编译器将设置这个标志以便后续处理




/* 声明加在 lua.stx 头部的 %{ 和 %} 之间 */
void PrintCode (Byte *base, Byte *end);
void yyerror (char *s);
int  yylex (void);

/* 下面这几个是报错最严重的，一定要加 */
void lua_pushvar (long n);
void lua_codeadjust (int n);
void lua_codestore (int n);
int  lua_localname (Word n);




/* Internal functions */

static void code_byte (Byte c)//内部函数：将一个字节c编码到代码缓冲区中
{
 if (pc>maxcurr-2)  /* 1 byte free to code HALT of main code *///代码缓冲区已经满了
 {
  maxcurr += GAPCODE;
  basepc = (Byte *)realloc(basepc, maxcurr*sizeof(Byte));//重新分配代码缓冲区的内存，新的大小为maxcurr字节
  if (basepc == NULL)
  {
   lua_error ("not enough memory");
   err = 1;
  }
 }
 basepc[pc++] = c;
}

static void code_word (Word n)//内部函数：将一个字n编码到代码缓冲区中
{
 CodeWord code;
 code.w = n;
 code_byte(code.m.c1);
 code_byte(code.m.c2);
}

static void code_float (float n)//内部函数：将一个浮点数n编码到代码缓冲区中
{
 CodeFloat code;
 code.f = n;
 code_byte(code.m.c1);
 code_byte(code.m.c2);
 code_byte(code.m.c3);
 code_byte(code.m.c4);
}

static void code_word_at (Byte *p, Word n)//内部函数：将一个字n编码到代码缓冲区的指定位置p
{
 CodeWord code;
 code.w = n;
 *p++ = code.m.c1;
 *p++ = code.m.c2;
}

static void push_field (Word name)//内部函数：将一个字段名称name推入字段表中
{
  if (nfields < STACKGAP-1)
    fields[nfields++] = name;
  else
  {
   lua_error ("too many fields in a constructor");//字段数量超过限制，报告错误并设置错误标志
   err = 1;
  }
}

static void flush_record (int n)//内部函数：将n个字段名称刷新到内存中
{
  int i;
  if (n == 0) return;
  code_byte(STORERECORD);
  code_byte(n);
  for (i=0; i<n; i++)
    code_word(fields[--nfields]);
  ntemp -= n;
}

static void flush_list (int m, int n)//内部函数：将n个元素存储到一个列表位置中
{
  if (n == 0) return;
  if (m == 0)
    code_byte(STORELIST0); 
  else
  {
    code_byte(STORELIST);
    code_byte(m);
  }
  code_byte(n);
  ntemp-=n;
}

static void incr_ntemp (void)//内部函数：增加临时变量的数量
{
 if (ntemp+nlocalvar+MAXVAR+1 < STACKGAP)
  ntemp++;
 else
 {
  lua_error ("stack overflow");
  err = 1;
 }
}

static void add_nlocalvar (int n)//内部函数：增加局部变量的数量
{
 if (ntemp+nlocalvar+MAXVAR+n < STACKGAP)
  nlocalvar += n;
 else
 {
  lua_error ("too many local variables or expression too complicate");
  err = 1;
 }
}

static void incr_nvarbuffer (void)//内部函数：增加变量缓冲区的数量
{
 if (nvarbuffer < MAXVAR-1)
  nvarbuffer++;
 else
 {
  lua_error ("variable buffer overflow");
  err = 1;
 }
}

static void code_number (float f)//内部函数：将一个数字f编码到代码缓冲区中
{ Word i = (Word)f;//将浮点数f转换为一个整数i
  if (f == (float)i)  /* f has an (short) integer value */
  {
   if (i <= 2) code_byte(PUSH0 + i);
   else if (i <= 255)
   {
    code_byte(PUSHBYTE);
    code_byte(i);
   }
   else
   {
    code_byte(PUSHWORD);
    code_word(i);
   }
  }
  else
  {
   code_byte(PUSHFLOAT);
   code_float(f);
  }
  incr_ntemp();
}


#line 269 "y.tab.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* Use api.header.include to #include this header
   instead of duplicating it here.  */
#ifndef YY_YY_Y_TAB_H_INCLUDED
# define YY_YY_Y_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    WRONGTOKEN = 258,              /* WRONGTOKEN  */
    NIL = 259,                     /* NIL  */
    IF = 260,                      /* IF  */
    THEN = 261,                    /* THEN  */
    ELSE = 262,                    /* ELSE  */
    ELSEIF = 263,                  /* ELSEIF  */
    WHILE = 264,                   /* WHILE  */
    DO = 265,                      /* DO  */
    REPEAT = 266,                  /* REPEAT  */
    UNTIL = 267,                   /* UNTIL  */
    END = 268,                     /* END  */
    RETURN = 269,                  /* RETURN  */
    LOCAL = 270,                   /* LOCAL  */
    NUMBER = 271,                  /* NUMBER  */
    FUNCTION = 272,                /* FUNCTION  */
    STRING = 273,                  /* STRING  */
    NAME = 274,                    /* NAME  */
    DEBUG = 275,                   /* DEBUG  */
    AND = 276,                     /* AND  */
    OR = 277,                      /* OR  */
    NE = 278,                      /* NE  */
    LE = 279,                      /* LE  */
    GE = 280,                      /* GE  */
    CONC = 281,                    /* CONC  */
    UNARY = 282,                   /* UNARY  */
    NOT = 283                      /* NOT  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif
/* Token kinds.  */
#define YYEMPTY -2
#define YYEOF 0
#define YYerror 256
#define YYUNDEF 257
#define WRONGTOKEN 258
#define NIL 259
#define IF 260
#define THEN 261
#define ELSE 262
#define ELSEIF 263
#define WHILE 264
#define DO 265
#define REPEAT 266
#define UNTIL 267
#define END 268
#define RETURN 269
#define LOCAL 270
#define NUMBER 271
#define FUNCTION 272
#define STRING 273
#define NAME 274
#define DEBUG 275
#define AND 276
#define OR 277
#define NE 278
#define LE 279
#define GE 280
#define CONC 281
#define UNARY 282
#define NOT 283

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 201 "lua.stx"

 int   vInt;
 long  vLong;
 float vFloat;
 char *pChar;
 Word  vWord;
 Byte *pByte;

#line 387 "y.tab.c"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_Y_TAB_H_INCLUDED  */
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_WRONGTOKEN = 3,                 /* WRONGTOKEN  */
  YYSYMBOL_NIL = 4,                        /* NIL  */
  YYSYMBOL_IF = 5,                         /* IF  */
  YYSYMBOL_THEN = 6,                       /* THEN  */
  YYSYMBOL_ELSE = 7,                       /* ELSE  */
  YYSYMBOL_ELSEIF = 8,                     /* ELSEIF  */
  YYSYMBOL_WHILE = 9,                      /* WHILE  */
  YYSYMBOL_DO = 10,                        /* DO  */
  YYSYMBOL_REPEAT = 11,                    /* REPEAT  */
  YYSYMBOL_UNTIL = 12,                     /* UNTIL  */
  YYSYMBOL_END = 13,                       /* END  */
  YYSYMBOL_RETURN = 14,                    /* RETURN  */
  YYSYMBOL_LOCAL = 15,                     /* LOCAL  */
  YYSYMBOL_NUMBER = 16,                    /* NUMBER  */
  YYSYMBOL_FUNCTION = 17,                  /* FUNCTION  */
  YYSYMBOL_STRING = 18,                    /* STRING  */
  YYSYMBOL_NAME = 19,                      /* NAME  */
  YYSYMBOL_DEBUG = 20,                     /* DEBUG  */
  YYSYMBOL_AND = 21,                       /* AND  */
  YYSYMBOL_OR = 22,                        /* OR  */
  YYSYMBOL_23_ = 23,                       /* '='  */
  YYSYMBOL_NE = 24,                        /* NE  */
  YYSYMBOL_25_ = 25,                       /* '>'  */
  YYSYMBOL_26_ = 26,                       /* '<'  */
  YYSYMBOL_LE = 27,                        /* LE  */
  YYSYMBOL_GE = 28,                        /* GE  */
  YYSYMBOL_CONC = 29,                      /* CONC  */
  YYSYMBOL_30_ = 30,                       /* '+'  */
  YYSYMBOL_31_ = 31,                       /* '-'  */
  YYSYMBOL_32_ = 32,                       /* '*'  */
  YYSYMBOL_33_ = 33,                       /* '/'  */
  YYSYMBOL_UNARY = 34,                     /* UNARY  */
  YYSYMBOL_NOT = 35,                       /* NOT  */
  YYSYMBOL_36_ = 36,                       /* '('  */
  YYSYMBOL_37_ = 37,                       /* ')'  */
  YYSYMBOL_38_ = 38,                       /* ';'  */
  YYSYMBOL_39_ = 39,                       /* '@'  */
  YYSYMBOL_40_ = 40,                       /* ','  */
  YYSYMBOL_41_ = 41,                       /* '{'  */
  YYSYMBOL_42_ = 42,                       /* '}'  */
  YYSYMBOL_43_ = 43,                       /* '['  */
  YYSYMBOL_44_ = 44,                       /* ']'  */
  YYSYMBOL_45_ = 45,                       /* '.'  */
  YYSYMBOL_YYACCEPT = 46,                  /* $accept  */
  YYSYMBOL_functionlist = 47,              /* functionlist  */
  YYSYMBOL_48_1 = 48,                      /* $@1  */
  YYSYMBOL_function = 49,                  /* function  */
  YYSYMBOL_50_2 = 50,                      /* @2  */
  YYSYMBOL_51_3 = 51,                      /* $@3  */
  YYSYMBOL_statlist = 52,                  /* statlist  */
  YYSYMBOL_stat = 53,                      /* stat  */
  YYSYMBOL_54_4 = 54,                      /* $@4  */
  YYSYMBOL_sc = 55,                        /* sc  */
  YYSYMBOL_stat1 = 56,                     /* stat1  */
  YYSYMBOL_57_5 = 57,                      /* @5  */
  YYSYMBOL_58_6 = 58,                      /* @6  */
  YYSYMBOL_elsepart = 59,                  /* elsepart  */
  YYSYMBOL_block = 60,                     /* block  */
  YYSYMBOL_61_7 = 61,                      /* @7  */
  YYSYMBOL_62_8 = 62,                      /* $@8  */
  YYSYMBOL_ret = 63,                       /* ret  */
  YYSYMBOL_64_9 = 64,                      /* $@9  */
  YYSYMBOL_PrepJump = 65,                  /* PrepJump  */
  YYSYMBOL_expr1 = 66,                     /* expr1  */
  YYSYMBOL_expr = 67,                      /* expr  */
  YYSYMBOL_68_10 = 68,                     /* $@10  */
  YYSYMBOL_69_11 = 69,                     /* $@11  */
  YYSYMBOL_typeconstructor = 70,           /* typeconstructor  */
  YYSYMBOL_71_12 = 71,                     /* @12  */
  YYSYMBOL_dimension = 72,                 /* dimension  */
  YYSYMBOL_functioncall = 73,              /* functioncall  */
  YYSYMBOL_74_13 = 74,                     /* @13  */
  YYSYMBOL_functionvalue = 75,             /* functionvalue  */
  YYSYMBOL_exprlist = 76,                  /* exprlist  */
  YYSYMBOL_exprlist1 = 77,                 /* exprlist1  */
  YYSYMBOL_78_14 = 78,                     /* $@14  */
  YYSYMBOL_parlist = 79,                   /* parlist  */
  YYSYMBOL_parlist1 = 80,                  /* parlist1  */
  YYSYMBOL_objectname = 81,                /* objectname  */
  YYSYMBOL_fieldlist = 82,                 /* fieldlist  */
  YYSYMBOL_ffieldlist = 83,                /* ffieldlist  */
  YYSYMBOL_ffieldlist1 = 84,               /* ffieldlist1  */
  YYSYMBOL_ffield = 85,                    /* ffield  */
  YYSYMBOL_86_15 = 86,                     /* @15  */
  YYSYMBOL_lfieldlist = 87,                /* lfieldlist  */
  YYSYMBOL_lfieldlist1 = 88,               /* lfieldlist1  */
  YYSYMBOL_varlist1 = 89,                  /* varlist1  */
  YYSYMBOL_var = 90,                       /* var  */
  YYSYMBOL_91_16 = 91,                     /* $@16  */
  YYSYMBOL_92_17 = 92,                     /* $@17  */
  YYSYMBOL_localdeclist = 93,              /* localdeclist  */
  YYSYMBOL_decinit = 94,                   /* decinit  */
  YYSYMBOL_setdebug = 95                   /* setdebug  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_uint8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   249

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  46
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  50
/* YYNRULES -- Number of rules.  */
#define YYNRULES  103
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  175

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   283


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      36,    37,    32,    30,    40,    31,    45,    33,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    38,
      26,    23,    25,     2,    39,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    43,     2,    44,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    41,     2,    42,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    24,    27,
      28,    29,    34,    35
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   241,   241,   243,   242,   251,   252,   256,   272,   255,
     301,   302,   305,   305,   314,   314,   317,   336,   336,   346,
     346,   354,   366,   367,   368,   371,   372,   373,   393,   393,
     393,   403,   404,   404,   413,   419,   422,   423,   424,   425,
     426,   427,   428,   429,   430,   431,   432,   433,   434,   435,
     436,   437,   442,   443,   444,   451,   452,   460,   461,   461,
     467,   467,   476,   475,   507,   508,   511,   511,   514,   517,
     518,   521,   522,   522,   526,   527,   530,   535,   542,   543,
     546,   551,   558,   559,   562,   563,   570,   570,   576,   577,
     580,   581,   589,   595,   602,   612,   612,   616,   616,   624,
     625,   632,   633,   636
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "WRONGTOKEN", "NIL",
  "IF", "THEN", "ELSE", "ELSEIF", "WHILE", "DO", "REPEAT", "UNTIL", "END",
  "RETURN", "LOCAL", "NUMBER", "FUNCTION", "STRING", "NAME", "DEBUG",
  "AND", "OR", "'='", "NE", "'>'", "'<'", "LE", "GE", "CONC", "'+'", "'-'",
  "'*'", "'/'", "UNARY", "NOT", "'('", "')'", "';'", "'@'", "','", "'{'",
  "'}'", "'['", "']'", "'.'", "$accept", "functionlist", "$@1", "function",
  "@2", "$@3", "statlist", "stat", "$@4", "sc", "stat1", "@5", "@6",
  "elsepart", "block", "@7", "$@8", "ret", "$@9", "PrepJump", "expr1",
  "expr", "$@10", "$@11", "typeconstructor", "@12", "dimension",
  "functioncall", "@13", "functionvalue", "exprlist", "exprlist1", "$@14",
  "parlist", "parlist1", "objectname", "fieldlist", "ffieldlist",
  "ffieldlist1", "ffield", "@15", "lfieldlist", "lfieldlist1", "varlist1",
  "var", "$@16", "$@17", "localdeclist", "decinit", "setdebug", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-127)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-98)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    -127,     5,  -127,    20,  -127,  -127,  -127,  -127,  -127,   -26,
      27,     7,  -127,  -127,    43,  -127,  -127,    26,  -127,  -127,
    -127,  -127,  -127,  -127,   -17,   -32,    44,  -127,  -127,  -127,
      43,    43,    43,    43,    31,   127,  -127,  -127,  -127,   -32,
      43,  -127,  -127,   -13,    49,    34,    43,    53,   -19,    30,
    -127,    40,    45,  -127,  -127,  -127,   141,    50,    43,  -127,
    -127,  -127,    43,    43,    43,    43,    43,    43,    43,    43,
      43,    43,    43,   179,    79,  -127,    43,    67,  -127,  -127,
     -10,    43,   216,    52,   -15,    43,    75,  -127,    77,  -127,
     141,    58,  -127,  -127,  -127,    70,    70,    70,    70,    70,
      70,    74,   -24,   -24,  -127,  -127,  -127,    43,    76,    52,
    -127,    89,    43,  -127,    85,    52,  -127,   192,  -127,  -127,
    -127,  -127,  -127,    43,    43,  -127,   141,   -26,   110,  -127,
      86,    87,  -127,   141,    90,    91,  -127,    43,  -127,   116,
      12,    88,    88,  -127,  -127,  -127,  -127,   118,   107,  -127,
      89,  -127,    43,   216,  -127,  -127,    43,   122,   123,    43,
      43,  -127,   141,  -127,   155,  -127,  -127,   -26,   141,  -127,
    -127,  -127,  -127,    12,  -127
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       2,     3,     1,     0,   103,    12,     5,     6,     7,    14,
       0,     0,    15,     4,     0,    17,    19,     0,    94,    62,
      13,    23,    22,    66,     0,    92,    74,    55,    53,    54,
       0,     0,     0,     0,    62,     0,    35,    50,    56,    52,
       0,    28,    99,   101,    78,     0,     0,     0,     0,     0,
      76,     0,    75,    48,    49,    57,     0,    35,    64,    34,
      34,    34,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    10,     0,     0,    24,    79,
       0,    69,    71,    21,    93,     0,     0,     8,     0,    36,
      65,     0,    28,    58,    60,    37,    40,    39,    38,    41,
      42,    47,    43,    44,    45,    46,    34,     0,    12,   102,
     100,    82,    88,    63,     0,    70,    72,     0,    98,    28,
      77,    51,    34,     0,     0,    28,    34,    14,    31,    86,
       0,    83,    84,    90,     0,    89,    67,     0,    96,     0,
      25,    59,    61,    34,    20,    11,    30,     0,     0,    80,
       0,    81,     0,    73,     9,    28,     0,     0,     0,    69,
       0,    85,    91,    26,     0,    16,    18,    14,    87,    34,
      33,    28,    34,    25,    27
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -127,  -127,  -127,  -127,  -127,  -127,  -127,    29,  -127,  -126,
    -127,  -127,  -127,   -34,   -90,  -127,  -127,  -127,  -127,   -46,
     -14,   -12,  -127,  -127,   130,  -127,  -127,   131,  -127,  -127,
     -16,   -39,  -127,  -127,  -127,  -127,  -127,  -127,  -127,    -6,
    -127,  -127,  -127,  -127,    -7,  -127,  -127,  -127,  -127,  -127
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,     1,     5,     6,    11,   119,   108,     9,    10,    13,
      20,    40,    41,   157,    74,    75,   128,   146,   147,    92,
      56,    36,   123,   124,    37,    44,    91,    38,    45,    23,
     114,   115,   137,    51,    52,    80,   113,   130,   131,   132,
     148,   134,   135,    24,    39,    48,    49,    43,    78,     7
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      35,   145,   122,    25,   -68,     2,    46,    83,    71,    72,
      76,   -95,    12,   -97,    93,    94,    53,    54,    55,   155,
     156,    57,     3,    47,    85,     4,    73,    77,   -95,   139,
     -97,   111,    14,   112,    82,   143,    15,   109,    16,     8,
      84,   170,    17,    26,    90,    42,    18,    27,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,    28,
     125,    29,    18,    50,    82,   163,    19,    58,    79,    82,
      81,   117,    18,    30,    31,    86,   140,    87,    32,    33,
     144,   172,    34,   -29,   -29,    88,   110,    89,   -29,   -29,
     -29,   107,   116,   126,   118,   121,   120,   158,   133,    68,
      69,    70,    71,    72,    69,    70,    71,    72,   129,   141,
     142,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,   136,   171,   -32,   153,   173,   150,   149,   154,
     160,   152,   159,    59,   151,   165,   166,   127,   162,   174,
      21,    22,   164,   167,   161,     0,   168,    82,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,   169,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,     0,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,   106,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   138,   -35,   -35,   -35,
     -35,   -35,   -35,   -35,   -35,   -35,   -35,   -35,   -35,   -35
};

static const yytype_int16 yycheck[] =
{
      14,   127,    92,    10,    36,     0,    23,    46,    32,    33,
      23,    43,    38,    45,    60,    61,    30,    31,    32,     7,
       8,    33,    17,    40,    43,    20,    40,    40,    43,   119,
      45,    41,     5,    43,    46,   125,     9,    76,    11,    19,
      47,   167,    15,    36,    58,    19,    19,     4,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    16,
     106,    18,    19,    19,    76,   155,    39,    36,    19,    81,
      36,    85,    19,    30,    31,    45,   122,    37,    35,    36,
     126,   171,    39,     7,     8,    40,    19,    37,    12,    13,
      14,    12,    40,   107,    19,    37,    19,   143,   112,    29,
      30,    31,    32,    33,    30,    31,    32,    33,    19,   123,
     124,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    37,   169,    14,   137,   172,    40,    42,    13,
      23,    40,    14,     6,    44,    13,    13,   108,   152,   173,
      10,    10,   156,   159,   150,    -1,   160,   159,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,     6,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    -1,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    10,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    44,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,    47,     0,    17,    20,    48,    49,    95,    19,    53,
      54,    50,    38,    55,     5,     9,    11,    15,    19,    39,
      56,    70,    73,    75,    89,    90,    36,     4,    16,    18,
      30,    31,    35,    36,    39,    66,    67,    70,    73,    90,
      57,    58,    19,    93,    71,    74,    23,    40,    91,    92,
      19,    79,    80,    66,    66,    66,    66,    67,    36,     6,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    66,    60,    61,    23,    40,    94,    19,
      81,    36,    67,    77,    90,    43,    45,    37,    40,    37,
      66,    72,    65,    65,    65,    66,    66,    66,    66,    66,
      66,    66,    66,    66,    66,    66,    10,    12,    52,    77,
      19,    41,    43,    82,    76,    77,    40,    66,    19,    51,
      19,    37,    60,    68,    69,    65,    66,    53,    62,    19,
      83,    84,    85,    66,    87,    88,    37,    78,    44,    60,
      65,    66,    66,    60,    65,    55,    63,    64,    86,    42,
      40,    44,    40,    67,    13,     7,     8,    59,    65,    14,
      23,    85,    66,    60,    66,    13,    13,    76,    66,     6,
      55,    65,    60,    65,    59
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    46,    47,    48,    47,    47,    47,    50,    51,    49,
      52,    52,    54,    53,    55,    55,    56,    57,    56,    58,
      56,    56,    56,    56,    56,    59,    59,    59,    61,    62,
      60,    63,    64,    63,    65,    66,    67,    67,    67,    67,
      67,    67,    67,    67,    67,    67,    67,    67,    67,    67,
      67,    67,    67,    67,    67,    67,    67,    67,    68,    67,
      69,    67,    71,    70,    72,    72,    74,    73,    75,    76,
      76,    77,    78,    77,    79,    79,    80,    80,    81,    81,
      82,    82,    83,    83,    84,    84,    86,    85,    87,    87,
      88,    88,    89,    89,    90,    91,    90,    92,    90,    93,
      93,    94,    94,    95
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     0,     4,     2,     2,     0,     0,     9,
       0,     3,     0,     2,     0,     1,     8,     0,     8,     0,
       6,     3,     1,     1,     3,     0,     2,     7,     0,     0,
       4,     0,     0,     4,     0,     1,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     2,     2,
       1,     4,     1,     1,     1,     1,     1,     2,     0,     5,
       0,     5,     0,     4,     0,     1,     0,     5,     1,     0,
       1,     1,     0,     4,     0,     1,     1,     3,     0,     1,
       3,     3,     0,     1,     1,     3,     0,     4,     0,     1,
       1,     3,     1,     3,     1,     0,     5,     0,     4,     1,
       3,     0,     2,     1
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 3: /* $@1: %empty  */
#line 243 "lua.stx"
                {
	  	  pc=maincode; basepc=initcode; maxcurr=maxmain;
		  nlocalvar=0;
	        }
#line 1601 "y.tab.c"
    break;

  case 4: /* functionlist: functionlist $@1 stat sc  */
#line 248 "lua.stx"
                {
		  maincode=pc; initcode=basepc; maxmain=maxcurr;
		}
#line 1609 "y.tab.c"
    break;

  case 7: /* @2: %empty  */
#line 256 "lua.stx"
               {
		if (code == NULL)	/* first function */
		{
		 code = (Byte *) calloc(GAPCODE, sizeof(Byte));
		 if (code == NULL)
		 {
		  lua_error("not enough memory");
		  err = 1;
		 }
		 maxcode = GAPCODE;
		}
		pc=0; basepc=code; maxcurr=maxcode; 
		nlocalvar=0;
		(yyval.vWord) = lua_findsymbol((yyvsp[0].pChar)); 
	       }
#line 1629 "y.tab.c"
    break;

  case 8: /* $@3: %empty  */
#line 272 "lua.stx"
               {
	        if (lua_debug)
		{
	         code_byte(SETFUNCTION); 
                 code_word(lua_nfile-1);
		 code_word((yyvsp[-3].vWord));
		}
	        lua_codeadjust (0);
	       }
#line 1643 "y.tab.c"
    break;

  case 9: /* function: FUNCTION NAME @2 '(' parlist ')' $@3 block END  */
#line 283 "lua.stx"
               { 
                if (lua_debug) code_byte(RESET); 
	        code_byte(RETCODE); code_byte(nlocalvar);
	        s_tag((yyvsp[-6].vWord)) = T_FUNCTION;
	        s_bvalue((yyvsp[-6].vWord)) = calloc (pc, sizeof(Byte));
		if (s_bvalue((yyvsp[-6].vWord)) == NULL)
		{
		 lua_error("not enough memory");
		 err = 1;
		}
	        memcpy (s_bvalue((yyvsp[-6].vWord)), basepc, pc*sizeof(Byte));
		code = basepc; maxcode=maxcurr;
#if LISTING
PrintCode(code,code+pc);
#endif
	       }
#line 1664 "y.tab.c"
    break;

  case 12: /* $@4: %empty  */
#line 305 "lua.stx"
           {
            ntemp = 0; 
            if (lua_debug)
            {
             code_byte(SETLINE); code_word(lua_linenumber);
            }
	   }
#line 1676 "y.tab.c"
    break;

  case 16: /* stat1: IF expr1 THEN PrepJump block PrepJump elsepart END  */
#line 318 "lua.stx"
       {
        {
	 Word elseinit = (yyvsp[-2].vWord)+sizeof(Word)+1;
	 if (pc - elseinit == 0)		/* no else */
	 {
	  pc -= sizeof(Word)+1;
	  elseinit = pc;
	 }
	 else
	 {
	  basepc[(yyvsp[-2].vWord)] = JMP;
	  code_word_at(basepc+(yyvsp[-2].vWord)+1, pc - elseinit);
	 }
	 basepc[(yyvsp[-4].vWord)] = IFFJMP;
	 code_word_at(basepc+(yyvsp[-4].vWord)+1,elseinit-((yyvsp[-4].vWord)+sizeof(Word)+1));
	}
       }
#line 1698 "y.tab.c"
    break;

  case 17: /* @5: %empty  */
#line 336 "lua.stx"
               {(yyval.vWord)=pc;}
#line 1704 "y.tab.c"
    break;

  case 18: /* stat1: WHILE @5 expr1 DO PrepJump block PrepJump END  */
#line 338 "lua.stx"
       {
        basepc[(yyvsp[-3].vWord)] = IFFJMP;
	code_word_at(basepc+(yyvsp[-3].vWord)+1, pc - ((yyvsp[-3].vWord) + sizeof(Word)+1));
        
        basepc[(yyvsp[-1].vWord)] = UPJMP;
	code_word_at(basepc+(yyvsp[-1].vWord)+1, pc - ((yyvsp[-6].vWord)));
       }
#line 1716 "y.tab.c"
    break;

  case 19: /* @6: %empty  */
#line 346 "lua.stx"
                {(yyval.vWord)=pc;}
#line 1722 "y.tab.c"
    break;

  case 20: /* stat1: REPEAT @6 block UNTIL expr1 PrepJump  */
#line 348 "lua.stx"
       {
        basepc[(yyvsp[0].vWord)] = IFFUPJMP;
	code_word_at(basepc+(yyvsp[0].vWord)+1, pc - ((yyvsp[-4].vWord)));
       }
#line 1731 "y.tab.c"
    break;

  case 21: /* stat1: varlist1 '=' exprlist1  */
#line 355 "lua.stx"
       {
        {
         int i;
         if ((yyvsp[0].vInt) == 0 || nvarbuffer != ntemp - (yyvsp[-2].vInt) * 2)
	  lua_codeadjust ((yyvsp[-2].vInt) * 2 + nvarbuffer);
	 for (i=nvarbuffer-1; i>=0; i--)
	  lua_codestore (i);
	 if ((yyvsp[-2].vInt) > 1 || ((yyvsp[-2].vInt) == 1 && varbuffer[0] != 0))
	  lua_codeadjust (0);
	}
       }
#line 1747 "y.tab.c"
    break;

  case 22: /* stat1: functioncall  */
#line 366 "lua.stx"
                                        { lua_codeadjust (0); }
#line 1753 "y.tab.c"
    break;

  case 23: /* stat1: typeconstructor  */
#line 367 "lua.stx"
                                        { lua_codeadjust (0); }
#line 1759 "y.tab.c"
    break;

  case 24: /* stat1: LOCAL localdeclist decinit  */
#line 368 "lua.stx"
                                      { add_nlocalvar((yyvsp[-1].vInt)); lua_codeadjust (0); }
#line 1765 "y.tab.c"
    break;

  case 27: /* elsepart: ELSEIF expr1 THEN PrepJump block PrepJump elsepart  */
#line 374 "lua.stx"
         {
          {
  	   Word elseinit = (yyvsp[-1].vWord)+sizeof(Word)+1;
  	   if (pc - elseinit == 0)		/* no else */
  	   {
  	    pc -= sizeof(Word)+1;
	    elseinit = pc;
	   }
	   else
	   {
	    basepc[(yyvsp[-1].vWord)] = JMP;
	    code_word_at(basepc+(yyvsp[-1].vWord)+1, pc - elseinit);
	   }
	   basepc[(yyvsp[-3].vWord)] = IFFJMP;
	   code_word_at(basepc+(yyvsp[-3].vWord)+1, elseinit - ((yyvsp[-3].vWord) + sizeof(Word)+1));
	  }  
         }
#line 1787 "y.tab.c"
    break;

  case 28: /* @7: %empty  */
#line 393 "lua.stx"
           {(yyval.vInt) = nlocalvar;}
#line 1793 "y.tab.c"
    break;

  case 29: /* $@8: %empty  */
#line 393 "lua.stx"
                                            {ntemp = 0;}
#line 1799 "y.tab.c"
    break;

  case 30: /* block: @7 statlist $@8 ret  */
#line 394 "lua.stx"
         {
	  if (nlocalvar != (yyvsp[-3].vInt))
	  {
           nlocalvar = (yyvsp[-3].vInt);
	   lua_codeadjust (0);
	  }
         }
#line 1811 "y.tab.c"
    break;

  case 32: /* $@9: %empty  */
#line 404 "lua.stx"
          { if (lua_debug){code_byte(SETLINE);code_word(lua_linenumber);}}
#line 1817 "y.tab.c"
    break;

  case 33: /* ret: $@9 RETURN exprlist sc  */
#line 406 "lua.stx"
          { 
           if (lua_debug) code_byte(RESET); 
           code_byte(RETCODE); code_byte(nlocalvar);
          }
#line 1826 "y.tab.c"
    break;

  case 34: /* PrepJump: %empty  */
#line 413 "lua.stx"
         { 
	  (yyval.vWord) = pc;
	  code_byte(0);		/* open space */
	  code_word (0);
         }
#line 1836 "y.tab.c"
    break;

  case 35: /* expr1: expr  */
#line 419 "lua.stx"
                { if ((yyvsp[0].vInt) == 0) {lua_codeadjust (ntemp+1); incr_ntemp();}}
#line 1842 "y.tab.c"
    break;

  case 36: /* expr: '(' expr ')'  */
#line 422 "lua.stx"
                        { (yyval.vInt) = (yyvsp[-1].vInt); }
#line 1848 "y.tab.c"
    break;

  case 37: /* expr: expr1 '=' expr1  */
#line 423 "lua.stx"
                        { code_byte(EQOP);   (yyval.vInt) = 1; ntemp--;}
#line 1854 "y.tab.c"
    break;

  case 38: /* expr: expr1 '<' expr1  */
#line 424 "lua.stx"
                        { code_byte(LTOP);   (yyval.vInt) = 1; ntemp--;}
#line 1860 "y.tab.c"
    break;

  case 39: /* expr: expr1 '>' expr1  */
#line 425 "lua.stx"
                        { code_byte(LEOP); code_byte(NOTOP); (yyval.vInt) = 1; ntemp--;}
#line 1866 "y.tab.c"
    break;

  case 40: /* expr: expr1 NE expr1  */
#line 426 "lua.stx"
                        { code_byte(EQOP); code_byte(NOTOP); (yyval.vInt) = 1; ntemp--;}
#line 1872 "y.tab.c"
    break;

  case 41: /* expr: expr1 LE expr1  */
#line 427 "lua.stx"
                        { code_byte(LEOP);   (yyval.vInt) = 1; ntemp--;}
#line 1878 "y.tab.c"
    break;

  case 42: /* expr: expr1 GE expr1  */
#line 428 "lua.stx"
                        { code_byte(LTOP); code_byte(NOTOP); (yyval.vInt) = 1; ntemp--;}
#line 1884 "y.tab.c"
    break;

  case 43: /* expr: expr1 '+' expr1  */
#line 429 "lua.stx"
                        { code_byte(ADDOP);  (yyval.vInt) = 1; ntemp--;}
#line 1890 "y.tab.c"
    break;

  case 44: /* expr: expr1 '-' expr1  */
#line 430 "lua.stx"
                        { code_byte(SUBOP);  (yyval.vInt) = 1; ntemp--;}
#line 1896 "y.tab.c"
    break;

  case 45: /* expr: expr1 '*' expr1  */
#line 431 "lua.stx"
                        { code_byte(MULTOP); (yyval.vInt) = 1; ntemp--;}
#line 1902 "y.tab.c"
    break;

  case 46: /* expr: expr1 '/' expr1  */
#line 432 "lua.stx"
                        { code_byte(DIVOP);  (yyval.vInt) = 1; ntemp--;}
#line 1908 "y.tab.c"
    break;

  case 47: /* expr: expr1 CONC expr1  */
#line 433 "lua.stx"
                         { code_byte(CONCOP);  (yyval.vInt) = 1; ntemp--;}
#line 1914 "y.tab.c"
    break;

  case 48: /* expr: '+' expr1  */
#line 434 "lua.stx"
                                { (yyval.vInt) = 1; }
#line 1920 "y.tab.c"
    break;

  case 49: /* expr: '-' expr1  */
#line 435 "lua.stx"
                                { code_byte(MINUSOP); (yyval.vInt) = 1;}
#line 1926 "y.tab.c"
    break;

  case 50: /* expr: typeconstructor  */
#line 436 "lua.stx"
                       { (yyval.vInt) = (yyvsp[0].vInt); }
#line 1932 "y.tab.c"
    break;

  case 51: /* expr: '@' '(' dimension ')'  */
#line 438 "lua.stx"
     { 
      code_byte(CREATEARRAY);
      (yyval.vInt) = 1;
     }
#line 1941 "y.tab.c"
    break;

  case 52: /* expr: var  */
#line 442 "lua.stx"
                        { lua_pushvar ((yyvsp[0].vLong)); (yyval.vInt) = 1;}
#line 1947 "y.tab.c"
    break;

  case 53: /* expr: NUMBER  */
#line 443 "lua.stx"
                        { code_number((yyvsp[0].vFloat)); (yyval.vInt) = 1; }
#line 1953 "y.tab.c"
    break;

  case 54: /* expr: STRING  */
#line 445 "lua.stx"
     {
      code_byte(PUSHSTRING);
      code_word((yyvsp[0].vWord));
      (yyval.vInt) = 1;
      incr_ntemp();
     }
#line 1964 "y.tab.c"
    break;

  case 55: /* expr: NIL  */
#line 451 "lua.stx"
                        {code_byte(PUSHNIL); (yyval.vInt) = 1; incr_ntemp();}
#line 1970 "y.tab.c"
    break;

  case 56: /* expr: functioncall  */
#line 453 "lua.stx"
     {
      (yyval.vInt) = 0;
      if (lua_debug)
      {
       code_byte(SETLINE); code_word(lua_linenumber);
      }
     }
#line 1982 "y.tab.c"
    break;

  case 57: /* expr: NOT expr1  */
#line 460 "lua.stx"
                        { code_byte(NOTOP);  (yyval.vInt) = 1;}
#line 1988 "y.tab.c"
    break;

  case 58: /* $@10: %empty  */
#line 461 "lua.stx"
                           {code_byte(POP); ntemp--;}
#line 1994 "y.tab.c"
    break;

  case 59: /* expr: expr1 AND PrepJump $@10 expr1  */
#line 462 "lua.stx"
     { 
      basepc[(yyvsp[-2].vWord)] = ONFJMP;
      code_word_at(basepc+(yyvsp[-2].vWord)+1, pc - ((yyvsp[-2].vWord) + sizeof(Word)+1));
      (yyval.vInt) = 1;
     }
#line 2004 "y.tab.c"
    break;

  case 60: /* $@11: %empty  */
#line 467 "lua.stx"
                          {code_byte(POP); ntemp--;}
#line 2010 "y.tab.c"
    break;

  case 61: /* expr: expr1 OR PrepJump $@11 expr1  */
#line 468 "lua.stx"
     { 
      basepc[(yyvsp[-2].vWord)] = ONTJMP;
      code_word_at(basepc+(yyvsp[-2].vWord)+1, pc - ((yyvsp[-2].vWord) + sizeof(Word)+1));
      (yyval.vInt) = 1;
     }
#line 2020 "y.tab.c"
    break;

  case 62: /* @12: %empty  */
#line 476 "lua.stx"
     {
      code_byte(PUSHBYTE);
      (yyval.vWord) = pc; code_byte(0);
      incr_ntemp();
      code_byte(CREATEARRAY);
     }
#line 2031 "y.tab.c"
    break;

  case 63: /* typeconstructor: '@' @12 objectname fieldlist  */
#line 483 "lua.stx"
     {
      basepc[(yyvsp[-2].vWord)] = (yyvsp[0].vInt); 
      if ((yyvsp[-1].vLong) < 0)	/* there is no function to be called */
      {
       (yyval.vInt) = 1;
      }
      else
      {
       lua_pushvar ((yyvsp[-1].vLong)+1);
       code_byte(PUSHMARK);
       incr_ntemp();
       code_byte(PUSHOBJECT);
       incr_ntemp();
       code_byte(CALLFUNC); 
       ntemp -= 4;
       (yyval.vInt) = 0;
       if (lua_debug)
       {
        code_byte(SETLINE); code_word(lua_linenumber);
       }
      }
     }
#line 2058 "y.tab.c"
    break;

  case 64: /* dimension: %empty  */
#line 507 "lua.stx"
                                { code_byte(PUSHNIL); incr_ntemp();}
#line 2064 "y.tab.c"
    break;

  case 66: /* @13: %empty  */
#line 511 "lua.stx"
                              {code_byte(PUSHMARK); (yyval.vInt) = ntemp; incr_ntemp();}
#line 2070 "y.tab.c"
    break;

  case 67: /* functioncall: functionvalue @13 '(' exprlist ')'  */
#line 512 "lua.stx"
                                 { code_byte(CALLFUNC); ntemp = (yyvsp[-3].vInt)-1;}
#line 2076 "y.tab.c"
    break;

  case 68: /* functionvalue: var  */
#line 514 "lua.stx"
                    {lua_pushvar ((yyvsp[0].vLong)); }
#line 2082 "y.tab.c"
    break;

  case 69: /* exprlist: %empty  */
#line 517 "lua.stx"
                                        { (yyval.vInt) = 1; }
#line 2088 "y.tab.c"
    break;

  case 70: /* exprlist: exprlist1  */
#line 518 "lua.stx"
                                        { (yyval.vInt) = (yyvsp[0].vInt); }
#line 2094 "y.tab.c"
    break;

  case 71: /* exprlist1: expr  */
#line 521 "lua.stx"
                                        { (yyval.vInt) = (yyvsp[0].vInt); }
#line 2100 "y.tab.c"
    break;

  case 72: /* $@14: %empty  */
#line 522 "lua.stx"
                              {if (!(yyvsp[-1].vInt)){lua_codeadjust (ntemp+1); incr_ntemp();}}
#line 2106 "y.tab.c"
    break;

  case 73: /* exprlist1: exprlist1 ',' $@14 expr  */
#line 523 "lua.stx"
                      {(yyval.vInt) = (yyvsp[0].vInt);}
#line 2112 "y.tab.c"
    break;

  case 76: /* parlist1: NAME  */
#line 531 "lua.stx"
                {
		 localvar[nlocalvar]=lua_findsymbol((yyvsp[0].pChar)); 
		 add_nlocalvar(1);
		}
#line 2121 "y.tab.c"
    break;

  case 77: /* parlist1: parlist1 ',' NAME  */
#line 536 "lua.stx"
                {
		 localvar[nlocalvar]=lua_findsymbol((yyvsp[0].pChar)); 
		 add_nlocalvar(1);
		}
#line 2130 "y.tab.c"
    break;

  case 78: /* objectname: %empty  */
#line 542 "lua.stx"
                                {(yyval.vLong)=-1;}
#line 2136 "y.tab.c"
    break;

  case 79: /* objectname: NAME  */
#line 543 "lua.stx"
                                {(yyval.vLong)=lua_findsymbol((yyvsp[0].pChar));}
#line 2142 "y.tab.c"
    break;

  case 80: /* fieldlist: '{' ffieldlist '}'  */
#line 547 "lua.stx"
              { 
	       flush_record((yyvsp[-1].vInt)%FIELDS_PER_FLUSH); 
	       (yyval.vInt) = (yyvsp[-1].vInt);
	      }
#line 2151 "y.tab.c"
    break;

  case 81: /* fieldlist: '[' lfieldlist ']'  */
#line 552 "lua.stx"
              { 
	       flush_list((yyvsp[-1].vInt)/FIELDS_PER_FLUSH, (yyvsp[-1].vInt)%FIELDS_PER_FLUSH);
	       (yyval.vInt) = (yyvsp[-1].vInt);
     	      }
#line 2160 "y.tab.c"
    break;

  case 82: /* ffieldlist: %empty  */
#line 558 "lua.stx"
                           { (yyval.vInt) = 0; }
#line 2166 "y.tab.c"
    break;

  case 83: /* ffieldlist: ffieldlist1  */
#line 559 "lua.stx"
                           { (yyval.vInt) = (yyvsp[0].vInt); }
#line 2172 "y.tab.c"
    break;

  case 84: /* ffieldlist1: ffield  */
#line 562 "lua.stx"
                                        {(yyval.vInt)=1;}
#line 2178 "y.tab.c"
    break;

  case 85: /* ffieldlist1: ffieldlist1 ',' ffield  */
#line 564 "lua.stx"
                {
		  (yyval.vInt)=(yyvsp[-2].vInt)+1;
		  if ((yyval.vInt)%FIELDS_PER_FLUSH == 0) flush_record(FIELDS_PER_FLUSH);
		}
#line 2187 "y.tab.c"
    break;

  case 86: /* @15: %empty  */
#line 570 "lua.stx"
                   {(yyval.vWord) = lua_findconstant((yyvsp[0].pChar));}
#line 2193 "y.tab.c"
    break;

  case 87: /* ffield: NAME @15 '=' expr1  */
#line 571 "lua.stx"
              { 
	       push_field((yyvsp[-2].vWord));
	      }
#line 2201 "y.tab.c"
    break;

  case 88: /* lfieldlist: %empty  */
#line 576 "lua.stx"
                           { (yyval.vInt) = 0; }
#line 2207 "y.tab.c"
    break;

  case 89: /* lfieldlist: lfieldlist1  */
#line 577 "lua.stx"
                           { (yyval.vInt) = (yyvsp[0].vInt); }
#line 2213 "y.tab.c"
    break;

  case 90: /* lfieldlist1: expr1  */
#line 580 "lua.stx"
                     {(yyval.vInt)=1;}
#line 2219 "y.tab.c"
    break;

  case 91: /* lfieldlist1: lfieldlist1 ',' expr1  */
#line 582 "lua.stx"
                {
		  (yyval.vInt)=(yyvsp[-2].vInt)+1;
		  if ((yyval.vInt)%FIELDS_PER_FLUSH == 0) 
		    flush_list((yyval.vInt)/FIELDS_PER_FLUSH - 1, FIELDS_PER_FLUSH);
		}
#line 2229 "y.tab.c"
    break;

  case 92: /* varlist1: var  */
#line 590 "lua.stx"
          {
	   nvarbuffer = 0; 
           varbuffer[nvarbuffer] = (yyvsp[0].vLong); incr_nvarbuffer();
	   (yyval.vInt) = ((yyvsp[0].vLong) == 0) ? 1 : 0;
	  }
#line 2239 "y.tab.c"
    break;

  case 93: /* varlist1: varlist1 ',' var  */
#line 596 "lua.stx"
          { 
           varbuffer[nvarbuffer] = (yyvsp[0].vLong); incr_nvarbuffer();
	   (yyval.vInt) = ((yyvsp[0].vLong) == 0) ? (yyvsp[-2].vInt) + 1 : (yyvsp[-2].vInt);
	  }
#line 2248 "y.tab.c"
    break;

  case 94: /* var: NAME  */
#line 603 "lua.stx"
          {
	   Word s = lua_findsymbol((yyvsp[0].pChar));
	   int local = lua_localname (s);
	   if (local == -1)	/* global var */
	    (yyval.vLong) = s + 1;		/* return positive value */
           else
	    (yyval.vLong) = -(local+1);		/* return negative value */
	  }
#line 2261 "y.tab.c"
    break;

  case 95: /* $@16: %empty  */
#line 612 "lua.stx"
                    {lua_pushvar ((yyvsp[0].vLong));}
#line 2267 "y.tab.c"
    break;

  case 96: /* var: var $@16 '[' expr1 ']'  */
#line 613 "lua.stx"
          {
	   (yyval.vLong) = 0;		/* indexed variable */
	  }
#line 2275 "y.tab.c"
    break;

  case 97: /* $@17: %empty  */
#line 616 "lua.stx"
                    {lua_pushvar ((yyvsp[0].vLong));}
#line 2281 "y.tab.c"
    break;

  case 98: /* var: var $@17 '.' NAME  */
#line 617 "lua.stx"
          {
	   code_byte(PUSHSTRING);
	   code_word(lua_findconstant((yyvsp[0].pChar))); incr_ntemp();
	   (yyval.vLong) = 0;		/* indexed variable */
	  }
#line 2291 "y.tab.c"
    break;

  case 99: /* localdeclist: NAME  */
#line 624 "lua.stx"
                     {localvar[nlocalvar]=lua_findsymbol((yyvsp[0].pChar)); (yyval.vInt) = 1;}
#line 2297 "y.tab.c"
    break;

  case 100: /* localdeclist: localdeclist ',' NAME  */
#line 626 "lua.stx"
            {
	     localvar[nlocalvar+(yyvsp[-2].vInt)]=lua_findsymbol((yyvsp[0].pChar)); 
	     (yyval.vInt) = (yyvsp[-2].vInt)+1;
	    }
#line 2306 "y.tab.c"
    break;

  case 103: /* setdebug: DEBUG  */
#line 636 "lua.stx"
                  {lua_debug = (yyvsp[0].vInt);}
#line 2312 "y.tab.c"
    break;


#line 2316 "y.tab.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 638 "lua.stx"


/*
** Search a local name and if find return its index. If do not find return -1
*/
int lua_localname (Word n)//内部函数：搜索一个局部变量名称n，如果找到则返回其索引，否则返回-1
{
 int i;
 for (i=nlocalvar-1; i >= 0; i--)
  if (n == localvar[i]) return i;	/* local var */
 return -1;		        /* global var */
}

/*
** Push a variable given a number. If number is positive, push global variable
** indexed by (number -1). If negative, push local indexed by ABS(number)-1.
** Otherwise, if zero, push indexed variable (record).
*/
void lua_pushvar (long number)//内部函数：根据给定的编号number推入一个变量
{ 
 if (number > 0)	/* global var *///如果number是正数，表示这是一个全局变量，编号为number-1，推入这个全局变量
 {
  code_byte(PUSHGLOBAL);
  code_word(number-1);
  incr_ntemp();
 }
 else if (number < 0)	/* local var *///如果number是负数，表示这是一个局部变量，编号为ABS(number)-1，推入这个局部变量
 {
  number = (-number) - 1;
  if (number < 10) code_byte(PUSHLOCAL0 + number);
  else
  {
   code_byte(PUSHLOCAL);
   code_byte(number);
  }
  incr_ntemp();
 }
 else//如果number是0，表示这是一个索引变量（记录），推入这个索引变量
 {
  code_byte(PUSHINDEXED);
  ntemp--;
 }
}

void lua_codeadjust (int n)//内部函数：生成一个调整指令，调整当前代码的偏移位置n
{
 code_byte(ADJUST);
 code_byte(n + nlocalvar);
}

void lua_codestore (int i)//内部函数：生成一个存储指令，将当前值存储到变量i中
{
 if (varbuffer[i] > 0)		/* global var *///如果varbuffer[i]是正数，表示这是一个全局变量
 {
  code_byte(STOREGLOBAL);
  code_word(varbuffer[i]-1);
 }
 else if (varbuffer[i] < 0)      /* local var *///如果varbuffer[i]是负数，表示这是一个局部变量
 {
  int number = (-varbuffer[i]) - 1;
  if (number < 10) code_byte(STORELOCAL0 + number);
  else
  {
   code_byte(STORELOCAL);
   code_byte(number);
  }
 }
 else				  /* indexed var *///如果varbuffer[i]是0，表示这是一个索引变量（记录）
 {
  int j;
  int upper=0;     	//计算在当前变量i之后的变量中有多少个是索引变量（记录）
  int param;		/* number of itens until indexed expression *///计算从当前变量i到下一个索引变量（记录）之间的变量数量
  for (j=i+1; j <nvarbuffer; j++)
   if (varbuffer[j] == 0) upper++;
  param = upper*2 + i;
  if (param == 0)//如果param是0，表示当前变量i之后没有索引变量（记录），直接生成一个存储索引变量的指令，参数为0
   code_byte(STOREINDEXED0);
  else//如果param不是0，表示当前变量i之后有索引变量（记录），生成一个存储索引变量的指令，参数为param
  {
   code_byte(STOREINDEXED);
   code_byte(param);
  }
 }
}

void yyerror (char *s)//接受一个错误信息字符串s作为参数，生成一个详细的错误报告
{
 static char msg[256];
 sprintf (msg,"%s near \"%s\" at line %d in file \"%s\"",
          s, lua_lasttext (), lua_linenumber, lua_filename());
 lua_error (msg);
 err = 1;
}

int yywrap (void)//当词法分析器扫描到输入的末尾时调用，返回1表示没有更多输入需要处理，结束词法分析过程
{
 return 1;
}


/*
** Parse LUA code and execute global statement.
** Return 0 on success or 1 on error.
*/
int lua_parse (void)//解析Lua代码并执行全局语句，返回0表示成功，返回1表示错误
{
 Byte *init = initcode = (Byte *) calloc(GAPCODE, sizeof(Byte));
 maincode = 0; 
 maxmain = GAPCODE;
 if (init == NULL)
 {
  lua_error("not enough memory");
  return 1;
 }
 err = 0;
 if (yyparse () || (err==1)) return 1;//调用语法分析器进行解析，如果解析过程中发生错误或者err标志被设置为1，返回1表示错误
 initcode[maincode++] = HALT;//在主代码的末尾添加一个HALT指令，表示程序的结束
 init = initcode;
#if LISTING
 PrintCode(init,init+maincode);
#endif
 if (lua_execute (init)) return 1;
 free(init);
 return 0;
}


#if LISTING
/*
**重构PrintCode函数，解决LISTING模式下原32与现64位数据类型与位宽不匹配问题
**同时美化代码格式
*/
void PrintCode (Byte *code, Byte *end)
{
    Byte *p = code;
    printf ("\n\nCODE\n");
    while (p < end)
    {
        long n = (long)(p - code); /* 使用 long 解决 %ld 匹配，并固定当前指令地址 */
        switch ((OpCode)*p)
        {
            case PUSHNIL:
                printf ("%ld    PUSHNIL\n", n);
                p++;
                break;
            case PUSH0: case PUSH1: case PUSH2:
                printf ("%ld    PUSH%c\n", n, (int)(*p - PUSH0 + '0'));
                p++;
                break;
            case PUSHBYTE:
                {
                    int val = (int)(*(++p));
                    printf ("%ld    PUSHBYTE   %d\n", n, val);
                    p++;
                }
                break;
            case PUSHWORD:
                {
                    CodeWord c;
                    p++;
                    get_word(c, p);
                    printf ("%ld    PUSHWORD   %d\n", n, (int)c.w);
                }
                break;
            case PUSHFLOAT:
                {
                    CodeFloat c;
                    p++;
                    get_float(c, p);
                    printf ("%ld    PUSHFLOAT  %f\n", n, c.f);
                }
                break;
            case PUSHSTRING:
                {
                    CodeWord c;
                    p++;
                    get_word(c, p);
                    printf ("%ld    PUSHSTRING   %d\n", n, (int)c.w);
                }
                break;
            case PUSHLOCAL0: case PUSHLOCAL1: case PUSHLOCAL2: case PUSHLOCAL3:
            case PUSHLOCAL4: case PUSHLOCAL5: case PUSHLOCAL6: case PUSHLOCAL7:
            case PUSHLOCAL8: case PUSHLOCAL9:
                printf ("%ld    PUSHLOCAL%c\n", n, (int)(*p - PUSHLOCAL0 + '0'));
                p++;
                break;
            case PUSHLOCAL:
                {
                    int val = (int)(*(++p));
                    printf ("%ld    PUSHLOCAL   %d\n", n, val);
                    p++;
                }
                break;
            case PUSHGLOBAL:
                {
                    CodeWord c;
                    p++;
                    get_word(c, p);
                    printf ("%ld    PUSHGLOBAL   %d\n", n, (int)c.w);
                }
                break;
            case PUSHINDEXED:  printf ("%ld    PUSHINDEXED\n", n); p++; break;
            case PUSHMARK:     printf ("%ld    PUSHMARK\n", n); p++; break;
            case PUSHOBJECT:   printf ("%ld    PUSHOBJECT\n", n); p++; break;
            case STORELOCAL0: case STORELOCAL1: case STORELOCAL2: case STORELOCAL3:
            case STORELOCAL4: case STORELOCAL5: case STORELOCAL6: case STORELOCAL7:
            case STORELOCAL8: case STORELOCAL9:
                printf ("%ld    STORELOCAL%c\n", n, (int)(*p - STORELOCAL0 + '0'));
                p++;
                break;
            case STORELOCAL:
                {
                    int val = (int)(*(++p));
                    printf ("%ld    STORELOCAL   %d\n", n, val);
                    p++;
                }
                break;
            case STOREGLOBAL:
                {
                    CodeWord c;
                    p++;
                    get_word(c, p);
                    printf ("%ld    STOREGLOBAL   %d\n", n, (int)c.w);
                }
                break;
            case STOREINDEXED0: printf ("%ld    STOREINDEXED0\n", n); p++; break;
            case STOREINDEXED:
                {
                    int val = (int)(*(++p));
                    printf ("%ld    STOREINDEXED   %d\n", n, val);
                    p++;
                }
                break;
            case STORELIST0:
                {
                    int val = (int)(*(++p));
                    printf ("%ld    STORELIST0  %d\n", n, val);
                    p++;
                }
                break;
            case STORELIST:
                {
                    int v1 = (int)(*(p+1));
                    int v2 = (int)(*(p+2));
                    printf ("%ld    STORELIST  %d %d\n", n, v1, v2);
                    p += 3;
                }
                break;
            case STORERECORD:
                {
                    int val = (int)(*(++p));
                    printf ("%ld    STORERECORD  %d\n", n, val);
                    p += val * sizeof(Word) + 1;
                }
                break;
            case ADJUST:
                {
                    int val = (int)(*(++p));
                    printf ("%ld    ADJUST   %d\n", n, val);
                    p++;
                }
                break;
            case CREATEARRAY:   printf ("%ld    CREATEARRAY\n", n); p++; break;
            case EQOP:          printf ("%ld    EQOP\n", n); p++; break;
            case LTOP:          printf ("%ld    LTOP\n", n); p++; break;
            case LEOP:          printf ("%ld    LEOP\n", n); p++; break;
            case ADDOP:         printf ("%ld    ADDOP\n", n); p++; break;
            case SUBOP:         printf ("%ld    SUBOP\n", n); p++; break;
            case MULTOP:        printf ("%ld    MULTOP\n", n); p++; break;
            case DIVOP:         printf ("%ld    DIVOP\n", n); p++; break;
            case CONCOP:        printf ("%ld    CONCOP\n", n); p++; break;
            case MINUSOP:       printf ("%ld    MINUSOP\n", n); p++; break;
            case NOTOP:         printf ("%ld    NOTOP\n", n); p++; break;
            case ONTJMP:
                {
                    CodeWord c; p++; get_word(c, p);
                    printf ("%ld    ONTJMP  %d\n", n, (int)c.w);
                }
                break;
            case ONFJMP:
                {
                    CodeWord c; p++; get_word(c, p);
                    printf ("%ld    ONFJMP  %d\n", n, (int)c.w);
                }
                break;
            case JMP:
                {
                    CodeWord c; p++; get_word(c, p);
                    printf ("%ld    JMP  %d\n", n, (int)c.w);
                }
                break;
            case UPJMP:
                {
                    CodeWord c; p++; get_word(c, p);
                    printf ("%ld    UPJMP  %d\n", n, (int)c.w);
                }
                break;
            case IFFJMP:
                {
                    CodeWord c; p++; get_word(c, p);
                    printf ("%ld    IFFJMP  %d\n", n, (int)c.w);
                }
                break;
            case IFFUPJMP:
                {
                    CodeWord c; p++; get_word(c, p);
                    printf ("%ld    IFFUPJMP  %d\n", n, (int)c.w);
                }
                break;
            case POP:           printf ("%ld    POP\n", n); p++; break;
            case CALLFUNC:      printf ("%ld    CALLFUNC\n", n); p++; break;
            case RETCODE:
                {
                    int val = (int)(*(++p));
                    printf ("%ld    RETCODE   %d\n", n, val);
                    p++;
                }
                break;
            case HALT:          printf ("%ld    HALT\n", n); p++; break;
            case SETFUNCTION:
                {
                    CodeWord c1, c2;
                    p++;
                    get_word(c1, p);
                    get_word(c2, p);
                    printf ("%ld    SETFUNCTION  %d  %d\n", n, (int)c1.w, (int)c2.w);
                }
                break;
            case SETLINE:
                {
                    CodeWord c;
                    p++;
                    get_word(c, p);
                    printf ("%ld    SETLINE  %d\n", n, (int)c.w);
                }
                break;
            case RESET:         printf ("%ld    RESET\n", n); p++; break;
            default:            printf ("%ld    Cannot happen: code %d\n", n, (int)(*p)); p++; break;
        }
    }
}
#endif

