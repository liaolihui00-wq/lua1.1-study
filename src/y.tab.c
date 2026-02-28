
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


#line 271 "y.tab.c"

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
#line 203 "lua.stx"

 int   vInt;
 long  vLong;
 float vFloat;
 char *pChar;
 Word  vWord;
 Byte *pByte;

#line 397 "y.tab.c"

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
  YYSYMBOL_statlist = 56,                  /* statlist  */
  YYSYMBOL_stat = 57,                      /* stat  */
  YYSYMBOL_58_4 = 58,                      /* $@4  */
  YYSYMBOL_sc = 59,                        /* sc  */
  YYSYMBOL_stat1 = 60,                     /* stat1  */
  YYSYMBOL_61_5 = 61,                      /* @5  */
  YYSYMBOL_62_6 = 62,                      /* @6  */
  YYSYMBOL_63_7 = 63,                      /* $@7  */
  YYSYMBOL_64_8 = 64,                      /* $@8  */
  YYSYMBOL_65_9 = 65,                      /* $@9  */
  YYSYMBOL_66_10 = 66,                     /* $@10  */
  YYSYMBOL_elsepart = 67,                  /* elsepart  */
  YYSYMBOL_block = 68,                     /* block  */
  YYSYMBOL_69_11 = 69,                     /* @11  */
  YYSYMBOL_70_12 = 70,                     /* $@12  */
  YYSYMBOL_ret = 71,                       /* ret  */
  YYSYMBOL_72_13 = 72,                     /* $@13  */
  YYSYMBOL_PrepJump = 73,                  /* PrepJump  */
  YYSYMBOL_expr1 = 74,                     /* expr1  */
  YYSYMBOL_expr = 75,                      /* expr  */
  YYSYMBOL_76_14 = 76,                     /* $@14  */
  YYSYMBOL_77_15 = 77,                     /* $@15  */
  YYSYMBOL_typeconstructor = 78,           /* typeconstructor  */
  YYSYMBOL_79_16 = 79,                     /* @16  */
  YYSYMBOL_dimension = 80,                 /* dimension  */
  YYSYMBOL_functioncall = 81,              /* functioncall  */
  YYSYMBOL_82_17 = 82,                     /* @17  */
  YYSYMBOL_functionvalue = 83,             /* functionvalue  */
  YYSYMBOL_exprlist = 84,                  /* exprlist  */
  YYSYMBOL_exprlist1 = 85,                 /* exprlist1  */
  YYSYMBOL_86_18 = 86,                     /* $@18  */
  YYSYMBOL_parlist = 87,                   /* parlist  */
  YYSYMBOL_parlist1 = 88,                  /* parlist1  */
  YYSYMBOL_objectname = 89,                /* objectname  */
  YYSYMBOL_fieldlist = 90,                 /* fieldlist  */
  YYSYMBOL_ffieldlist = 91,                /* ffieldlist  */
  YYSYMBOL_ffieldlist1 = 92,               /* ffieldlist1  */
  YYSYMBOL_ffield = 93,                    /* ffield  */
  YYSYMBOL_94_19 = 94,                     /* @19  */
  YYSYMBOL_lfieldlist = 95,                /* lfieldlist  */
  YYSYMBOL_lfieldlist1 = 96,               /* lfieldlist1  */
  YYSYMBOL_varlist1 = 97,                  /* varlist1  */
  YYSYMBOL_var = 98,                       /* var  */
  YYSYMBOL_99_20 = 99,                     /* $@20  */
  YYSYMBOL_100_21 = 100,                   /* $@21  */
  YYSYMBOL_localdeclist = 101,             /* localdeclist  */
  YYSYMBOL_decinit = 102,                  /* decinit  */
  YYSYMBOL_setdebug = 103                  /* setdebug  */
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
#define YYLAST   238

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  50
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  54
/* YYNRULES -- Number of rules.  */
#define YYNRULES  111
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  187

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
       0,   245,   245,   247,   246,   255,   256,   260,   276,   259,
     305,   306,   309,   309,   318,   318,   321,   340,   340,   350,
     350,   358,   370,   370,   372,   372,   374,   374,   376,   376,
     379,   380,   381,   384,   385,   386,   406,   406,   406,   416,
     417,   417,   426,   432,   435,   436,   437,   438,   439,   440,
     441,   442,   443,   444,   445,   446,   447,   448,   449,   450,
     455,   456,   457,   464,   465,   473,   474,   474,   480,   480,
     489,   488,   520,   521,   524,   524,   527,   530,   531,   534,
     535,   535,   539,   540,   543,   548,   555,   556,   559,   564,
     571,   572,   575,   576,   583,   583,   589,   590,   593,   594,
     602,   608,   615,   625,   625,   629,   629,   637,   638,   645,
     646,   649
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
  "$accept", "functionlist", "$@1", "function", "@2", "$@3", "statlist",
  "stat", "$@4", "sc", "stat1", "@5", "@6", "$@7", "$@8", "$@9", "$@10",
  "elsepart", "block", "@11", "$@12", "ret", "$@13", "PrepJump", "expr1",
  "expr", "$@14", "$@15", "typeconstructor", "@16", "dimension",
  "functioncall", "@17", "functionvalue", "exprlist", "exprlist1", "$@18",
  "parlist", "parlist1", "objectname", "fieldlist", "ffieldlist",
  "ffieldlist1", "ffield", "@19", "lfieldlist", "lfieldlist1", "varlist1",
  "var", "$@20", "$@21", "localdeclist", "decinit", "setdebug", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-139)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-106)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    -139,     7,  -139,     1,  -139,  -139,  -139,  -139,  -139,     4,
       0,     2,  -139,  -139,    31,  -139,  -139,     9,  -139,  -139,
    -139,  -139,  -139,  -139,   -21,   102,    32,  -139,  -139,  -139,
      31,    31,    31,    31,    24,    82,  -139,  -139,  -139,   -18,
      31,  -139,  -139,   -19,    48,    40,    31,    53,  -139,  -139,
    -139,  -139,    34,    33,  -139,    43,    42,  -139,  -139,  -139,
     149,    59,    31,  -139,  -139,  -139,    31,    31,    31,    31,
      31,    31,    31,    31,    31,    31,    31,   163,    90,  -139,
      31,    84,  -139,  -139,   -35,    31,   200,    61,   -17,    31,
      31,    31,    31,    31,   101,  -139,   108,  -139,   149,    87,
    -139,  -139,  -139,   181,   181,   181,   181,   181,   181,   185,
       8,     8,  -139,  -139,  -139,    31,   131,    61,  -139,   110,
      31,  -139,    91,    61,  -139,   149,   149,   149,   149,   176,
    -139,  -139,  -139,  -139,  -139,    31,    31,  -139,   149,     4,
     117,  -139,    88,    92,  -139,   149,    98,    96,  -139,    31,
    -139,   122,    29,    62,    62,  -139,  -139,  -139,  -139,   133,
     121,  -139,   110,  -139,    31,   200,  -139,  -139,    31,   139,
     140,    31,    31,  -139,   149,  -139,   135,  -139,  -139,     4,
     149,  -139,  -139,  -139,  -139,    29,  -139
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       2,     3,     1,     0,   111,    12,     5,     6,     7,    14,
       0,     0,    15,     4,     0,    17,    19,     0,   102,    70,
      13,    31,    30,    74,     0,   100,    82,    63,    61,    62,
       0,     0,     0,     0,    70,     0,    43,    58,    64,    60,
       0,    36,   107,   109,    86,     0,     0,     0,    22,    24,
      26,    28,     0,     0,    84,     0,    83,    56,    57,    65,
       0,    43,    72,    42,    42,    42,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    10,
       0,     0,    32,    87,     0,    77,    79,    21,   101,     0,
       0,     0,     0,     0,     0,     8,     0,    44,    73,     0,
      36,    66,    68,    45,    48,    47,    46,    49,    50,    55,
      51,    52,    53,    54,    42,     0,    12,   110,   108,    90,
      96,    71,     0,    78,    80,    23,    25,    27,    29,     0,
     106,    36,    85,    59,    42,     0,     0,    36,    42,    14,
      39,    94,     0,    91,    92,    98,     0,    97,    75,     0,
     104,     0,    33,    67,    69,    42,    20,    11,    38,     0,
       0,    88,     0,    89,     0,    81,     9,    36,     0,     0,
       0,    77,     0,    93,    99,    34,     0,    16,    18,    14,
      95,    42,    41,    36,    42,    33,    35
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -139,  -139,  -139,  -139,  -139,  -139,  -139,    39,  -139,  -138,
    -139,  -139,  -139,  -139,  -139,  -139,  -139,   -29,   -98,  -139,
    -139,  -139,  -139,   -51,   -14,   -12,  -139,  -139,   147,  -139,
    -139,   177,  -139,  -139,    52,   -42,  -139,  -139,  -139,  -139,
    -139,  -139,  -139,    76,  -139,  -139,  -139,  -139,    -7,  -139,
    -139,  -139,  -139,  -139
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,     1,     5,     6,    11,   131,   116,     9,    10,    13,
      20,    40,    41,    89,    90,    91,    92,   169,    78,    79,
     140,   158,   159,   100,    60,    36,   135,   136,    37,    44,
      99,    38,    45,    23,   122,   123,   149,    55,    56,    84,
     121,   142,   143,   144,   160,   146,   147,    24,    39,    52,
      53,    43,    82,     7
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      35,   157,   134,    25,    87,    14,    46,     2,    80,    15,
     119,    16,   120,   101,   102,    17,    57,    58,    59,    18,
       8,    61,   -76,    47,     3,    81,    77,     4,    42,  -103,
    -103,  -105,  -105,   151,    86,    27,   167,   168,   117,   155,
      88,   182,    26,    19,    75,    76,    12,    28,    98,    29,
      18,    54,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   137,    62,    30,    31,    83,    86,   175,
      32,    33,    18,    86,    34,   125,   126,   127,   128,   129,
      85,    93,    94,   152,    95,   184,    96,   156,    63,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      97,   138,   115,   118,   170,   124,   145,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
     130,   153,   154,    48,    49,    50,    51,   132,   133,   141,
     183,   -40,   148,   185,   161,   166,   162,   165,   -37,   -37,
     164,   181,   -76,   -37,   -37,   -37,   163,   171,   172,  -103,
     174,  -105,   177,   178,   176,   139,   186,    21,   180,    86,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,   114,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    22,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    72,    73,    74,    75,    76,    73,
      74,    75,    76,   179,   150,   -43,   -43,   -43,   -43,   -43,
     -43,   -43,   -43,   -43,   -43,   -43,   -43,   -43,   173
};

static const yytype_uint8 yycheck[] =
{
      14,   139,   100,    10,    46,     5,    27,     0,    27,     9,
      45,    11,    47,    64,    65,    15,    30,    31,    32,    19,
      19,    33,    40,    44,    17,    44,    40,    20,    19,    47,
      47,    49,    49,   131,    46,     4,     7,     8,    80,   137,
      47,   179,    40,    43,    36,    37,    42,    16,    62,    18,
      19,    19,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,   114,    40,    34,    35,    19,    80,   167,
      39,    40,    19,    85,    43,    89,    90,    91,    92,    93,
      40,    47,    49,   134,    41,   183,    44,   138,     6,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      41,   115,    12,    19,   155,    44,   120,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      19,   135,   136,    21,    22,    23,    24,    19,    41,    19,
     181,    14,    41,   184,    46,    13,    44,   149,     7,     8,
      44,     6,    40,    12,    13,    14,    48,    14,    27,    47,
     164,    49,    13,    13,   168,   116,   185,    10,   172,   171,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    10,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    10,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    33,    34,    35,    36,    37,    34,
      35,    36,    37,   171,    48,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,   162
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,    51,     0,    17,    20,    52,    53,   103,    19,    57,
      58,    54,    42,    59,     5,     9,    11,    15,    19,    43,
      60,    78,    81,    83,    97,    98,    40,     4,    16,    18,
      34,    35,    39,    40,    43,    74,    75,    78,    81,    98,
      61,    62,    19,   101,    79,    82,    27,    44,    21,    22,
      23,    24,    99,   100,    19,    87,    88,    74,    74,    74,
      74,    75,    40,     6,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    74,    68,    69,
      27,    44,   102,    19,    89,    40,    75,    85,    98,    63,
      64,    65,    66,    47,    49,    41,    44,    41,    74,    80,
      73,    73,    73,    74,    74,    74,    74,    74,    74,    74,
      74,    74,    74,    74,    10,    12,    56,    85,    19,    45,
      47,    90,    84,    85,    44,    74,    74,    74,    74,    74,
      19,    55,    19,    41,    68,    76,    77,    73,    74,    57,
      70,    19,    91,    92,    93,    74,    95,    96,    41,    86,
      48,    68,    73,    74,    74,    68,    73,    59,    71,    72,
      94,    46,    44,    48,    44,    75,    13,     7,     8,    67,
      73,    14,    27,    93,    74,    68,    74,    13,    13,    84,
      74,     6,    59,    73,    68,    73,    67
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    50,    51,    52,    51,    51,    51,    54,    55,    53,
      56,    56,    58,    57,    59,    59,    60,    61,    60,    62,
      60,    60,    63,    60,    64,    60,    65,    60,    66,    60,
      60,    60,    60,    67,    67,    67,    69,    70,    68,    71,
      72,    71,    73,    74,    75,    75,    75,    75,    75,    75,
      75,    75,    75,    75,    75,    75,    75,    75,    75,    75,
      75,    75,    75,    75,    75,    75,    76,    75,    77,    75,
      79,    78,    80,    80,    82,    81,    83,    84,    84,    85,
      86,    85,    87,    87,    88,    88,    89,    89,    90,    90,
      91,    91,    92,    92,    94,    93,    95,    95,    96,    96,
      97,    97,    98,    99,    98,   100,    98,   101,   101,   102,
     102,   103
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     0,     4,     2,     2,     0,     0,     9,
       0,     3,     0,     2,     0,     1,     8,     0,     8,     0,
       6,     3,     0,     4,     0,     4,     0,     4,     0,     4,
       1,     1,     3,     0,     2,     7,     0,     0,     4,     0,
       0,     4,     0,     1,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     2,     2,     1,     4,
       1,     1,     1,     1,     1,     2,     0,     5,     0,     5,
       0,     4,     0,     1,     0,     5,     1,     0,     1,     1,
       0,     4,     0,     1,     1,     3,     0,     1,     3,     3,
       0,     1,     1,     3,     0,     4,     0,     1,     1,     3,
       1,     3,     1,     0,     5,     0,     4,     1,     3,     0,
       2,     1
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
#line 247 "lua.stx"
                {
	  	  pc=maincode; basepc=initcode; maxcurr=maxmain;
		  nlocalvar=0;
	        }
#line 1626 "y.tab.c"
    break;

  case 4: /* functionlist: functionlist $@1 stat sc  */
#line 252 "lua.stx"
                {
		  maincode=pc; initcode=basepc; maxmain=maxcurr;
		}
#line 1634 "y.tab.c"
    break;

  case 7: /* @2: %empty  */
#line 260 "lua.stx"
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
#line 1654 "y.tab.c"
    break;

  case 8: /* $@3: %empty  */
#line 276 "lua.stx"
               {
	        if (lua_debug)
		{
	         code_byte(SETFUNCTION); 
                 code_word(lua_nfile-1);
		 code_word((yyvsp[-3].vWord));
		}
	        lua_codeadjust (0);
	       }
#line 1668 "y.tab.c"
    break;

  case 9: /* function: FUNCTION NAME @2 '(' parlist ')' $@3 block END  */
#line 287 "lua.stx"
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
#line 1689 "y.tab.c"
    break;

  case 12: /* $@4: %empty  */
#line 309 "lua.stx"
           {
            ntemp = 0; 
            if (lua_debug)
            {
             code_byte(SETLINE); code_word(lua_linenumber);
            }
	   }
#line 1701 "y.tab.c"
    break;

  case 16: /* stat1: IF expr1 THEN PrepJump block PrepJump elsepart END  */
#line 322 "lua.stx"
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
#line 1723 "y.tab.c"
    break;

  case 17: /* @5: %empty  */
#line 340 "lua.stx"
               {(yyval.vWord)=pc;}
#line 1729 "y.tab.c"
    break;

  case 18: /* stat1: WHILE @5 expr1 DO PrepJump block PrepJump END  */
#line 342 "lua.stx"
       {
        basepc[(yyvsp[-3].vWord)] = IFFJMP;
	code_word_at(basepc+(yyvsp[-3].vWord)+1, pc - ((yyvsp[-3].vWord) + sizeof(Word)+1));
        
        basepc[(yyvsp[-1].vWord)] = UPJMP;
	code_word_at(basepc+(yyvsp[-1].vWord)+1, pc - ((yyvsp[-6].vWord)));
       }
#line 1741 "y.tab.c"
    break;

  case 19: /* @6: %empty  */
#line 350 "lua.stx"
                {(yyval.vWord)=pc;}
#line 1747 "y.tab.c"
    break;

  case 20: /* stat1: REPEAT @6 block UNTIL expr1 PrepJump  */
#line 352 "lua.stx"
       {
        basepc[(yyvsp[0].vWord)] = IFFUPJMP;
	code_word_at(basepc+(yyvsp[0].vWord)+1, pc - ((yyvsp[-4].vWord)));
       }
#line 1756 "y.tab.c"
    break;

  case 21: /* stat1: varlist1 '=' exprlist1  */
#line 359 "lua.stx"
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
#line 1772 "y.tab.c"
    break;

  case 22: /* $@7: %empty  */
#line 370 "lua.stx"
                   { lua_pushvar((yyvsp[-1].vLong)); }
#line 1778 "y.tab.c"
    break;

  case 23: /* stat1: var ADDEQ $@7 expr1  */
#line 371 "lua.stx"
        { code_compound_finish((yyvsp[-3].vLong), ADDOP); ntemp--; }
#line 1784 "y.tab.c"
    break;

  case 24: /* $@8: %empty  */
#line 372 "lua.stx"
                   { lua_pushvar((yyvsp[-1].vLong)); }
#line 1790 "y.tab.c"
    break;

  case 25: /* stat1: var SUBEQ $@8 expr1  */
#line 373 "lua.stx"
        { code_compound_finish((yyvsp[-3].vLong), SUBOP); ntemp--; }
#line 1796 "y.tab.c"
    break;

  case 26: /* $@9: %empty  */
#line 374 "lua.stx"
                    { lua_pushvar((yyvsp[-1].vLong)); }
#line 1802 "y.tab.c"
    break;

  case 27: /* stat1: var MULTEQ $@9 expr1  */
#line 375 "lua.stx"
        { code_compound_finish((yyvsp[-3].vLong), MULTOP); ntemp--; }
#line 1808 "y.tab.c"
    break;

  case 28: /* $@10: %empty  */
#line 376 "lua.stx"
                   { lua_pushvar((yyvsp[-1].vLong)); }
#line 1814 "y.tab.c"
    break;

  case 29: /* stat1: var DIVEQ $@10 expr1  */
#line 377 "lua.stx"
        { code_compound_finish((yyvsp[-3].vLong), DIVOP); ntemp--; }
#line 1820 "y.tab.c"
    break;

  case 30: /* stat1: functioncall  */
#line 379 "lua.stx"
                                        { lua_codeadjust (0); }
#line 1826 "y.tab.c"
    break;

  case 31: /* stat1: typeconstructor  */
#line 380 "lua.stx"
                                        { lua_codeadjust (0); }
#line 1832 "y.tab.c"
    break;

  case 32: /* stat1: LOCAL localdeclist decinit  */
#line 381 "lua.stx"
                                      { add_nlocalvar((yyvsp[-1].vInt)); lua_codeadjust (0); }
#line 1838 "y.tab.c"
    break;

  case 35: /* elsepart: ELSEIF expr1 THEN PrepJump block PrepJump elsepart  */
#line 387 "lua.stx"
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
#line 1860 "y.tab.c"
    break;

  case 36: /* @11: %empty  */
#line 406 "lua.stx"
           {(yyval.vInt) = nlocalvar;}
#line 1866 "y.tab.c"
    break;

  case 37: /* $@12: %empty  */
#line 406 "lua.stx"
                                            {ntemp = 0;}
#line 1872 "y.tab.c"
    break;

  case 38: /* block: @11 statlist $@12 ret  */
#line 407 "lua.stx"
         {
	  if (nlocalvar != (yyvsp[-3].vInt))
	  {
           nlocalvar = (yyvsp[-3].vInt);
	   lua_codeadjust (0);
	  }
         }
#line 1884 "y.tab.c"
    break;

  case 40: /* $@13: %empty  */
#line 417 "lua.stx"
          { if (lua_debug){code_byte(SETLINE);code_word(lua_linenumber);}}
#line 1890 "y.tab.c"
    break;

  case 41: /* ret: $@13 RETURN exprlist sc  */
#line 419 "lua.stx"
          { 
           if (lua_debug) code_byte(RESET); 
           code_byte(RETCODE); code_byte(nlocalvar);
          }
#line 1899 "y.tab.c"
    break;

  case 42: /* PrepJump: %empty  */
#line 426 "lua.stx"
         { 
	  (yyval.vWord) = pc;
	  code_byte(0);		/* open space */
	  code_word (0);
         }
#line 1909 "y.tab.c"
    break;

  case 43: /* expr1: expr  */
#line 432 "lua.stx"
                { if ((yyvsp[0].vInt) == 0) {lua_codeadjust (ntemp+1); incr_ntemp();}}
#line 1915 "y.tab.c"
    break;

  case 44: /* expr: '(' expr ')'  */
#line 435 "lua.stx"
                        { (yyval.vInt) = (yyvsp[-1].vInt); }
#line 1921 "y.tab.c"
    break;

  case 45: /* expr: expr1 '=' expr1  */
#line 436 "lua.stx"
                        { code_byte(EQOP);   (yyval.vInt) = 1; ntemp--;}
#line 1927 "y.tab.c"
    break;

  case 46: /* expr: expr1 '<' expr1  */
#line 437 "lua.stx"
                        { code_byte(LTOP);   (yyval.vInt) = 1; ntemp--;}
#line 1933 "y.tab.c"
    break;

  case 47: /* expr: expr1 '>' expr1  */
#line 438 "lua.stx"
                        { code_byte(LEOP); code_byte(NOTOP); (yyval.vInt) = 1; ntemp--;}
#line 1939 "y.tab.c"
    break;

  case 48: /* expr: expr1 NE expr1  */
#line 439 "lua.stx"
                        { code_byte(EQOP); code_byte(NOTOP); (yyval.vInt) = 1; ntemp--;}
#line 1945 "y.tab.c"
    break;

  case 49: /* expr: expr1 LE expr1  */
#line 440 "lua.stx"
                        { code_byte(LEOP);   (yyval.vInt) = 1; ntemp--;}
#line 1951 "y.tab.c"
    break;

  case 50: /* expr: expr1 GE expr1  */
#line 441 "lua.stx"
                        { code_byte(LTOP); code_byte(NOTOP); (yyval.vInt) = 1; ntemp--;}
#line 1957 "y.tab.c"
    break;

  case 51: /* expr: expr1 '+' expr1  */
#line 442 "lua.stx"
                        { code_byte(ADDOP);  (yyval.vInt) = 1; ntemp--;}
#line 1963 "y.tab.c"
    break;

  case 52: /* expr: expr1 '-' expr1  */
#line 443 "lua.stx"
                        { code_byte(SUBOP);  (yyval.vInt) = 1; ntemp--;}
#line 1969 "y.tab.c"
    break;

  case 53: /* expr: expr1 '*' expr1  */
#line 444 "lua.stx"
                        { code_byte(MULTOP); (yyval.vInt) = 1; ntemp--;}
#line 1975 "y.tab.c"
    break;

  case 54: /* expr: expr1 '/' expr1  */
#line 445 "lua.stx"
                        { code_byte(DIVOP);  (yyval.vInt) = 1; ntemp--;}
#line 1981 "y.tab.c"
    break;

  case 55: /* expr: expr1 CONC expr1  */
#line 446 "lua.stx"
                         { code_byte(CONCOP);  (yyval.vInt) = 1; ntemp--;}
#line 1987 "y.tab.c"
    break;

  case 56: /* expr: '+' expr1  */
#line 447 "lua.stx"
                                { (yyval.vInt) = 1; }
#line 1993 "y.tab.c"
    break;

  case 57: /* expr: '-' expr1  */
#line 448 "lua.stx"
                                { code_byte(MINUSOP); (yyval.vInt) = 1;}
#line 1999 "y.tab.c"
    break;

  case 58: /* expr: typeconstructor  */
#line 449 "lua.stx"
                       { (yyval.vInt) = (yyvsp[0].vInt); }
#line 2005 "y.tab.c"
    break;

  case 59: /* expr: '@' '(' dimension ')'  */
#line 451 "lua.stx"
     { 
      code_byte(CREATEARRAY);
      (yyval.vInt) = 1;
     }
#line 2014 "y.tab.c"
    break;

  case 60: /* expr: var  */
#line 455 "lua.stx"
                        { lua_pushvar ((yyvsp[0].vLong)); (yyval.vInt) = 1;}
#line 2020 "y.tab.c"
    break;

  case 61: /* expr: NUMBER  */
#line 456 "lua.stx"
                        { code_number((yyvsp[0].vFloat)); (yyval.vInt) = 1; }
#line 2026 "y.tab.c"
    break;

  case 62: /* expr: STRING  */
#line 458 "lua.stx"
     {
      code_byte(PUSHSTRING);
      code_word((yyvsp[0].vWord));
      (yyval.vInt) = 1;
      incr_ntemp();
     }
#line 2037 "y.tab.c"
    break;

  case 63: /* expr: NIL  */
#line 464 "lua.stx"
                        {code_byte(PUSHNIL); (yyval.vInt) = 1; incr_ntemp();}
#line 2043 "y.tab.c"
    break;

  case 64: /* expr: functioncall  */
#line 466 "lua.stx"
     {
      (yyval.vInt) = 0;
      if (lua_debug)
      {
       code_byte(SETLINE); code_word(lua_linenumber);
      }
     }
#line 2055 "y.tab.c"
    break;

  case 65: /* expr: NOT expr1  */
#line 473 "lua.stx"
                        { code_byte(NOTOP);  (yyval.vInt) = 1;}
#line 2061 "y.tab.c"
    break;

  case 66: /* $@14: %empty  */
#line 474 "lua.stx"
                           {code_byte(POP); ntemp--;}
#line 2067 "y.tab.c"
    break;

  case 67: /* expr: expr1 AND PrepJump $@14 expr1  */
#line 475 "lua.stx"
     { 
      basepc[(yyvsp[-2].vWord)] = ONFJMP;
      code_word_at(basepc+(yyvsp[-2].vWord)+1, pc - ((yyvsp[-2].vWord) + sizeof(Word)+1));
      (yyval.vInt) = 1;
     }
#line 2077 "y.tab.c"
    break;

  case 68: /* $@15: %empty  */
#line 480 "lua.stx"
                          {code_byte(POP); ntemp--;}
#line 2083 "y.tab.c"
    break;

  case 69: /* expr: expr1 OR PrepJump $@15 expr1  */
#line 481 "lua.stx"
     { 
      basepc[(yyvsp[-2].vWord)] = ONTJMP;
      code_word_at(basepc+(yyvsp[-2].vWord)+1, pc - ((yyvsp[-2].vWord) + sizeof(Word)+1));
      (yyval.vInt) = 1;
     }
#line 2093 "y.tab.c"
    break;

  case 70: /* @16: %empty  */
#line 489 "lua.stx"
     {
      code_byte(PUSHBYTE);
      (yyval.vWord) = pc; code_byte(0);
      incr_ntemp();
      code_byte(CREATEARRAY);
     }
#line 2104 "y.tab.c"
    break;

  case 71: /* typeconstructor: '@' @16 objectname fieldlist  */
#line 496 "lua.stx"
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
#line 2131 "y.tab.c"
    break;

  case 72: /* dimension: %empty  */
#line 520 "lua.stx"
                                { code_byte(PUSHNIL); incr_ntemp();}
#line 2137 "y.tab.c"
    break;

  case 74: /* @17: %empty  */
#line 524 "lua.stx"
                              {code_byte(PUSHMARK); (yyval.vInt) = ntemp; incr_ntemp();}
#line 2143 "y.tab.c"
    break;

  case 75: /* functioncall: functionvalue @17 '(' exprlist ')'  */
#line 525 "lua.stx"
                                 { code_byte(CALLFUNC); ntemp = (yyvsp[-3].vInt)-1;}
#line 2149 "y.tab.c"
    break;

  case 76: /* functionvalue: var  */
#line 527 "lua.stx"
                    {lua_pushvar ((yyvsp[0].vLong)); }
#line 2155 "y.tab.c"
    break;

  case 77: /* exprlist: %empty  */
#line 530 "lua.stx"
                                        { (yyval.vInt) = 1; }
#line 2161 "y.tab.c"
    break;

  case 78: /* exprlist: exprlist1  */
#line 531 "lua.stx"
                                        { (yyval.vInt) = (yyvsp[0].vInt); }
#line 2167 "y.tab.c"
    break;

  case 79: /* exprlist1: expr  */
#line 534 "lua.stx"
                                        { (yyval.vInt) = (yyvsp[0].vInt); }
#line 2173 "y.tab.c"
    break;

  case 80: /* $@18: %empty  */
#line 535 "lua.stx"
                              {if (!(yyvsp[-1].vInt)){lua_codeadjust (ntemp+1); incr_ntemp();}}
#line 2179 "y.tab.c"
    break;

  case 81: /* exprlist1: exprlist1 ',' $@18 expr  */
#line 536 "lua.stx"
                      {(yyval.vInt) = (yyvsp[0].vInt);}
#line 2185 "y.tab.c"
    break;

  case 84: /* parlist1: NAME  */
#line 544 "lua.stx"
                {
		 localvar[nlocalvar]=lua_findsymbol((yyvsp[0].pChar)); 
		 add_nlocalvar(1);
		}
#line 2194 "y.tab.c"
    break;

  case 85: /* parlist1: parlist1 ',' NAME  */
#line 549 "lua.stx"
                {
		 localvar[nlocalvar]=lua_findsymbol((yyvsp[0].pChar)); 
		 add_nlocalvar(1);
		}
#line 2203 "y.tab.c"
    break;

  case 86: /* objectname: %empty  */
#line 555 "lua.stx"
                                {(yyval.vLong)=-1;}
#line 2209 "y.tab.c"
    break;

  case 87: /* objectname: NAME  */
#line 556 "lua.stx"
                                {(yyval.vLong)=lua_findsymbol((yyvsp[0].pChar));}
#line 2215 "y.tab.c"
    break;

  case 88: /* fieldlist: '{' ffieldlist '}'  */
#line 560 "lua.stx"
              { 
	       flush_record((yyvsp[-1].vInt)%FIELDS_PER_FLUSH); 
	       (yyval.vInt) = (yyvsp[-1].vInt);
	      }
#line 2224 "y.tab.c"
    break;

  case 89: /* fieldlist: '[' lfieldlist ']'  */
#line 565 "lua.stx"
              { 
	       flush_list((yyvsp[-1].vInt)/FIELDS_PER_FLUSH, (yyvsp[-1].vInt)%FIELDS_PER_FLUSH);
	       (yyval.vInt) = (yyvsp[-1].vInt);
     	      }
#line 2233 "y.tab.c"
    break;

  case 90: /* ffieldlist: %empty  */
#line 571 "lua.stx"
                           { (yyval.vInt) = 0; }
#line 2239 "y.tab.c"
    break;

  case 91: /* ffieldlist: ffieldlist1  */
#line 572 "lua.stx"
                           { (yyval.vInt) = (yyvsp[0].vInt); }
#line 2245 "y.tab.c"
    break;

  case 92: /* ffieldlist1: ffield  */
#line 575 "lua.stx"
                                        {(yyval.vInt)=1;}
#line 2251 "y.tab.c"
    break;

  case 93: /* ffieldlist1: ffieldlist1 ',' ffield  */
#line 577 "lua.stx"
                {
		  (yyval.vInt)=(yyvsp[-2].vInt)+1;
		  if ((yyval.vInt)%FIELDS_PER_FLUSH == 0) flush_record(FIELDS_PER_FLUSH);
		}
#line 2260 "y.tab.c"
    break;

  case 94: /* @19: %empty  */
#line 583 "lua.stx"
                   {(yyval.vWord) = lua_findconstant((yyvsp[0].pChar));}
#line 2266 "y.tab.c"
    break;

  case 95: /* ffield: NAME @19 '=' expr1  */
#line 584 "lua.stx"
              { 
	       push_field((yyvsp[-2].vWord));
	      }
#line 2274 "y.tab.c"
    break;

  case 96: /* lfieldlist: %empty  */
#line 589 "lua.stx"
                           { (yyval.vInt) = 0; }
#line 2280 "y.tab.c"
    break;

  case 97: /* lfieldlist: lfieldlist1  */
#line 590 "lua.stx"
                           { (yyval.vInt) = (yyvsp[0].vInt); }
#line 2286 "y.tab.c"
    break;

  case 98: /* lfieldlist1: expr1  */
#line 593 "lua.stx"
                     {(yyval.vInt)=1;}
#line 2292 "y.tab.c"
    break;

  case 99: /* lfieldlist1: lfieldlist1 ',' expr1  */
#line 595 "lua.stx"
                {
		  (yyval.vInt)=(yyvsp[-2].vInt)+1;
		  if ((yyval.vInt)%FIELDS_PER_FLUSH == 0) 
		    flush_list((yyval.vInt)/FIELDS_PER_FLUSH - 1, FIELDS_PER_FLUSH);
		}
#line 2302 "y.tab.c"
    break;

  case 100: /* varlist1: var  */
#line 603 "lua.stx"
          {
	   nvarbuffer = 0; 
           varbuffer[nvarbuffer] = (yyvsp[0].vLong); incr_nvarbuffer();
	   (yyval.vInt) = ((yyvsp[0].vLong) == 0) ? 1 : 0;
	  }
#line 2312 "y.tab.c"
    break;

  case 101: /* varlist1: varlist1 ',' var  */
#line 609 "lua.stx"
          { 
           varbuffer[nvarbuffer] = (yyvsp[0].vLong); incr_nvarbuffer();
	   (yyval.vInt) = ((yyvsp[0].vLong) == 0) ? (yyvsp[-2].vInt) + 1 : (yyvsp[-2].vInt);
	  }
#line 2321 "y.tab.c"
    break;

  case 102: /* var: NAME  */
#line 616 "lua.stx"
          {
	   Word s = lua_findsymbol((yyvsp[0].pChar));
	   int local = lua_localname (s);
	   if (local == -1)	/* global var */
	    (yyval.vLong) = s + 1;		/* return positive value */
           else
	    (yyval.vLong) = -(local+1);		/* return negative value */
	  }
#line 2334 "y.tab.c"
    break;

  case 103: /* $@20: %empty  */
#line 625 "lua.stx"
                    {lua_pushvar ((yyvsp[0].vLong));}
#line 2340 "y.tab.c"
    break;

  case 104: /* var: var $@20 '[' expr1 ']'  */
#line 626 "lua.stx"
          {
	   (yyval.vLong) = 0;		/* indexed variable */
	  }
#line 2348 "y.tab.c"
    break;

  case 105: /* $@21: %empty  */
#line 629 "lua.stx"
                    {lua_pushvar ((yyvsp[0].vLong));}
#line 2354 "y.tab.c"
    break;

  case 106: /* var: var $@21 '.' NAME  */
#line 630 "lua.stx"
          {
	   code_byte(PUSHSTRING);
	   code_word(lua_findconstant((yyvsp[0].pChar))); incr_ntemp();
	   (yyval.vLong) = 0;		/* indexed variable */
	  }
#line 2364 "y.tab.c"
    break;

  case 107: /* localdeclist: NAME  */
#line 637 "lua.stx"
                     {localvar[nlocalvar]=lua_findsymbol((yyvsp[0].pChar)); (yyval.vInt) = 1;}
#line 2370 "y.tab.c"
    break;

  case 108: /* localdeclist: localdeclist ',' NAME  */
#line 639 "lua.stx"
            {
	     localvar[nlocalvar+(yyvsp[-2].vInt)]=lua_findsymbol((yyvsp[0].pChar)); 
	     (yyval.vInt) = (yyvsp[-2].vInt)+1;
	    }
#line 2379 "y.tab.c"
    break;

  case 111: /* setdebug: DEBUG  */
#line 649 "lua.stx"
                  {lua_debug = (yyvsp[0].vInt);}
#line 2385 "y.tab.c"
    break;


#line 2389 "y.tab.c"

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

#line 651 "lua.stx"


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

