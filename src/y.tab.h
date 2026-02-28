/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

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

#line 140 "y.tab.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_Y_TAB_H_INCLUDED  */
