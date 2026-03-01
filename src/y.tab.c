
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

#define LISTING 0 //是否生成汇编代码列表的标志，0表示不生成，1表示生成，以便在编译过程中输出详细的汇编代码信息，方便调试和分析Lua虚拟机的执行过程

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


/* 静态变量：用于在编译匿名函数时备份主程序的进度 */
static Byte *save_basepc;
static int save_pc;
static int save_maxcurr;
static int save_nlocalvar;
static int save_ntemp;


/* 声明加在 lua.stx 头部的 %{ 和 %} 之间 */
void PrintCode (Byte *base, Byte *end);
void yyerror (char *s);
int  yylex (void);

/* 下面这几个是报错最严重的，一定要加 */
void lua_pushvar (long n);
void lua_codeadjust (int n);
void lua_codestore (int n);
int  lua_localname (Word n);

//新增code_compound_finish（）函数，供+=、-=等使用
void code_compound_finish(long var_info, int op_code);

//开启函数编译作用域
void lua_begin_scope(int symbol_idx);
//结束函数编译作用域
void lua_end_scope(int symbol_idx);



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


#line 282 "y.tab.c"

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
    ADDEQ = 276,                   /* ADDEQ  */
    SUBEQ = 277,                   /* SUBEQ  */
    MULTEQ = 278,                  /* MULTEQ  */
    DIVEQ = 279,                   /* DIVEQ  */
    AND = 280,                     /* AND  */
    OR = 281,                      /* OR  */
    NE = 282,                      /* NE  */
    LE = 283,                      /* LE  */
    GE = 284,                      /* GE  */
    CONC = 285,                    /* CONC  */
    UNARY = 286,                   /* UNARY  */
    NOT = 287                      /* NOT  */
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
#define ADDEQ 276
#define SUBEQ 277
#define MULTEQ 278
#define DIVEQ 279
#define AND 280
#define OR 281
#define NE 282
#define LE 283
#define GE 284
#define CONC 285
#define UNARY 286
#define NOT 287

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 214 "lua.stx"

 int   vInt;
 long  vLong;
 float vFloat;
 char *pChar;
 Word  vWord;
 Byte *pByte;

#line 408 "y.tab.c"

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
  YYSYMBOL_ADDEQ = 21,                     /* ADDEQ  */
  YYSYMBOL_SUBEQ = 22,                     /* SUBEQ  */
  YYSYMBOL_MULTEQ = 23,                    /* MULTEQ  */
  YYSYMBOL_DIVEQ = 24,                     /* DIVEQ  */
  YYSYMBOL_AND = 25,                       /* AND  */
  YYSYMBOL_OR = 26,                        /* OR  */
  YYSYMBOL_27_ = 27,                       /* '='  */
  YYSYMBOL_NE = 28,                        /* NE  */
  YYSYMBOL_29_ = 29,                       /* '>'  */
  YYSYMBOL_30_ = 30,                       /* '<'  */
  YYSYMBOL_LE = 31,                        /* LE  */
  YYSYMBOL_GE = 32,                        /* GE  */
  YYSYMBOL_CONC = 33,                      /* CONC  */
  YYSYMBOL_34_ = 34,                       /* '+'  */
  YYSYMBOL_35_ = 35,                       /* '-'  */
  YYSYMBOL_36_ = 36,                       /* '*'  */
  YYSYMBOL_37_ = 37,                       /* '/'  */
  YYSYMBOL_UNARY = 38,                     /* UNARY  */
  YYSYMBOL_NOT = 39,                       /* NOT  */
  YYSYMBOL_40_ = 40,                       /* '('  */
  YYSYMBOL_41_ = 41,                       /* ')'  */
  YYSYMBOL_42_ = 42,                       /* ';'  */
  YYSYMBOL_43_ = 43,                       /* '@'  */
  YYSYMBOL_44_ = 44,                       /* ','  */
  YYSYMBOL_45_ = 45,                       /* '{'  */
  YYSYMBOL_46_ = 46,                       /* '}'  */
  YYSYMBOL_47_ = 47,                       /* '['  */
  YYSYMBOL_48_ = 48,                       /* ']'  */
  YYSYMBOL_49_ = 49,                       /* '.'  */
  YYSYMBOL_YYACCEPT = 50,                  /* $accept  */
  YYSYMBOL_functionlist = 51,              /* functionlist  */
  YYSYMBOL_52_1 = 52,                      /* $@1  */
  YYSYMBOL_function = 53,                  /* function  */
  YYSYMBOL_54_2 = 54,                      /* @2  */
  YYSYMBOL_55_3 = 55,                      /* $@3  */
  YYSYMBOL_anonymous_function = 56,        /* anonymous_function  */
  YYSYMBOL_57_4 = 57,                      /* @4  */
  YYSYMBOL_58_5 = 58,                      /* $@5  */
  YYSYMBOL_statlist = 59,                  /* statlist  */
  YYSYMBOL_stat = 60,                      /* stat  */
  YYSYMBOL_61_6 = 61,                      /* $@6  */
  YYSYMBOL_sc = 62,                        /* sc  */
  YYSYMBOL_stat1 = 63,                     /* stat1  */
  YYSYMBOL_64_7 = 64,                      /* @7  */
  YYSYMBOL_65_8 = 65,                      /* @8  */
  YYSYMBOL_66_9 = 66,                      /* $@9  */
  YYSYMBOL_67_10 = 67,                     /* $@10  */
  YYSYMBOL_68_11 = 68,                     /* $@11  */
  YYSYMBOL_69_12 = 69,                     /* $@12  */
  YYSYMBOL_elsepart = 70,                  /* elsepart  */
  YYSYMBOL_block = 71,                     /* block  */
  YYSYMBOL_72_13 = 72,                     /* @13  */
  YYSYMBOL_73_14 = 73,                     /* $@14  */
  YYSYMBOL_ret = 74,                       /* ret  */
  YYSYMBOL_75_15 = 75,                     /* $@15  */
  YYSYMBOL_PrepJump = 76,                  /* PrepJump  */
  YYSYMBOL_expr1 = 77,                     /* expr1  */
  YYSYMBOL_expr = 78,                      /* expr  */
  YYSYMBOL_79_16 = 79,                     /* $@16  */
  YYSYMBOL_80_17 = 80,                     /* $@17  */
  YYSYMBOL_typeconstructor = 81,           /* typeconstructor  */
  YYSYMBOL_82_18 = 82,                     /* @18  */
  YYSYMBOL_dimension = 83,                 /* dimension  */
  YYSYMBOL_functioncall = 84,              /* functioncall  */
  YYSYMBOL_85_19 = 85,                     /* @19  */
  YYSYMBOL_functionvalue = 86,             /* functionvalue  */
  YYSYMBOL_exprlist = 87,                  /* exprlist  */
  YYSYMBOL_exprlist1 = 88,                 /* exprlist1  */
  YYSYMBOL_89_20 = 89,                     /* $@20  */
  YYSYMBOL_parlist = 90,                   /* parlist  */
  YYSYMBOL_parlist1 = 91,                  /* parlist1  */
  YYSYMBOL_objectname = 92,                /* objectname  */
  YYSYMBOL_fieldlist = 93,                 /* fieldlist  */
  YYSYMBOL_ffieldlist = 94,                /* ffieldlist  */
  YYSYMBOL_ffieldlist1 = 95,               /* ffieldlist1  */
  YYSYMBOL_ffield = 96,                    /* ffield  */
  YYSYMBOL_97_21 = 97,                     /* @21  */
  YYSYMBOL_lfieldlist = 98,                /* lfieldlist  */
  YYSYMBOL_lfieldlist1 = 99,               /* lfieldlist1  */
  YYSYMBOL_varlist1 = 100,                 /* varlist1  */
  YYSYMBOL_var = 101,                      /* var  */
  YYSYMBOL_102_22 = 102,                   /* $@22  */
  YYSYMBOL_103_23 = 103,                   /* $@23  */
  YYSYMBOL_localdeclist = 104,             /* localdeclist  */
  YYSYMBOL_decinit = 105,                  /* decinit  */
  YYSYMBOL_setdebug = 106                  /* setdebug  */
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
#define YYLAST   268

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  50
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  57
/* YYNRULES -- Number of rules.  */
#define YYNRULES  115
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  198

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   287


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
      40,    41,    36,    34,    44,    35,    49,    37,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    42,
      30,    27,    29,     2,    43,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    47,     2,    48,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    45,     2,    46,     2,     2,     2,     2,
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
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    28,    31,    32,    33,    38,    39
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   256,   256,   258,   257,   266,   267,   272,   277,   271,
     292,   297,   291,   313,   314,   317,   317,   326,   326,   329,
     348,   348,   358,   358,   366,   378,   378,   380,   380,   382,
     382,   384,   384,   387,   388,   389,   392,   393,   394,   414,
     414,   414,   424,   425,   425,   434,   440,   443,   444,   445,
     446,   447,   448,   449,   450,   451,   452,   453,   454,   455,
     456,   457,   458,   463,   464,   465,   472,   473,   481,   482,
     482,   488,   488,   497,   496,   528,   529,   532,   532,   535,
     536,   539,   540,   543,   544,   544,   548,   549,   552,   557,
     564,   565,   568,   573,   580,   581,   584,   585,   592,   592,
     598,   599,   602,   603,   611,   617,   624,   634,   634,   638,
     638,   646,   647,   654,   655,   658
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
  "ADDEQ", "SUBEQ", "MULTEQ", "DIVEQ", "AND", "OR", "'='", "NE", "'>'",
  "'<'", "LE", "GE", "CONC", "'+'", "'-'", "'*'", "'/'", "UNARY", "NOT",
  "'('", "')'", "';'", "'@'", "','", "'{'", "'}'", "'['", "']'", "'.'",
  "$accept", "functionlist", "$@1", "function", "@2", "$@3",
  "anonymous_function", "@4", "$@5", "statlist", "stat", "$@6", "sc",
  "stat1", "@7", "@8", "$@9", "$@10", "$@11", "$@12", "elsepart", "block",
  "@13", "$@14", "ret", "$@15", "PrepJump", "expr1", "expr", "$@16",
  "$@17", "typeconstructor", "@18", "dimension", "functioncall", "@19",
  "functionvalue", "exprlist", "exprlist1", "$@20", "parlist", "parlist1",
  "objectname", "fieldlist", "ffieldlist", "ffieldlist1", "ffield", "@21",
  "lfieldlist", "lfieldlist1", "varlist1", "var", "$@22", "$@23",
  "localdeclist", "decinit", "setdebug", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-142)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-110)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    -142,     6,  -142,    -3,  -142,  -142,  -142,  -142,  -142,   -33,
       2,   -16,  -142,  -142,   112,  -142,  -142,    15,  -142,    21,
    -142,  -142,  -142,  -142,  -142,  -142,   -19,    85,    24,  -142,
    -142,  -142,   112,   112,   112,    80,    10,   142,  -142,  -142,
    -142,   -35,   112,  -142,  -142,   -17,  -142,    35,    31,   112,
      53,  -142,  -142,  -142,  -142,    27,    26,  -142,    37,    44,
    -142,  -142,  -142,   207,    48,   112,  -142,  -142,  -142,   112,
     112,   112,   112,   112,   112,   112,   112,   112,   112,   112,
     170,    82,  -142,   112,    81,  -142,    61,  -142,   -10,   112,
     220,    58,   -35,   112,   112,   112,   112,   112,    84,  -142,
      91,  -142,   207,    71,  -142,  -142,  -142,    -4,    -4,    -4,
      -4,    -4,    -4,    32,     4,     4,  -142,  -142,  -142,   112,
      79,    58,  -142,    24,    94,   112,  -142,    76,    58,  -142,
     207,   207,   207,   207,   183,  -142,  -142,  -142,  -142,  -142,
     112,   112,  -142,   207,   -33,   104,    92,  -142,    75,    78,
    -142,   207,    87,    93,  -142,   112,  -142,   123,    41,   231,
     231,  -142,  -142,  -142,  -142,   124,  -142,   113,  -142,    94,
    -142,   112,   220,  -142,  -142,   112,   126,   128,   112,  -142,
     112,  -142,   207,  -142,   156,  -142,  -142,   -33,   130,   207,
    -142,  -142,   103,  -142,  -142,  -142,    41,  -142
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       2,     3,     1,     0,   115,    15,     5,     6,     7,    17,
       0,     0,    18,     4,     0,    20,    22,     0,   106,     0,
      73,    80,    16,    34,    33,    77,     0,   104,    86,    66,
      64,    65,     0,     0,     0,     0,    73,     0,    46,    61,
      67,    63,     0,    39,   111,   113,    10,    90,     0,     0,
       0,    25,    27,    29,    31,     0,     0,    88,     0,    87,
      59,    60,    68,     0,    46,    75,    45,    45,    45,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    13,     0,     0,    35,     0,    91,     0,    81,
      83,    24,   105,     0,     0,     0,     0,     0,     0,     8,
       0,    47,    76,     0,    39,    69,    71,    48,    51,    50,
      49,    52,    53,    58,    54,    55,    56,    57,    45,     0,
      15,   114,   112,    86,    94,   100,    74,     0,    82,    84,
      26,    28,    30,    32,     0,   110,    39,    89,    62,    45,
       0,     0,    39,    45,    17,    42,     0,    98,     0,    95,
      96,   102,     0,   101,    78,     0,   108,     0,    36,    70,
      72,    45,    23,    14,    41,     0,    11,     0,    92,     0,
      93,     0,    85,     9,    39,     0,     0,     0,    81,    39,
       0,    97,   103,    37,     0,    19,    21,    17,     0,    99,
      45,    44,     0,    39,    12,    45,    36,    38
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -142,  -142,  -142,  -142,  -142,  -142,  -142,  -142,  -142,  -142,
      25,  -142,  -141,  -142,  -142,  -142,  -142,  -142,  -142,  -142,
     -47,   -89,  -142,  -142,  -142,  -142,   -66,   -14,   -13,  -142,
    -142,   140,  -142,  -142,   143,  -142,  -142,   -24,   -44,  -142,
      33,  -142,  -142,  -142,  -142,  -142,   -11,  -142,  -142,  -142,
    -142,    -6,  -142,  -142,  -142,  -142,  -142
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,     1,     5,     6,    11,   136,    21,    86,   179,   120,
       9,    10,    13,    22,    42,    43,    93,    94,    95,    96,
     176,    81,    82,   145,   164,   165,   104,    63,    38,   140,
     141,    39,    47,   103,    40,    48,    25,   127,   128,   155,
      58,    59,    88,   126,   148,   149,   150,   167,   152,   153,
      26,    41,    55,    56,    45,    85,     7
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      37,   105,   106,   163,    27,    91,     2,    14,    49,    12,
      83,    15,  -107,    16,  -109,   139,     8,    17,    60,    61,
      62,    18,    64,     3,    28,    50,     4,    84,    80,    75,
      76,    77,    78,    79,    44,   124,    90,   125,    46,   121,
      78,    79,    19,    57,    92,    20,   191,   157,   174,   175,
      65,   102,   142,   161,    87,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,    76,    77,    78,    79,
      90,    89,    18,   158,    97,    98,    90,   162,    99,   130,
     131,   132,   133,   134,    29,   183,   -40,   -40,   100,   101,
     188,   -40,   -40,   -40,   119,   177,    30,    46,    31,    18,
     122,   123,   129,   135,   195,   143,    51,    52,    53,    54,
     137,   151,   138,   147,    32,    33,    29,   154,   -43,    34,
      35,   168,   169,    36,   193,   -79,   159,   160,    30,   196,
      31,    18,  -107,   166,  -109,   170,   173,   171,   178,   185,
     180,   186,   172,   192,   194,   144,    32,    33,    66,   197,
      23,    34,    35,    24,   187,    36,   146,   182,   181,     0,
       0,   184,   190,     0,     0,    90,   189,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
     118,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,     0,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   156,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,   -46,   -46,   -46,   -46,   -46,
     -46,   -46,   -46,   -46,   -46,   -46,   -46,   -46,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79
};

static const yytype_int16 yycheck[] =
{
      14,    67,    68,   144,    10,    49,     0,     5,    27,    42,
      27,     9,    47,    11,    49,   104,    19,    15,    32,    33,
      34,    19,    35,    17,    40,    44,    20,    44,    42,    33,
      34,    35,    36,    37,    19,    45,    49,    47,    17,    83,
      36,    37,    40,    19,    50,    43,   187,   136,     7,     8,
      40,    65,   118,   142,    19,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    34,    35,    36,    37,
      83,    40,    19,   139,    47,    49,    89,   143,    41,    93,
      94,    95,    96,    97,     4,   174,     7,     8,    44,    41,
     179,    12,    13,    14,    12,   161,    16,    17,    18,    19,
      19,    40,    44,    19,   193,   119,    21,    22,    23,    24,
      19,   125,    41,    19,    34,    35,     4,    41,    14,    39,
      40,    46,    44,    43,   190,    40,   140,   141,    16,   195,
      18,    19,    47,    41,    49,    48,    13,    44,    14,    13,
      27,    13,   155,    13,    41,   120,    34,    35,     6,   196,
      10,    39,    40,    10,   178,    43,   123,   171,   169,    -1,
      -1,   175,     6,    -1,    -1,   178,   180,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      10,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    -1,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    48,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,    51,     0,    17,    20,    52,    53,   106,    19,    60,
      61,    54,    42,    62,     5,     9,    11,    15,    19,    40,
      43,    56,    63,    81,    84,    86,   100,   101,    40,     4,
      16,    18,    34,    35,    39,    40,    43,    77,    78,    81,
      84,   101,    64,    65,    19,   104,    17,    82,    85,    27,
      44,    21,    22,    23,    24,   102,   103,    19,    90,    91,
      77,    77,    77,    77,    78,    40,     6,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      77,    71,    72,    27,    44,   105,    57,    19,    92,    40,
      78,    88,   101,    66,    67,    68,    69,    47,    49,    41,
      44,    41,    77,    83,    76,    76,    76,    77,    77,    77,
      77,    77,    77,    77,    77,    77,    77,    77,    10,    12,
      59,    88,    19,    40,    45,    47,    93,    87,    88,    44,
      77,    77,    77,    77,    77,    19,    55,    19,    41,    71,
      79,    80,    76,    77,    60,    73,    90,    19,    94,    95,
      96,    77,    98,    99,    41,    89,    48,    71,    76,    77,
      77,    71,    76,    62,    74,    75,    41,    97,    46,    44,
      48,    44,    78,    13,     7,     8,    70,    76,    14,    58,
      27,    96,    77,    71,    77,    13,    13,    87,    71,    77,
       6,    62,    13,    76,    41,    71,    76,    70
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    50,    51,    52,    51,    51,    51,    54,    55,    53,
      57,    58,    56,    59,    59,    61,    60,    62,    62,    63,
      64,    63,    65,    63,    63,    66,    63,    67,    63,    68,
      63,    69,    63,    63,    63,    63,    70,    70,    70,    72,
      73,    71,    74,    75,    74,    76,    77,    78,    78,    78,
      78,    78,    78,    78,    78,    78,    78,    78,    78,    78,
      78,    78,    78,    78,    78,    78,    78,    78,    78,    79,
      78,    80,    78,    82,    81,    83,    83,    85,    84,    86,
      86,    87,    87,    88,    89,    88,    90,    90,    91,    91,
      92,    92,    93,    93,    94,    94,    95,    95,    97,    96,
      98,    98,    99,    99,   100,   100,   101,   102,   101,   103,
     101,   104,   104,   105,   105,   106
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     0,     4,     2,     2,     0,     0,     9,
       0,     0,    10,     0,     3,     0,     2,     0,     1,     8,
       0,     8,     0,     6,     3,     0,     4,     0,     4,     0,
       4,     0,     4,     1,     1,     3,     0,     2,     7,     0,
       0,     4,     0,     0,     4,     0,     1,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     2,
       2,     1,     4,     1,     1,     1,     1,     1,     2,     0,
       5,     0,     5,     0,     4,     0,     1,     0,     5,     1,
       1,     0,     1,     1,     0,     4,     0,     1,     1,     3,
       0,     1,     3,     3,     0,     1,     1,     3,     0,     4,
       0,     1,     1,     3,     1,     3,     1,     0,     5,     0,
       4,     1,     3,     0,     2,     1
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
#line 258 "lua.stx"
                {
	  	  pc=maincode; basepc=initcode; maxcurr=maxmain;
		  nlocalvar=0;
	        }
#line 1650 "y.tab.c"
    break;

  case 4: /* functionlist: functionlist $@1 stat sc  */
#line 263 "lua.stx"
                {
		  maincode=pc; initcode=basepc; maxmain=maxcurr;
		}
#line 1658 "y.tab.c"
    break;

  case 7: /* @2: %empty  */
#line 272 "lua.stx"
               {
                (yyval.vWord) = lua_findsymbol((yyvsp[0].pChar)); 
                lua_begin_scope((yyval.vWord));
	       }
#line 1667 "y.tab.c"
    break;

  case 8: /* $@3: %empty  */
#line 277 "lua.stx"
           { 
                lua_codeadjust(0); 
            }
#line 1675 "y.tab.c"
    break;

  case 9: /* function: FUNCTION NAME @2 '(' parlist ')' $@3 block END  */
#line 282 "lua.stx"
               {
            lua_end_scope((yyvsp[-6].vWord));
#if LISTING
PrintCode(code,code+pc);
#endif
	       }
#line 1686 "y.tab.c"
    break;

  case 10: /* @4: %empty  */
#line 292 "lua.stx"
            {
                (yyval.vWord) = lua_findsymbol("__ANONYMOUS_FUN"); 
                lua_begin_scope((yyval.vWord));
            }
#line 1695 "y.tab.c"
    break;

  case 11: /* $@5: %empty  */
#line 297 "lua.stx"
            { 
                lua_codeadjust(0); 
            }
#line 1703 "y.tab.c"
    break;

  case 12: /* anonymous_function: '(' FUNCTION @4 '(' parlist ')' $@5 block END ')'  */
#line 302 "lua.stx"
               { 
                lua_end_scope((yyvsp[-7].vWord)); 
                // 定义完立刻压栈，供 functioncall 使用
                lua_pushvar((yyvsp[-7].vWord)+1);

#if LISTING
PrintCode(code,code+pc);
#endif
	       }
#line 1717 "y.tab.c"
    break;

  case 15: /* $@6: %empty  */
#line 317 "lua.stx"
           {
            ntemp = 0; 
            if (lua_debug)
            {
             code_byte(SETLINE); code_word(lua_linenumber);
            }
	   }
#line 1729 "y.tab.c"
    break;

  case 19: /* stat1: IF expr1 THEN PrepJump block PrepJump elsepart END  */
#line 330 "lua.stx"
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
#line 1751 "y.tab.c"
    break;

  case 20: /* @7: %empty  */
#line 348 "lua.stx"
               {(yyval.vWord)=pc;}
#line 1757 "y.tab.c"
    break;

  case 21: /* stat1: WHILE @7 expr1 DO PrepJump block PrepJump END  */
#line 350 "lua.stx"
       {
        basepc[(yyvsp[-3].vWord)] = IFFJMP;
	code_word_at(basepc+(yyvsp[-3].vWord)+1, pc - ((yyvsp[-3].vWord) + sizeof(Word)+1));
        
        basepc[(yyvsp[-1].vWord)] = UPJMP;
	code_word_at(basepc+(yyvsp[-1].vWord)+1, pc - ((yyvsp[-6].vWord)));
       }
#line 1769 "y.tab.c"
    break;

  case 22: /* @8: %empty  */
#line 358 "lua.stx"
                {(yyval.vWord)=pc;}
#line 1775 "y.tab.c"
    break;

  case 23: /* stat1: REPEAT @8 block UNTIL expr1 PrepJump  */
#line 360 "lua.stx"
       {
        basepc[(yyvsp[0].vWord)] = IFFUPJMP;
	code_word_at(basepc+(yyvsp[0].vWord)+1, pc - ((yyvsp[-4].vWord)));
       }
#line 1784 "y.tab.c"
    break;

  case 24: /* stat1: varlist1 '=' exprlist1  */
#line 367 "lua.stx"
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
#line 1800 "y.tab.c"
    break;

  case 25: /* $@9: %empty  */
#line 378 "lua.stx"
                   { lua_pushvar((yyvsp[-1].vLong)); }
#line 1806 "y.tab.c"
    break;

  case 26: /* stat1: var ADDEQ $@9 expr1  */
#line 379 "lua.stx"
        { code_compound_finish((yyvsp[-3].vLong), ADDOP); ntemp--; }
#line 1812 "y.tab.c"
    break;

  case 27: /* $@10: %empty  */
#line 380 "lua.stx"
                   { lua_pushvar((yyvsp[-1].vLong)); }
#line 1818 "y.tab.c"
    break;

  case 28: /* stat1: var SUBEQ $@10 expr1  */
#line 381 "lua.stx"
        { code_compound_finish((yyvsp[-3].vLong), SUBOP); ntemp--; }
#line 1824 "y.tab.c"
    break;

  case 29: /* $@11: %empty  */
#line 382 "lua.stx"
                    { lua_pushvar((yyvsp[-1].vLong)); }
#line 1830 "y.tab.c"
    break;

  case 30: /* stat1: var MULTEQ $@11 expr1  */
#line 383 "lua.stx"
        { code_compound_finish((yyvsp[-3].vLong), MULTOP); ntemp--; }
#line 1836 "y.tab.c"
    break;

  case 31: /* $@12: %empty  */
#line 384 "lua.stx"
                   { lua_pushvar((yyvsp[-1].vLong)); }
#line 1842 "y.tab.c"
    break;

  case 32: /* stat1: var DIVEQ $@12 expr1  */
#line 385 "lua.stx"
        { code_compound_finish((yyvsp[-3].vLong), DIVOP); ntemp--; }
#line 1848 "y.tab.c"
    break;

  case 33: /* stat1: functioncall  */
#line 387 "lua.stx"
                                        { lua_codeadjust (0); }
#line 1854 "y.tab.c"
    break;

  case 34: /* stat1: typeconstructor  */
#line 388 "lua.stx"
                                        { lua_codeadjust (0); }
#line 1860 "y.tab.c"
    break;

  case 35: /* stat1: LOCAL localdeclist decinit  */
#line 389 "lua.stx"
                                      { add_nlocalvar((yyvsp[-1].vInt)); lua_codeadjust (0); }
#line 1866 "y.tab.c"
    break;

  case 38: /* elsepart: ELSEIF expr1 THEN PrepJump block PrepJump elsepart  */
#line 395 "lua.stx"
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
#line 1888 "y.tab.c"
    break;

  case 39: /* @13: %empty  */
#line 414 "lua.stx"
           {(yyval.vInt) = nlocalvar;}
#line 1894 "y.tab.c"
    break;

  case 40: /* $@14: %empty  */
#line 414 "lua.stx"
                                            {ntemp = 0;}
#line 1900 "y.tab.c"
    break;

  case 41: /* block: @13 statlist $@14 ret  */
#line 415 "lua.stx"
         {
	  if (nlocalvar != (yyvsp[-3].vInt))
	  {
           nlocalvar = (yyvsp[-3].vInt);
	   lua_codeadjust (0);
	  }
         }
#line 1912 "y.tab.c"
    break;

  case 43: /* $@15: %empty  */
#line 425 "lua.stx"
          { if (lua_debug){code_byte(SETLINE);code_word(lua_linenumber);}}
#line 1918 "y.tab.c"
    break;

  case 44: /* ret: $@15 RETURN exprlist sc  */
#line 427 "lua.stx"
          { 
           if (lua_debug) code_byte(RESET); 
           code_byte(RETCODE); code_byte(nlocalvar);
          }
#line 1927 "y.tab.c"
    break;

  case 45: /* PrepJump: %empty  */
#line 434 "lua.stx"
         { 
	  (yyval.vWord) = pc;
	  code_byte(0);		/* open space */
	  code_word (0);
         }
#line 1937 "y.tab.c"
    break;

  case 46: /* expr1: expr  */
#line 440 "lua.stx"
                { if ((yyvsp[0].vInt) == 0) {lua_codeadjust (ntemp+1); incr_ntemp();}}
#line 1943 "y.tab.c"
    break;

  case 47: /* expr: '(' expr ')'  */
#line 443 "lua.stx"
                        { (yyval.vInt) = (yyvsp[-1].vInt); }
#line 1949 "y.tab.c"
    break;

  case 48: /* expr: expr1 '=' expr1  */
#line 444 "lua.stx"
                        { code_byte(EQOP);   (yyval.vInt) = 1; ntemp--;}
#line 1955 "y.tab.c"
    break;

  case 49: /* expr: expr1 '<' expr1  */
#line 445 "lua.stx"
                        { code_byte(LTOP);   (yyval.vInt) = 1; ntemp--;}
#line 1961 "y.tab.c"
    break;

  case 50: /* expr: expr1 '>' expr1  */
#line 446 "lua.stx"
                        { code_byte(LEOP); code_byte(NOTOP); (yyval.vInt) = 1; ntemp--;}
#line 1967 "y.tab.c"
    break;

  case 51: /* expr: expr1 NE expr1  */
#line 447 "lua.stx"
                        { code_byte(EQOP); code_byte(NOTOP); (yyval.vInt) = 1; ntemp--;}
#line 1973 "y.tab.c"
    break;

  case 52: /* expr: expr1 LE expr1  */
#line 448 "lua.stx"
                        { code_byte(LEOP);   (yyval.vInt) = 1; ntemp--;}
#line 1979 "y.tab.c"
    break;

  case 53: /* expr: expr1 GE expr1  */
#line 449 "lua.stx"
                        { code_byte(LTOP); code_byte(NOTOP); (yyval.vInt) = 1; ntemp--;}
#line 1985 "y.tab.c"
    break;

  case 54: /* expr: expr1 '+' expr1  */
#line 450 "lua.stx"
                        { code_byte(ADDOP);  (yyval.vInt) = 1; ntemp--;}
#line 1991 "y.tab.c"
    break;

  case 55: /* expr: expr1 '-' expr1  */
#line 451 "lua.stx"
                        { code_byte(SUBOP);  (yyval.vInt) = 1; ntemp--;}
#line 1997 "y.tab.c"
    break;

  case 56: /* expr: expr1 '*' expr1  */
#line 452 "lua.stx"
                        { code_byte(MULTOP); (yyval.vInt) = 1; ntemp--;}
#line 2003 "y.tab.c"
    break;

  case 57: /* expr: expr1 '/' expr1  */
#line 453 "lua.stx"
                        { code_byte(DIVOP);  (yyval.vInt) = 1; ntemp--;}
#line 2009 "y.tab.c"
    break;

  case 58: /* expr: expr1 CONC expr1  */
#line 454 "lua.stx"
                         { code_byte(CONCOP);  (yyval.vInt) = 1; ntemp--;}
#line 2015 "y.tab.c"
    break;

  case 59: /* expr: '+' expr1  */
#line 455 "lua.stx"
                                { (yyval.vInt) = 1; }
#line 2021 "y.tab.c"
    break;

  case 60: /* expr: '-' expr1  */
#line 456 "lua.stx"
                                { code_byte(MINUSOP); (yyval.vInt) = 1;}
#line 2027 "y.tab.c"
    break;

  case 61: /* expr: typeconstructor  */
#line 457 "lua.stx"
                       { (yyval.vInt) = (yyvsp[0].vInt); }
#line 2033 "y.tab.c"
    break;

  case 62: /* expr: '@' '(' dimension ')'  */
#line 459 "lua.stx"
     { 
      code_byte(CREATEARRAY);
      (yyval.vInt) = 1;
     }
#line 2042 "y.tab.c"
    break;

  case 63: /* expr: var  */
#line 463 "lua.stx"
                        { lua_pushvar ((yyvsp[0].vLong)); (yyval.vInt) = 1;}
#line 2048 "y.tab.c"
    break;

  case 64: /* expr: NUMBER  */
#line 464 "lua.stx"
                        { code_number((yyvsp[0].vFloat)); (yyval.vInt) = 1; }
#line 2054 "y.tab.c"
    break;

  case 65: /* expr: STRING  */
#line 466 "lua.stx"
     {
      code_byte(PUSHSTRING);
      code_word((yyvsp[0].vWord));
      (yyval.vInt) = 1;
      incr_ntemp();
     }
#line 2065 "y.tab.c"
    break;

  case 66: /* expr: NIL  */
#line 472 "lua.stx"
                        {code_byte(PUSHNIL); (yyval.vInt) = 1; incr_ntemp();}
#line 2071 "y.tab.c"
    break;

  case 67: /* expr: functioncall  */
#line 474 "lua.stx"
     {
      (yyval.vInt) = 0;
      if (lua_debug)
      {
       code_byte(SETLINE); code_word(lua_linenumber);
      }
     }
#line 2083 "y.tab.c"
    break;

  case 68: /* expr: NOT expr1  */
#line 481 "lua.stx"
                        { code_byte(NOTOP);  (yyval.vInt) = 1;}
#line 2089 "y.tab.c"
    break;

  case 69: /* $@16: %empty  */
#line 482 "lua.stx"
                           {code_byte(POP); ntemp--;}
#line 2095 "y.tab.c"
    break;

  case 70: /* expr: expr1 AND PrepJump $@16 expr1  */
#line 483 "lua.stx"
     { 
      basepc[(yyvsp[-2].vWord)] = ONFJMP;
      code_word_at(basepc+(yyvsp[-2].vWord)+1, pc - ((yyvsp[-2].vWord) + sizeof(Word)+1));
      (yyval.vInt) = 1;
     }
#line 2105 "y.tab.c"
    break;

  case 71: /* $@17: %empty  */
#line 488 "lua.stx"
                          {code_byte(POP); ntemp--;}
#line 2111 "y.tab.c"
    break;

  case 72: /* expr: expr1 OR PrepJump $@17 expr1  */
#line 489 "lua.stx"
     { 
      basepc[(yyvsp[-2].vWord)] = ONTJMP;
      code_word_at(basepc+(yyvsp[-2].vWord)+1, pc - ((yyvsp[-2].vWord) + sizeof(Word)+1));
      (yyval.vInt) = 1;
     }
#line 2121 "y.tab.c"
    break;

  case 73: /* @18: %empty  */
#line 497 "lua.stx"
     {
      code_byte(PUSHBYTE);
      (yyval.vWord) = pc; code_byte(0);
      incr_ntemp();
      code_byte(CREATEARRAY);
     }
#line 2132 "y.tab.c"
    break;

  case 74: /* typeconstructor: '@' @18 objectname fieldlist  */
#line 504 "lua.stx"
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
#line 2159 "y.tab.c"
    break;

  case 75: /* dimension: %empty  */
#line 528 "lua.stx"
                                { code_byte(PUSHNIL); incr_ntemp();}
#line 2165 "y.tab.c"
    break;

  case 77: /* @19: %empty  */
#line 532 "lua.stx"
                              {code_byte(PUSHMARK); (yyval.vInt) = ntemp; incr_ntemp();}
#line 2171 "y.tab.c"
    break;

  case 78: /* functioncall: functionvalue @19 '(' exprlist ')'  */
#line 533 "lua.stx"
                                 { code_byte(CALLFUNC); ntemp = (yyvsp[-3].vInt)-1;}
#line 2177 "y.tab.c"
    break;

  case 79: /* functionvalue: var  */
#line 535 "lua.stx"
                    {lua_pushvar ((yyvsp[0].vLong)); }
#line 2183 "y.tab.c"
    break;

  case 81: /* exprlist: %empty  */
#line 539 "lua.stx"
                                        { (yyval.vInt) = 1; }
#line 2189 "y.tab.c"
    break;

  case 82: /* exprlist: exprlist1  */
#line 540 "lua.stx"
                                        { (yyval.vInt) = (yyvsp[0].vInt); }
#line 2195 "y.tab.c"
    break;

  case 83: /* exprlist1: expr  */
#line 543 "lua.stx"
                                        { (yyval.vInt) = (yyvsp[0].vInt); }
#line 2201 "y.tab.c"
    break;

  case 84: /* $@20: %empty  */
#line 544 "lua.stx"
                              {if (!(yyvsp[-1].vInt)){lua_codeadjust (ntemp+1); incr_ntemp();}}
#line 2207 "y.tab.c"
    break;

  case 85: /* exprlist1: exprlist1 ',' $@20 expr  */
#line 545 "lua.stx"
                      {(yyval.vInt) = (yyvsp[0].vInt);}
#line 2213 "y.tab.c"
    break;

  case 88: /* parlist1: NAME  */
#line 553 "lua.stx"
                {
		 localvar[nlocalvar]=lua_findsymbol((yyvsp[0].pChar)); 
		 add_nlocalvar(1);
		}
#line 2222 "y.tab.c"
    break;

  case 89: /* parlist1: parlist1 ',' NAME  */
#line 558 "lua.stx"
                {
		 localvar[nlocalvar]=lua_findsymbol((yyvsp[0].pChar)); 
		 add_nlocalvar(1);
		}
#line 2231 "y.tab.c"
    break;

  case 90: /* objectname: %empty  */
#line 564 "lua.stx"
                                {(yyval.vLong)=-1;}
#line 2237 "y.tab.c"
    break;

  case 91: /* objectname: NAME  */
#line 565 "lua.stx"
                                {(yyval.vLong)=lua_findsymbol((yyvsp[0].pChar));}
#line 2243 "y.tab.c"
    break;

  case 92: /* fieldlist: '{' ffieldlist '}'  */
#line 569 "lua.stx"
              { 
	       flush_record((yyvsp[-1].vInt)%FIELDS_PER_FLUSH); 
	       (yyval.vInt) = (yyvsp[-1].vInt);
	      }
#line 2252 "y.tab.c"
    break;

  case 93: /* fieldlist: '[' lfieldlist ']'  */
#line 574 "lua.stx"
              { 
	       flush_list((yyvsp[-1].vInt)/FIELDS_PER_FLUSH, (yyvsp[-1].vInt)%FIELDS_PER_FLUSH);
	       (yyval.vInt) = (yyvsp[-1].vInt);
     	      }
#line 2261 "y.tab.c"
    break;

  case 94: /* ffieldlist: %empty  */
#line 580 "lua.stx"
                           { (yyval.vInt) = 0; }
#line 2267 "y.tab.c"
    break;

  case 95: /* ffieldlist: ffieldlist1  */
#line 581 "lua.stx"
                           { (yyval.vInt) = (yyvsp[0].vInt); }
#line 2273 "y.tab.c"
    break;

  case 96: /* ffieldlist1: ffield  */
#line 584 "lua.stx"
                                        {(yyval.vInt)=1;}
#line 2279 "y.tab.c"
    break;

  case 97: /* ffieldlist1: ffieldlist1 ',' ffield  */
#line 586 "lua.stx"
                {
		  (yyval.vInt)=(yyvsp[-2].vInt)+1;
		  if ((yyval.vInt)%FIELDS_PER_FLUSH == 0) flush_record(FIELDS_PER_FLUSH);
		}
#line 2288 "y.tab.c"
    break;

  case 98: /* @21: %empty  */
#line 592 "lua.stx"
                   {(yyval.vWord) = lua_findconstant((yyvsp[0].pChar));}
#line 2294 "y.tab.c"
    break;

  case 99: /* ffield: NAME @21 '=' expr1  */
#line 593 "lua.stx"
              { 
	       push_field((yyvsp[-2].vWord));
	      }
#line 2302 "y.tab.c"
    break;

  case 100: /* lfieldlist: %empty  */
#line 598 "lua.stx"
                           { (yyval.vInt) = 0; }
#line 2308 "y.tab.c"
    break;

  case 101: /* lfieldlist: lfieldlist1  */
#line 599 "lua.stx"
                           { (yyval.vInt) = (yyvsp[0].vInt); }
#line 2314 "y.tab.c"
    break;

  case 102: /* lfieldlist1: expr1  */
#line 602 "lua.stx"
                     {(yyval.vInt)=1;}
#line 2320 "y.tab.c"
    break;

  case 103: /* lfieldlist1: lfieldlist1 ',' expr1  */
#line 604 "lua.stx"
                {
		  (yyval.vInt)=(yyvsp[-2].vInt)+1;
		  if ((yyval.vInt)%FIELDS_PER_FLUSH == 0) 
		    flush_list((yyval.vInt)/FIELDS_PER_FLUSH - 1, FIELDS_PER_FLUSH);
		}
#line 2330 "y.tab.c"
    break;

  case 104: /* varlist1: var  */
#line 612 "lua.stx"
          {
	   nvarbuffer = 0; 
           varbuffer[nvarbuffer] = (yyvsp[0].vLong); incr_nvarbuffer();
	   (yyval.vInt) = ((yyvsp[0].vLong) == 0) ? 1 : 0;
	  }
#line 2340 "y.tab.c"
    break;

  case 105: /* varlist1: varlist1 ',' var  */
#line 618 "lua.stx"
          { 
           varbuffer[nvarbuffer] = (yyvsp[0].vLong); incr_nvarbuffer();
	   (yyval.vInt) = ((yyvsp[0].vLong) == 0) ? (yyvsp[-2].vInt) + 1 : (yyvsp[-2].vInt);
	  }
#line 2349 "y.tab.c"
    break;

  case 106: /* var: NAME  */
#line 625 "lua.stx"
          {
	   Word s = lua_findsymbol((yyvsp[0].pChar));
	   int local = lua_localname (s);
	   if (local == -1)	/* global var */
	    (yyval.vLong) = s + 1;		/* return positive value */
           else
	    (yyval.vLong) = -(local+1);		/* return negative value */
	  }
#line 2362 "y.tab.c"
    break;

  case 107: /* $@22: %empty  */
#line 634 "lua.stx"
                    {lua_pushvar ((yyvsp[0].vLong));}
#line 2368 "y.tab.c"
    break;

  case 108: /* var: var $@22 '[' expr1 ']'  */
#line 635 "lua.stx"
          {
	   (yyval.vLong) = 0;		/* indexed variable */
	  }
#line 2376 "y.tab.c"
    break;

  case 109: /* $@23: %empty  */
#line 638 "lua.stx"
                    {lua_pushvar ((yyvsp[0].vLong));}
#line 2382 "y.tab.c"
    break;

  case 110: /* var: var $@23 '.' NAME  */
#line 639 "lua.stx"
          {
	   code_byte(PUSHSTRING);
	   code_word(lua_findconstant((yyvsp[0].pChar))); incr_ntemp();
	   (yyval.vLong) = 0;		/* indexed variable */
	  }
#line 2392 "y.tab.c"
    break;

  case 111: /* localdeclist: NAME  */
#line 646 "lua.stx"
                     {localvar[nlocalvar]=lua_findsymbol((yyvsp[0].pChar)); (yyval.vInt) = 1;}
#line 2398 "y.tab.c"
    break;

  case 112: /* localdeclist: localdeclist ',' NAME  */
#line 648 "lua.stx"
            {
	     localvar[nlocalvar+(yyvsp[-2].vInt)]=lua_findsymbol((yyvsp[0].pChar)); 
	     (yyval.vInt) = (yyvsp[-2].vInt)+1;
	    }
#line 2407 "y.tab.c"
    break;

  case 115: /* setdebug: DEBUG  */
#line 658 "lua.stx"
                  {lua_debug = (yyvsp[0].vInt);}
#line 2413 "y.tab.c"
    break;


#line 2417 "y.tab.c"

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

#line 660 "lua.stx"


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


/* 封装复合赋值的后半段逻辑：运算 + 存储 */
void code_compound_finish(long var_info, int op_code) {
    code_byte(op_code);
    /*判断变量类型并生成存储指令 */
    if (var_info > 0) { /* 全局变量 (符号表索引+1) */
        code_byte(STOREGLOBAL);
        code_word(var_info - 1);
    } else { /* 局部变量 (栈偏移取反) */
        code_byte(STORELOCAL);
        code_byte(-(var_info + 1));
    }
}



// 开启函数编译作用域
void lua_begin_scope(int symbol_idx) {
    // 备份主程序现场
    save_pc = pc;
    save_basepc = basepc;
    save_maxcurr = maxcurr;
    save_nlocalvar = nlocalvar;
    save_ntemp = ntemp;

    // 为匿名函数开辟全新的独立缓冲区
    basepc = (Byte *) calloc(GAPCODE, sizeof(Byte));
    if (basepc == NULL) lua_error("not enough memory");
    
    code = basepc;     // 切换当前写入指针到新内存
    pc = 0;            // 匿名函数的代码从 0 开始
    maxcurr = GAPCODE; // 新缓冲区的容量
    nlocalvar = 0;     // 重置局部变量计数
    ntemp = 0; // 匿名函数内部从 0 开始算临时变量

    // 复用调试逻辑：标记当前正在编译哪个函数
    if (lua_debug) {
        code_byte(SETFUNCTION); 
        code_word(lua_nfile - 1);
        code_word(symbol_idx);
    }
}

// 结束函数编译作用域
void lua_end_scope(int symbol_idx) {
    if (lua_debug) code_byte(RESET); 
    
    // 生成返回指令
    code_byte(RETCODE); 
    code_byte(nlocalvar);
    
    // 物理存储到符号表
    s_tag(symbol_idx) = T_FUNCTION;
    
    // 内存安全：释放该槽位旧的代码
    if (s_bvalue(symbol_idx)) free(s_bvalue(symbol_idx)); 
    
    s_bvalue(symbol_idx) = (Byte *) calloc(pc, sizeof(Byte));
    if (s_bvalue(symbol_idx) == NULL) lua_error("not enough memory");
    
    memcpy(s_bvalue(symbol_idx), basepc, pc * sizeof(Byte));
    
    // 彻底恢复主程序的现场
    basepc = save_basepc;
    pc = save_pc;
    maxcurr = save_maxcurr;
    nlocalvar = save_nlocalvar;
    ntemp = save_ntemp;
    if (basepc == initcode) {
        maincode = pc; 
    }
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

