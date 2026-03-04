
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
static int n_anonymous_f=0;


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


#line 283 "y.tab.c"

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
    FOR = 280,                     /* FOR  */
    AND = 281,                     /* AND  */
    OR = 282,                      /* OR  */
    NE = 283,                      /* NE  */
    LE = 284,                      /* LE  */
    GE = 285,                      /* GE  */
    CONC = 286,                    /* CONC  */
    UNARY = 287,                   /* UNARY  */
    NOT = 288                      /* NOT  */
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
#define FOR 280
#define AND 281
#define OR 282
#define NE 283
#define LE 284
#define GE 285
#define CONC 286
#define UNARY 287
#define NOT 288

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 215 "lua.stx"

 int   vInt;
 long  vLong;
 float vFloat;
 char *pChar;
 Word  vWord;
 Byte *pByte;

#line 411 "y.tab.c"

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
  YYSYMBOL_FOR = 25,                       /* FOR  */
  YYSYMBOL_AND = 26,                       /* AND  */
  YYSYMBOL_OR = 27,                        /* OR  */
  YYSYMBOL_28_ = 28,                       /* '='  */
  YYSYMBOL_NE = 29,                        /* NE  */
  YYSYMBOL_30_ = 30,                       /* '>'  */
  YYSYMBOL_31_ = 31,                       /* '<'  */
  YYSYMBOL_LE = 32,                        /* LE  */
  YYSYMBOL_GE = 33,                        /* GE  */
  YYSYMBOL_CONC = 34,                      /* CONC  */
  YYSYMBOL_35_ = 35,                       /* '+'  */
  YYSYMBOL_36_ = 36,                       /* '-'  */
  YYSYMBOL_37_ = 37,                       /* '*'  */
  YYSYMBOL_38_ = 38,                       /* '/'  */
  YYSYMBOL_UNARY = 39,                     /* UNARY  */
  YYSYMBOL_NOT = 40,                       /* NOT  */
  YYSYMBOL_41_ = 41,                       /* '('  */
  YYSYMBOL_42_ = 42,                       /* ')'  */
  YYSYMBOL_43_ = 43,                       /* ';'  */
  YYSYMBOL_44_ = 44,                       /* ','  */
  YYSYMBOL_45_ = 45,                       /* '@'  */
  YYSYMBOL_46_ = 46,                       /* '{'  */
  YYSYMBOL_47_ = 47,                       /* '}'  */
  YYSYMBOL_48_ = 48,                       /* '['  */
  YYSYMBOL_49_ = 49,                       /* ']'  */
  YYSYMBOL_50_ = 50,                       /* '.'  */
  YYSYMBOL_YYACCEPT = 51,                  /* $accept  */
  YYSYMBOL_functionlist = 52,              /* functionlist  */
  YYSYMBOL_53_1 = 53,                      /* $@1  */
  YYSYMBOL_function = 54,                  /* function  */
  YYSYMBOL_55_2 = 55,                      /* @2  */
  YYSYMBOL_56_3 = 56,                      /* $@3  */
  YYSYMBOL_anonymous_function = 57,        /* anonymous_function  */
  YYSYMBOL_58_4 = 58,                      /* @4  */
  YYSYMBOL_59_5 = 59,                      /* $@5  */
  YYSYMBOL_statlist = 60,                  /* statlist  */
  YYSYMBOL_stat = 61,                      /* stat  */
  YYSYMBOL_62_6 = 62,                      /* $@6  */
  YYSYMBOL_sc = 63,                        /* sc  */
  YYSYMBOL_stat1 = 64,                     /* stat1  */
  YYSYMBOL_65_7 = 65,                      /* @7  */
  YYSYMBOL_66_8 = 66,                      /* @8  */
  YYSYMBOL_67_9 = 67,                      /* @9  */
  YYSYMBOL_68_10 = 68,                     /* @10  */
  YYSYMBOL_69_11 = 69,                     /* @11  */
  YYSYMBOL_70_12 = 70,                     /* $@12  */
  YYSYMBOL_71_13 = 71,                     /* $@13  */
  YYSYMBOL_72_14 = 72,                     /* $@14  */
  YYSYMBOL_73_15 = 73,                     /* $@15  */
  YYSYMBOL_74_16 = 74,                     /* $@16  */
  YYSYMBOL_elsepart = 75,                  /* elsepart  */
  YYSYMBOL_block = 76,                     /* block  */
  YYSYMBOL_77_17 = 77,                     /* @17  */
  YYSYMBOL_78_18 = 78,                     /* $@18  */
  YYSYMBOL_ret = 79,                       /* ret  */
  YYSYMBOL_80_19 = 80,                     /* $@19  */
  YYSYMBOL_PrepJump = 81,                  /* PrepJump  */
  YYSYMBOL_expr1 = 82,                     /* expr1  */
  YYSYMBOL_expr = 83,                      /* expr  */
  YYSYMBOL_84_20 = 84,                     /* $@20  */
  YYSYMBOL_85_21 = 85,                     /* $@21  */
  YYSYMBOL_typeconstructor = 86,           /* typeconstructor  */
  YYSYMBOL_87_22 = 87,                     /* @22  */
  YYSYMBOL_dimension = 88,                 /* dimension  */
  YYSYMBOL_functioncall = 89,              /* functioncall  */
  YYSYMBOL_90_23 = 90,                     /* @23  */
  YYSYMBOL_functionvalue = 91,             /* functionvalue  */
  YYSYMBOL_exprlist = 92,                  /* exprlist  */
  YYSYMBOL_exprlist1 = 93,                 /* exprlist1  */
  YYSYMBOL_94_24 = 94,                     /* $@24  */
  YYSYMBOL_parlist = 95,                   /* parlist  */
  YYSYMBOL_parlist1 = 96,                  /* parlist1  */
  YYSYMBOL_objectname = 97,                /* objectname  */
  YYSYMBOL_fieldlist = 98,                 /* fieldlist  */
  YYSYMBOL_ffieldlist = 99,                /* ffieldlist  */
  YYSYMBOL_ffieldlist1 = 100,              /* ffieldlist1  */
  YYSYMBOL_ffield = 101,                   /* ffield  */
  YYSYMBOL_102_25 = 102,                   /* @25  */
  YYSYMBOL_lfieldlist = 103,               /* lfieldlist  */
  YYSYMBOL_lfieldlist1 = 104,              /* lfieldlist1  */
  YYSYMBOL_varlist1 = 105,                 /* varlist1  */
  YYSYMBOL_var = 106,                      /* var  */
  YYSYMBOL_107_26 = 107,                   /* $@26  */
  YYSYMBOL_108_27 = 108,                   /* $@27  */
  YYSYMBOL_localdeclist = 109,             /* localdeclist  */
  YYSYMBOL_decinit = 110,                  /* decinit  */
  YYSYMBOL_setdebug = 111                  /* setdebug  */
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
#define YYLAST   322

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  51
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  61
/* YYNRULES -- Number of rules.  */
#define YYNRULES  120
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  213

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   288


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
      41,    42,    37,    35,    44,    36,    50,    38,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    43,
      31,    28,    30,     2,    45,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    48,     2,    49,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    46,     2,    47,     2,     2,     2,     2,
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
      25,    26,    27,    29,    32,    33,    34,    39,    40
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   258,   258,   260,   259,   268,   269,   274,   279,   273,
     291,   296,   290,   308,   309,   312,   312,   321,   321,   324,
     343,   343,   353,   353,   361,   365,   384,   388,   360,   408,
     420,   420,   422,   422,   424,   424,   426,   426,   429,   430,
     431,   434,   435,   436,   456,   456,   456,   466,   467,   467,
     476,   482,   485,   486,   487,   488,   489,   490,   491,   492,
     493,   494,   495,   496,   497,   498,   499,   500,   505,   506,
     507,   514,   515,   523,   524,   524,   530,   530,   539,   538,
     570,   571,   574,   574,   577,   578,   581,   582,   585,   586,
     586,   590,   591,   594,   599,   606,   607,   610,   615,   622,
     623,   626,   627,   634,   634,   640,   641,   644,   645,   653,
     659,   666,   676,   676,   680,   680,   688,   689,   696,   697,
     700
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
  "ADDEQ", "SUBEQ", "MULTEQ", "DIVEQ", "FOR", "AND", "OR", "'='", "NE",
  "'>'", "'<'", "LE", "GE", "CONC", "'+'", "'-'", "'*'", "'/'", "UNARY",
  "NOT", "'('", "')'", "';'", "','", "'@'", "'{'", "'}'", "'['", "']'",
  "'.'", "$accept", "functionlist", "$@1", "function", "@2", "$@3",
  "anonymous_function", "@4", "$@5", "statlist", "stat", "$@6", "sc",
  "stat1", "@7", "@8", "@9", "@10", "@11", "$@12", "$@13", "$@14", "$@15",
  "$@16", "elsepart", "block", "@17", "$@18", "ret", "$@19", "PrepJump",
  "expr1", "expr", "$@20", "$@21", "typeconstructor", "@22", "dimension",
  "functioncall", "@23", "functionvalue", "exprlist", "exprlist1", "$@24",
  "parlist", "parlist1", "objectname", "fieldlist", "ffieldlist",
  "ffieldlist1", "ffield", "@25", "lfieldlist", "lfieldlist1", "varlist1",
  "var", "$@26", "$@27", "localdeclist", "decinit", "setdebug", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-147)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-115)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    -147,    15,  -147,    11,  -147,  -147,  -147,  -147,  -147,   -29,
     108,     5,  -147,  -147,    52,  -147,  -147,    17,  -147,    32,
      56,  -147,  -147,  -147,  -147,  -147,  -147,   -22,   120,    33,
    -147,  -147,  -147,    52,    52,    52,     9,    28,   156,  -147,
    -147,  -147,   -17,    52,  -147,  -147,   -20,  -147,  -147,    57,
      34,    52,    59,  -147,  -147,  -147,  -147,    41,    40,  -147,
      49,    50,  -147,  -147,  -147,   183,    53,    52,  -147,  -147,
    -147,    52,    52,    52,    52,    52,    52,    52,    52,    52,
      52,    52,   198,    86,  -147,    52,    80,  -147,    73,    68,
    -147,    -9,    52,   273,    66,   -17,    52,    52,    52,    52,
      52,    92,  -147,    95,  -147,   183,    74,  -147,  -147,  -147,
      69,    69,    69,    69,    69,    69,    99,   -28,   -28,  -147,
    -147,  -147,    52,     4,    66,  -147,    52,    33,   101,    52,
    -147,    76,    66,  -147,   183,   183,   183,   183,   211,  -147,
    -147,  -147,  -147,  -147,    52,    52,  -147,   183,   -29,   107,
     235,    82,  -147,    75,    81,  -147,   183,    77,    84,  -147,
      52,  -147,   116,    35,   284,   284,  -147,  -147,  -147,  -147,
     124,    52,  -147,   112,  -147,   101,  -147,    52,   273,  -147,
    -147,    52,   132,   133,    52,   254,  -147,    52,  -147,   183,
    -147,   169,  -147,  -147,   -29,    52,   135,   183,  -147,  -147,
     183,   109,  -147,   140,  -147,  -147,  -147,    35,  -147,  -147,
    -147,   139,  -147
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       2,     3,     1,     0,   120,    15,     5,     6,     7,    17,
       0,     0,    18,     4,     0,    20,    22,     0,   111,     0,
       0,    78,    85,    16,    39,    38,    82,     0,   109,    91,
      71,    69,    70,     0,     0,     0,     0,    78,     0,    51,
      66,    72,    68,     0,    44,   116,   118,    24,    10,    95,
       0,     0,     0,    30,    32,    34,    36,     0,     0,    93,
       0,    92,    64,    65,    73,     0,    51,    80,    50,    50,
      50,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    13,     0,     0,    40,     0,     0,
      96,     0,    86,    88,    29,   110,     0,     0,     0,     0,
       0,     0,     8,     0,    52,    81,     0,    44,    74,    76,
      53,    56,    55,    54,    57,    58,    63,    59,    60,    61,
      62,    50,     0,    15,   119,   117,     0,    91,    99,   105,
      79,     0,    87,    89,    31,    33,    35,    37,     0,   115,
      44,    94,    67,    50,     0,     0,    44,    50,    17,    47,
       0,     0,   103,     0,   100,   101,   107,     0,   106,    83,
       0,   113,     0,    41,    75,    77,    50,    23,    14,    46,
       0,     0,    11,     0,    97,     0,    98,     0,    90,     9,
      44,     0,     0,     0,    86,     0,    44,     0,   102,   108,
      42,     0,    19,    21,    17,     0,     0,   104,    50,    49,
      25,     0,    44,     0,    12,    50,    26,    41,    44,    43,
      27,     0,    28
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -147,  -147,  -147,  -147,  -147,  -147,  -147,  -147,  -147,  -147,
      31,  -147,  -146,  -147,  -147,  -147,  -147,  -147,  -147,  -147,
    -147,  -147,  -147,  -147,   -52,  -106,  -147,  -147,  -147,  -147,
     -66,   -14,   -13,  -147,  -147,   146,  -147,  -147,   148,  -147,
    -147,   -25,   -44,  -147,    37,  -147,  -147,  -147,  -147,  -147,
     -15,  -147,  -147,  -147,  -147,    -5,  -147,  -147,  -147,  -147,
    -147
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,     1,     5,     6,    11,   140,    22,    89,   186,   123,
       9,    10,    13,    23,    43,    44,    88,   203,   208,   211,
      96,    97,    98,    99,   182,    83,    84,   149,   169,   170,
     107,    65,    39,   144,   145,    40,    49,   106,    41,    50,
      26,   131,   132,   160,    60,    61,    91,   130,   153,   154,
     155,   173,   157,   158,    27,    42,    57,    58,    46,    87,
       7
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      38,   143,   168,   108,   109,    28,    51,    94,    85,    80,
      81,   -45,   -45,    30,    12,     2,   -45,   -45,   -45,    62,
      63,    64,    52,    66,    86,    31,    48,    32,    18,    82,
       8,  -112,     3,  -114,   162,     4,    45,   128,    93,   129,
     166,   124,   180,   181,    33,    34,    29,    95,   199,    35,
      36,    47,    59,   105,    37,   146,    30,   110,   111,   112,
     113,   114,   115,   116,   117,   118,   119,   120,    31,    67,
      32,    18,    93,    48,   190,    92,    90,   163,    18,    93,
     196,   167,   134,   135,   136,   137,   138,    33,    34,   100,
     101,   102,    35,    36,   103,   104,   205,    37,   122,   125,
     183,   126,   210,    77,    78,    79,    80,    81,   147,   127,
     133,   139,   150,    14,   141,   156,   142,    15,   159,    16,
     152,   -48,   174,    17,   172,   175,   176,    18,   177,   179,
     164,   165,   202,    19,    78,    79,    80,    81,   184,   207,
     187,    53,    54,    55,    56,   192,   193,   178,   201,    20,
     206,   204,   212,    21,   148,   209,    24,   185,    25,   194,
     188,   -84,    68,   189,   151,     0,     0,   191,  -112,     0,
    -114,    93,     0,   197,     0,   198,     0,     0,     0,     0,
       0,   200,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,   121,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,     0,     0,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     161,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,     0,     0,     0,     0,     0,   171,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,     0,     0,     0,     0,     0,   195,   -51,
     -51,   -51,   -51,   -51,   -51,   -51,   -51,   -51,   -51,   -51,
     -51,   -51,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81
};

static const yytype_int16 yycheck[] =
{
      14,   107,   148,    69,    70,    10,    28,    51,    28,    37,
      38,     7,     8,     4,    43,     0,    12,    13,    14,    33,
      34,    35,    44,    36,    44,    16,    17,    18,    19,    43,
      19,    48,    17,    50,   140,    20,    19,    46,    51,    48,
     146,    85,     7,     8,    35,    36,    41,    52,   194,    40,
      41,    19,    19,    67,    45,   121,     4,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    16,    41,
      18,    19,    85,    17,   180,    41,    19,   143,    19,    92,
     186,   147,    96,    97,    98,    99,   100,    35,    36,    48,
      50,    42,    40,    41,    44,    42,   202,    45,    12,    19,
     166,    28,   208,    34,    35,    36,    37,    38,   122,    41,
      44,    19,   126,     5,    19,   129,    42,     9,    42,    11,
      19,    14,    47,    15,    42,    44,    49,    19,    44,    13,
     144,   145,   198,    25,    35,    36,    37,    38,    14,   205,
      28,    21,    22,    23,    24,    13,    13,   160,    13,    41,
      10,    42,    13,    45,   123,   207,    10,   171,    10,   184,
     175,    41,     6,   177,   127,    -1,    -1,   181,    48,    -1,
      50,   184,    -1,   187,    -1,     6,    -1,    -1,    -1,    -1,
      -1,   195,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    10,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    -1,    -1,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      49,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    -1,    -1,    -1,    -1,    -1,    44,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    -1,    -1,    -1,    -1,    -1,    44,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,    52,     0,    17,    20,    53,    54,   111,    19,    61,
      62,    55,    43,    63,     5,     9,    11,    15,    19,    25,
      41,    45,    57,    64,    86,    89,    91,   105,   106,    41,
       4,    16,    18,    35,    36,    40,    41,    45,    82,    83,
      86,    89,   106,    65,    66,    19,   109,    19,    17,    87,
      90,    28,    44,    21,    22,    23,    24,   107,   108,    19,
      95,    96,    82,    82,    82,    82,    83,    41,     6,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    82,    76,    77,    28,    44,   110,    67,    58,
      19,    97,    41,    83,    93,   106,    71,    72,    73,    74,
      48,    50,    42,    44,    42,    82,    88,    81,    81,    81,
      82,    82,    82,    82,    82,    82,    82,    82,    82,    82,
      82,    10,    12,    60,    93,    19,    28,    41,    46,    48,
      98,    92,    93,    44,    82,    82,    82,    82,    82,    19,
      56,    19,    42,    76,    84,    85,    81,    82,    61,    78,
      82,    95,    19,    99,   100,   101,    82,   103,   104,    42,
      94,    49,    76,    81,    82,    82,    76,    81,    63,    79,
      80,    44,    42,   102,    47,    44,    49,    44,    83,    13,
       7,     8,    75,    81,    14,    82,    59,    28,   101,    82,
      76,    82,    13,    13,    92,    44,    76,    82,     6,    63,
      82,    13,    81,    68,    42,    76,    10,    81,    69,    75,
      76,    70,    13
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    51,    52,    53,    52,    52,    52,    55,    56,    54,
      58,    59,    57,    60,    60,    62,    61,    63,    63,    64,
      65,    64,    66,    64,    67,    68,    69,    70,    64,    64,
      71,    64,    72,    64,    73,    64,    74,    64,    64,    64,
      64,    75,    75,    75,    77,    78,    76,    79,    80,    79,
      81,    82,    83,    83,    83,    83,    83,    83,    83,    83,
      83,    83,    83,    83,    83,    83,    83,    83,    83,    83,
      83,    83,    83,    83,    84,    83,    85,    83,    87,    86,
      88,    88,    90,    89,    91,    91,    92,    92,    93,    94,
      93,    95,    95,    96,    96,    97,    97,    98,    98,    99,
      99,   100,   100,   102,   101,   103,   103,   104,   104,   105,
     105,   106,   107,   106,   108,   106,   109,   109,   110,   110,
     111
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     0,     4,     2,     2,     0,     0,     9,
       0,     0,    10,     0,     3,     0,     2,     0,     1,     8,
       0,     8,     0,     6,     0,     0,     0,     0,    15,     3,
       0,     4,     0,     4,     0,     4,     0,     4,     1,     1,
       3,     0,     2,     7,     0,     0,     4,     0,     0,     4,
       0,     1,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     2,     2,     1,     4,     1,     1,
       1,     1,     1,     2,     0,     5,     0,     5,     0,     4,
       0,     1,     0,     5,     1,     1,     0,     1,     1,     0,
       4,     0,     1,     1,     3,     0,     1,     3,     3,     0,
       1,     1,     3,     0,     4,     0,     1,     1,     3,     1,
       3,     1,     0,     5,     0,     4,     1,     3,     0,     2,
       1
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
#line 260 "lua.stx"
                {
	  	  pc=maincode; basepc=initcode; maxcurr=maxmain;
		  nlocalvar=0;
	        }
#line 1681 "y.tab.c"
    break;

  case 4: /* functionlist: functionlist $@1 stat sc  */
#line 265 "lua.stx"
                {
		  maincode=pc; initcode=basepc; maxmain=maxcurr;
		}
#line 1689 "y.tab.c"
    break;

  case 7: /* @2: %empty  */
#line 274 "lua.stx"
               {
                (yyval.vWord) = lua_findsymbol((yyvsp[0].pChar)); 
                lua_begin_scope((yyval.vWord));
	       }
#line 1698 "y.tab.c"
    break;

  case 8: /* $@3: %empty  */
#line 279 "lua.stx"
           { 
                lua_codeadjust(0); 
            }
#line 1706 "y.tab.c"
    break;

  case 9: /* function: FUNCTION NAME @2 '(' parlist ')' $@3 block END  */
#line 284 "lua.stx"
               {
            lua_end_scope((yyvsp[-6].vWord));
	       }
#line 1714 "y.tab.c"
    break;

  case 10: /* @4: %empty  */
#line 291 "lua.stx"
            {
                (yyval.vWord) = lua_findsymbol("__ANONYMOUS_FUN"+(++n_anonymous_f)); 
                lua_begin_scope((yyval.vWord));
            }
#line 1723 "y.tab.c"
    break;

  case 11: /* $@5: %empty  */
#line 296 "lua.stx"
            { 
                lua_codeadjust(0); 
            }
#line 1731 "y.tab.c"
    break;

  case 12: /* anonymous_function: '(' FUNCTION @4 '(' parlist ')' $@5 block END ')'  */
#line 301 "lua.stx"
               { 
                lua_end_scope((yyvsp[-7].vWord)); 
                // 定义完立刻压栈，供 functioncall 使用
                lua_pushvar((yyvsp[-7].vWord)+1);
	       }
#line 1741 "y.tab.c"
    break;

  case 15: /* $@6: %empty  */
#line 312 "lua.stx"
           {
            ntemp = 0; 
            if (lua_debug)
            {
             code_byte(SETLINE); code_word(lua_linenumber);
            }
	   }
#line 1753 "y.tab.c"
    break;

  case 19: /* stat1: IF expr1 THEN PrepJump block PrepJump elsepart END  */
#line 325 "lua.stx"
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
#line 1775 "y.tab.c"
    break;

  case 20: /* @7: %empty  */
#line 343 "lua.stx"
               {(yyval.vWord)=pc;}
#line 1781 "y.tab.c"
    break;

  case 21: /* stat1: WHILE @7 expr1 DO PrepJump block PrepJump END  */
#line 345 "lua.stx"
       {
        basepc[(yyvsp[-3].vWord)] = IFFJMP;
	code_word_at(basepc+(yyvsp[-3].vWord)+1, pc - ((yyvsp[-3].vWord) + sizeof(Word)+1));
        
        basepc[(yyvsp[-1].vWord)] = UPJMP;
	code_word_at(basepc+(yyvsp[-1].vWord)+1, pc - ((yyvsp[-6].vWord)));
       }
#line 1793 "y.tab.c"
    break;

  case 22: /* @8: %empty  */
#line 353 "lua.stx"
                {(yyval.vWord)=pc;}
#line 1799 "y.tab.c"
    break;

  case 23: /* stat1: REPEAT @8 block UNTIL expr1 PrepJump  */
#line 355 "lua.stx"
       {
        basepc[(yyvsp[0].vWord)] = IFFUPJMP;
	code_word_at(basepc+(yyvsp[0].vWord)+1, pc - ((yyvsp[-4].vWord)));
       }
#line 1808 "y.tab.c"
    break;

  case 24: /* @9: %empty  */
#line 361 "lua.stx"
       {
            (yyval.vWord) = lua_findsymbol((yyvsp[0].pChar));
        }
#line 1816 "y.tab.c"
    break;

  case 25: /* @10: %empty  */
#line 365 "lua.stx"
         {
           
           Word varsymbol = (yyvsp[-6].vWord);
           
           int slot = nlocalvar;//当前局部变量的个数

           localvar[nlocalvar + 0] = varsymbol;//保存局部初值、限制、步长
           localvar[nlocalvar + 1] = lua_findsymbol("_for_limit");
           localvar[nlocalvar + 2] = lua_findsymbol("_for_step");
           add_nlocalvar(3);

           ntemp -= 3;

           code_byte(FORPREP);
           code_byte((Byte)slot);
           (yyval.vWord) = pc;    // 记录 skip_offset 占位的位置
           code_word(0);     
         }
#line 1839 "y.tab.c"
    break;

  case 26: /* @11: %empty  */
#line 384 "lua.stx"
         {
           (yyval.vWord) = pc;    // loop_head = 循环体第一条指令的 pc 
         }
#line 1847 "y.tab.c"
    break;

  case 27: /* $@12: %empty  */
#line 388 "lua.stx"
         {
           Word skip_pos  = (yyvsp[-3].vWord);  
           Word loop_head = (yyvsp[-1].vWord); 
           int  slot      = nlocalvar - 3;

           Word back_offset = (pc + 4) - loop_head;

           code_byte(FORLOOP);
           code_byte((Byte)slot);
           code_word(back_offset);

           code_word_at(basepc + skip_pos,
                        pc - (skip_pos + (Word)sizeof(Word)));
         }
#line 1866 "y.tab.c"
    break;

  case 28: /* stat1: FOR NAME @9 '=' expr1 ',' expr1 ',' expr1 @10 DO @11 block $@12 END  */
#line 403 "lua.stx"
         {
           nlocalvar -= 3;
           lua_codeadjust(0);
         }
#line 1875 "y.tab.c"
    break;

  case 29: /* stat1: varlist1 '=' exprlist1  */
#line 409 "lua.stx"
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
#line 1891 "y.tab.c"
    break;

  case 30: /* $@13: %empty  */
#line 420 "lua.stx"
                   { lua_pushvar((yyvsp[-1].vLong)); }
#line 1897 "y.tab.c"
    break;

  case 31: /* stat1: var ADDEQ $@13 expr1  */
#line 421 "lua.stx"
        { code_compound_finish((yyvsp[-3].vLong), ADDOP); ntemp--; }
#line 1903 "y.tab.c"
    break;

  case 32: /* $@14: %empty  */
#line 422 "lua.stx"
                   { lua_pushvar((yyvsp[-1].vLong)); }
#line 1909 "y.tab.c"
    break;

  case 33: /* stat1: var SUBEQ $@14 expr1  */
#line 423 "lua.stx"
        { code_compound_finish((yyvsp[-3].vLong), SUBOP); ntemp--; }
#line 1915 "y.tab.c"
    break;

  case 34: /* $@15: %empty  */
#line 424 "lua.stx"
                    { lua_pushvar((yyvsp[-1].vLong)); }
#line 1921 "y.tab.c"
    break;

  case 35: /* stat1: var MULTEQ $@15 expr1  */
#line 425 "lua.stx"
        { code_compound_finish((yyvsp[-3].vLong), MULTOP); ntemp--; }
#line 1927 "y.tab.c"
    break;

  case 36: /* $@16: %empty  */
#line 426 "lua.stx"
                   { lua_pushvar((yyvsp[-1].vLong)); }
#line 1933 "y.tab.c"
    break;

  case 37: /* stat1: var DIVEQ $@16 expr1  */
#line 427 "lua.stx"
        { code_compound_finish((yyvsp[-3].vLong), DIVOP); ntemp--; }
#line 1939 "y.tab.c"
    break;

  case 38: /* stat1: functioncall  */
#line 429 "lua.stx"
                                        { lua_codeadjust (0); }
#line 1945 "y.tab.c"
    break;

  case 39: /* stat1: typeconstructor  */
#line 430 "lua.stx"
                                        { lua_codeadjust (0); }
#line 1951 "y.tab.c"
    break;

  case 40: /* stat1: LOCAL localdeclist decinit  */
#line 431 "lua.stx"
                                      { add_nlocalvar((yyvsp[-1].vInt)); lua_codeadjust (0); }
#line 1957 "y.tab.c"
    break;

  case 43: /* elsepart: ELSEIF expr1 THEN PrepJump block PrepJump elsepart  */
#line 437 "lua.stx"
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
#line 1979 "y.tab.c"
    break;

  case 44: /* @17: %empty  */
#line 456 "lua.stx"
           {(yyval.vInt) = nlocalvar;}
#line 1985 "y.tab.c"
    break;

  case 45: /* $@18: %empty  */
#line 456 "lua.stx"
                                            {ntemp = 0;}
#line 1991 "y.tab.c"
    break;

  case 46: /* block: @17 statlist $@18 ret  */
#line 457 "lua.stx"
         {
	  if (nlocalvar != (yyvsp[-3].vInt))
	  {
           nlocalvar = (yyvsp[-3].vInt);
	   lua_codeadjust (0);
	  }
         }
#line 2003 "y.tab.c"
    break;

  case 48: /* $@19: %empty  */
#line 467 "lua.stx"
          { if (lua_debug){code_byte(SETLINE);code_word(lua_linenumber);}}
#line 2009 "y.tab.c"
    break;

  case 49: /* ret: $@19 RETURN exprlist sc  */
#line 469 "lua.stx"
          { 
           if (lua_debug) code_byte(RESET); 
           code_byte(RETCODE); code_byte(nlocalvar);
          }
#line 2018 "y.tab.c"
    break;

  case 50: /* PrepJump: %empty  */
#line 476 "lua.stx"
         { 
	  (yyval.vWord) = pc;
	  code_byte(0);		/* open space */
	  code_word (0);
         }
#line 2028 "y.tab.c"
    break;

  case 51: /* expr1: expr  */
#line 482 "lua.stx"
                { if ((yyvsp[0].vInt) == 0) {lua_codeadjust (ntemp+1); incr_ntemp();}}
#line 2034 "y.tab.c"
    break;

  case 52: /* expr: '(' expr ')'  */
#line 485 "lua.stx"
                        { (yyval.vInt) = (yyvsp[-1].vInt); }
#line 2040 "y.tab.c"
    break;

  case 53: /* expr: expr1 '=' expr1  */
#line 486 "lua.stx"
                        { code_byte(EQOP);   (yyval.vInt) = 1; ntemp--;}
#line 2046 "y.tab.c"
    break;

  case 54: /* expr: expr1 '<' expr1  */
#line 487 "lua.stx"
                        { code_byte(LTOP);   (yyval.vInt) = 1; ntemp--;}
#line 2052 "y.tab.c"
    break;

  case 55: /* expr: expr1 '>' expr1  */
#line 488 "lua.stx"
                        { code_byte(LEOP); code_byte(NOTOP); (yyval.vInt) = 1; ntemp--;}
#line 2058 "y.tab.c"
    break;

  case 56: /* expr: expr1 NE expr1  */
#line 489 "lua.stx"
                        { code_byte(EQOP); code_byte(NOTOP); (yyval.vInt) = 1; ntemp--;}
#line 2064 "y.tab.c"
    break;

  case 57: /* expr: expr1 LE expr1  */
#line 490 "lua.stx"
                        { code_byte(LEOP);   (yyval.vInt) = 1; ntemp--;}
#line 2070 "y.tab.c"
    break;

  case 58: /* expr: expr1 GE expr1  */
#line 491 "lua.stx"
                        { code_byte(LTOP); code_byte(NOTOP); (yyval.vInt) = 1; ntemp--;}
#line 2076 "y.tab.c"
    break;

  case 59: /* expr: expr1 '+' expr1  */
#line 492 "lua.stx"
                        { code_byte(ADDOP);  (yyval.vInt) = 1; ntemp--;}
#line 2082 "y.tab.c"
    break;

  case 60: /* expr: expr1 '-' expr1  */
#line 493 "lua.stx"
                        { code_byte(SUBOP);  (yyval.vInt) = 1; ntemp--;}
#line 2088 "y.tab.c"
    break;

  case 61: /* expr: expr1 '*' expr1  */
#line 494 "lua.stx"
                        { code_byte(MULTOP); (yyval.vInt) = 1; ntemp--;}
#line 2094 "y.tab.c"
    break;

  case 62: /* expr: expr1 '/' expr1  */
#line 495 "lua.stx"
                        { code_byte(DIVOP);  (yyval.vInt) = 1; ntemp--;}
#line 2100 "y.tab.c"
    break;

  case 63: /* expr: expr1 CONC expr1  */
#line 496 "lua.stx"
                         { code_byte(CONCOP);  (yyval.vInt) = 1; ntemp--;}
#line 2106 "y.tab.c"
    break;

  case 64: /* expr: '+' expr1  */
#line 497 "lua.stx"
                                { (yyval.vInt) = 1; }
#line 2112 "y.tab.c"
    break;

  case 65: /* expr: '-' expr1  */
#line 498 "lua.stx"
                                { code_byte(MINUSOP); (yyval.vInt) = 1;}
#line 2118 "y.tab.c"
    break;

  case 66: /* expr: typeconstructor  */
#line 499 "lua.stx"
                       { (yyval.vInt) = (yyvsp[0].vInt); }
#line 2124 "y.tab.c"
    break;

  case 67: /* expr: '@' '(' dimension ')'  */
#line 501 "lua.stx"
     { 
      code_byte(CREATEARRAY);
      (yyval.vInt) = 1;
     }
#line 2133 "y.tab.c"
    break;

  case 68: /* expr: var  */
#line 505 "lua.stx"
                        { lua_pushvar ((yyvsp[0].vLong)); (yyval.vInt) = 1;}
#line 2139 "y.tab.c"
    break;

  case 69: /* expr: NUMBER  */
#line 506 "lua.stx"
                        { code_number((yyvsp[0].vFloat)); (yyval.vInt) = 1; }
#line 2145 "y.tab.c"
    break;

  case 70: /* expr: STRING  */
#line 508 "lua.stx"
     {
      code_byte(PUSHSTRING);
      code_word((yyvsp[0].vWord));
      (yyval.vInt) = 1;
      incr_ntemp();
     }
#line 2156 "y.tab.c"
    break;

  case 71: /* expr: NIL  */
#line 514 "lua.stx"
                        {code_byte(PUSHNIL); (yyval.vInt) = 1; incr_ntemp();}
#line 2162 "y.tab.c"
    break;

  case 72: /* expr: functioncall  */
#line 516 "lua.stx"
     {
      (yyval.vInt) = 0;
      if (lua_debug)
      {
       code_byte(SETLINE); code_word(lua_linenumber);
      }
     }
#line 2174 "y.tab.c"
    break;

  case 73: /* expr: NOT expr1  */
#line 523 "lua.stx"
                        { code_byte(NOTOP);  (yyval.vInt) = 1;}
#line 2180 "y.tab.c"
    break;

  case 74: /* $@20: %empty  */
#line 524 "lua.stx"
                           {code_byte(POP); ntemp--;}
#line 2186 "y.tab.c"
    break;

  case 75: /* expr: expr1 AND PrepJump $@20 expr1  */
#line 525 "lua.stx"
     { 
      basepc[(yyvsp[-2].vWord)] = ONFJMP;
      code_word_at(basepc+(yyvsp[-2].vWord)+1, pc - ((yyvsp[-2].vWord) + sizeof(Word)+1));
      (yyval.vInt) = 1;
     }
#line 2196 "y.tab.c"
    break;

  case 76: /* $@21: %empty  */
#line 530 "lua.stx"
                          {code_byte(POP); ntemp--;}
#line 2202 "y.tab.c"
    break;

  case 77: /* expr: expr1 OR PrepJump $@21 expr1  */
#line 531 "lua.stx"
     { 
      basepc[(yyvsp[-2].vWord)] = ONTJMP;
      code_word_at(basepc+(yyvsp[-2].vWord)+1, pc - ((yyvsp[-2].vWord) + sizeof(Word)+1));
      (yyval.vInt) = 1;
     }
#line 2212 "y.tab.c"
    break;

  case 78: /* @22: %empty  */
#line 539 "lua.stx"
     {
      code_byte(PUSHBYTE);
      (yyval.vWord) = pc; code_byte(0);
      incr_ntemp();
      code_byte(CREATEARRAY);
     }
#line 2223 "y.tab.c"
    break;

  case 79: /* typeconstructor: '@' @22 objectname fieldlist  */
#line 546 "lua.stx"
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
#line 2250 "y.tab.c"
    break;

  case 80: /* dimension: %empty  */
#line 570 "lua.stx"
                                { code_byte(PUSHNIL); incr_ntemp();}
#line 2256 "y.tab.c"
    break;

  case 82: /* @23: %empty  */
#line 574 "lua.stx"
                              {code_byte(PUSHMARK); (yyval.vInt) = ntemp; incr_ntemp();}
#line 2262 "y.tab.c"
    break;

  case 83: /* functioncall: functionvalue @23 '(' exprlist ')'  */
#line 575 "lua.stx"
                                 { code_byte(CALLFUNC); ntemp = (yyvsp[-3].vInt)-1;}
#line 2268 "y.tab.c"
    break;

  case 84: /* functionvalue: var  */
#line 577 "lua.stx"
                    {lua_pushvar ((yyvsp[0].vLong)); }
#line 2274 "y.tab.c"
    break;

  case 86: /* exprlist: %empty  */
#line 581 "lua.stx"
                                        { (yyval.vInt) = 1; }
#line 2280 "y.tab.c"
    break;

  case 87: /* exprlist: exprlist1  */
#line 582 "lua.stx"
                                        { (yyval.vInt) = (yyvsp[0].vInt); }
#line 2286 "y.tab.c"
    break;

  case 88: /* exprlist1: expr  */
#line 585 "lua.stx"
                                        { (yyval.vInt) = (yyvsp[0].vInt); }
#line 2292 "y.tab.c"
    break;

  case 89: /* $@24: %empty  */
#line 586 "lua.stx"
                              {if (!(yyvsp[-1].vInt)){lua_codeadjust (ntemp+1); incr_ntemp();}}
#line 2298 "y.tab.c"
    break;

  case 90: /* exprlist1: exprlist1 ',' $@24 expr  */
#line 587 "lua.stx"
                      {(yyval.vInt) = (yyvsp[0].vInt);}
#line 2304 "y.tab.c"
    break;

  case 93: /* parlist1: NAME  */
#line 595 "lua.stx"
                {
		 localvar[nlocalvar]=lua_findsymbol((yyvsp[0].pChar)); 
		 add_nlocalvar(1);
		}
#line 2313 "y.tab.c"
    break;

  case 94: /* parlist1: parlist1 ',' NAME  */
#line 600 "lua.stx"
                {
		 localvar[nlocalvar]=lua_findsymbol((yyvsp[0].pChar)); 
		 add_nlocalvar(1);
		}
#line 2322 "y.tab.c"
    break;

  case 95: /* objectname: %empty  */
#line 606 "lua.stx"
                                {(yyval.vLong)=-1;}
#line 2328 "y.tab.c"
    break;

  case 96: /* objectname: NAME  */
#line 607 "lua.stx"
                                {(yyval.vLong)=lua_findsymbol((yyvsp[0].pChar));}
#line 2334 "y.tab.c"
    break;

  case 97: /* fieldlist: '{' ffieldlist '}'  */
#line 611 "lua.stx"
              { 
	       flush_record((yyvsp[-1].vInt)%FIELDS_PER_FLUSH); 
	       (yyval.vInt) = (yyvsp[-1].vInt);
	      }
#line 2343 "y.tab.c"
    break;

  case 98: /* fieldlist: '[' lfieldlist ']'  */
#line 616 "lua.stx"
              { 
	       flush_list((yyvsp[-1].vInt)/FIELDS_PER_FLUSH, (yyvsp[-1].vInt)%FIELDS_PER_FLUSH);
	       (yyval.vInt) = (yyvsp[-1].vInt);
     	      }
#line 2352 "y.tab.c"
    break;

  case 99: /* ffieldlist: %empty  */
#line 622 "lua.stx"
                           { (yyval.vInt) = 0; }
#line 2358 "y.tab.c"
    break;

  case 100: /* ffieldlist: ffieldlist1  */
#line 623 "lua.stx"
                           { (yyval.vInt) = (yyvsp[0].vInt); }
#line 2364 "y.tab.c"
    break;

  case 101: /* ffieldlist1: ffield  */
#line 626 "lua.stx"
                                        {(yyval.vInt)=1;}
#line 2370 "y.tab.c"
    break;

  case 102: /* ffieldlist1: ffieldlist1 ',' ffield  */
#line 628 "lua.stx"
                {
		  (yyval.vInt)=(yyvsp[-2].vInt)+1;
		  if ((yyval.vInt)%FIELDS_PER_FLUSH == 0) flush_record(FIELDS_PER_FLUSH);
		}
#line 2379 "y.tab.c"
    break;

  case 103: /* @25: %empty  */
#line 634 "lua.stx"
                   {(yyval.vWord) = lua_findconstant((yyvsp[0].pChar));}
#line 2385 "y.tab.c"
    break;

  case 104: /* ffield: NAME @25 '=' expr1  */
#line 635 "lua.stx"
              { 
	       push_field((yyvsp[-2].vWord));
	      }
#line 2393 "y.tab.c"
    break;

  case 105: /* lfieldlist: %empty  */
#line 640 "lua.stx"
                           { (yyval.vInt) = 0; }
#line 2399 "y.tab.c"
    break;

  case 106: /* lfieldlist: lfieldlist1  */
#line 641 "lua.stx"
                           { (yyval.vInt) = (yyvsp[0].vInt); }
#line 2405 "y.tab.c"
    break;

  case 107: /* lfieldlist1: expr1  */
#line 644 "lua.stx"
                     {(yyval.vInt)=1;}
#line 2411 "y.tab.c"
    break;

  case 108: /* lfieldlist1: lfieldlist1 ',' expr1  */
#line 646 "lua.stx"
                {
		  (yyval.vInt)=(yyvsp[-2].vInt)+1;
		  if ((yyval.vInt)%FIELDS_PER_FLUSH == 0) 
		    flush_list((yyval.vInt)/FIELDS_PER_FLUSH - 1, FIELDS_PER_FLUSH);
		}
#line 2421 "y.tab.c"
    break;

  case 109: /* varlist1: var  */
#line 654 "lua.stx"
          {
	   nvarbuffer = 0; 
           varbuffer[nvarbuffer] = (yyvsp[0].vLong); incr_nvarbuffer();
	   (yyval.vInt) = ((yyvsp[0].vLong) == 0) ? 1 : 0;
	  }
#line 2431 "y.tab.c"
    break;

  case 110: /* varlist1: varlist1 ',' var  */
#line 660 "lua.stx"
          { 
           varbuffer[nvarbuffer] = (yyvsp[0].vLong); incr_nvarbuffer();
	   (yyval.vInt) = ((yyvsp[0].vLong) == 0) ? (yyvsp[-2].vInt) + 1 : (yyvsp[-2].vInt);
	  }
#line 2440 "y.tab.c"
    break;

  case 111: /* var: NAME  */
#line 667 "lua.stx"
          {
	   Word s = lua_findsymbol((yyvsp[0].pChar));
	   int local = lua_localname (s);
	   if (local == -1)	/* global var */
	    (yyval.vLong) = s + 1;		/* return positive value */
           else
	    (yyval.vLong) = -(local+1);		/* return negative value */
	  }
#line 2453 "y.tab.c"
    break;

  case 112: /* $@26: %empty  */
#line 676 "lua.stx"
                    {lua_pushvar ((yyvsp[0].vLong));}
#line 2459 "y.tab.c"
    break;

  case 113: /* var: var $@26 '[' expr1 ']'  */
#line 677 "lua.stx"
          {
	   (yyval.vLong) = 0;		/* indexed variable */
	  }
#line 2467 "y.tab.c"
    break;

  case 114: /* $@27: %empty  */
#line 680 "lua.stx"
                    {lua_pushvar ((yyvsp[0].vLong));}
#line 2473 "y.tab.c"
    break;

  case 115: /* var: var $@27 '.' NAME  */
#line 681 "lua.stx"
          {
	   code_byte(PUSHSTRING);
	   code_word(lua_findconstant((yyvsp[0].pChar))); incr_ntemp();
	   (yyval.vLong) = 0;		/* indexed variable */
	  }
#line 2483 "y.tab.c"
    break;

  case 116: /* localdeclist: NAME  */
#line 688 "lua.stx"
                     {localvar[nlocalvar]=lua_findsymbol((yyvsp[0].pChar)); (yyval.vInt) = 1;}
#line 2489 "y.tab.c"
    break;

  case 117: /* localdeclist: localdeclist ',' NAME  */
#line 690 "lua.stx"
            {
	     localvar[nlocalvar+(yyvsp[-2].vInt)]=lua_findsymbol((yyvsp[0].pChar)); 
	     (yyval.vInt) = (yyvsp[-2].vInt)+1;
	    }
#line 2498 "y.tab.c"
    break;

  case 120: /* setdebug: DEBUG  */
#line 700 "lua.stx"
                  {lua_debug = (yyvsp[0].vInt);}
#line 2504 "y.tab.c"
    break;


#line 2508 "y.tab.c"

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

#line 702 "lua.stx"


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
    
    basepc = code;     // 切换当前写入指针到新内存
    pc = 0;            // 匿名函数的代码从 0 开始
    maxcurr = maxcode; // 新缓冲区的容量
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
    
    s_bvalue(symbol_idx) = (Byte *) calloc(pc, sizeof(Byte));
    if (s_bvalue(symbol_idx) == NULL) {
        lua_error("not enough memory");
        err = 1;
    }
    
    memcpy(s_bvalue(symbol_idx), basepc, pc * sizeof(Byte));
    
    code=basepc;
    maxcode=maxcurr;

#if LISTING
PrintCode(code,code+pc);
#endif

    // 彻底恢复主程序的现场
    basepc = save_basepc;
    pc = save_pc;
    maxcurr = save_maxcurr;
    nlocalvar = save_nlocalvar;
    ntemp = save_ntemp;
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
            case FORPREP:
                {
                    int slot = (int)(*(++p)); p++;   /* 读 slot，p前进1 */
                    CodeWord c;
                    get_word(c, p);                  /* 读 skip，p前进2 */
                    printf("%ld    FORPREP   slot=%d  skip=%d\n", n, slot, (int)c.w);
                }
                break;
            case FORLOOP:
                {
                    int slot = (int)(*(++p)); p++;   /* 读 slot，p前进1 */
                    CodeWord c;
                    get_word(c, p);                  /* 读 back，p前进2 */
                    printf("%ld    FORLOOP   slot=%d  back=%d\n", n, slot, (int)c.w);
                }
                break;
            case RESET:         printf ("%ld    RESET\n", n); p++; break;
            default:            printf ("%ld    Cannot happen: code %d\n", n, (int)(*p)); p++; break;
        }
    }
}
#endif

