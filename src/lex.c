char *rcs_lex = "$Id: lex.c,v 2.1 1994/04/15 19:00:28 celes Exp $";
/*$Log: lex.c,v $
 * Revision 2.1  1994/04/15  19:00:28  celes
 * Retirar chamada da funcao lua_findsymbol associada a cada
 * token NAME. A decisao de chamar lua_findsymbol ou lua_findconstant
 * fica a cargo do modulo "lua.stx".
 *
 * Revision 1.3  1993/12/28  16:42:29  roberto
 * "include"s de string.h e stdlib.h para evitar warnings
 *
 * Revision 1.2  1993/12/22  21:39:15  celes
 * Tratamento do token $debug e $nodebug
 *
 * Revision 1.1  1993/12/22  21:15:16  roberto
 * Initial revision
 **/

#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "opcode.h"
#include "hash.h"
#include "inout.h"
#include "table.h"
#include "y.tab.h"

#define next() { current = input(); }//获取下一个输入字符，并将其存储在current变量中，以供后续的词法分析使用
#define save(x) { *yytextLast++ = (x); }//将当前字符x保存到yytext数组中，并将yytextLast指针向后移动一个位置，以便下次保存时覆盖下一个位置，这个宏用于构建当前识别的词法单元的文本表示
#define save_and_next()  { save(current); next(); }//将当前字符保存到yytext数组中，并获取下一个输入字符，这个宏是save和next的组合，常用于在识别过程中需要同时保存当前字符并继续读取下一个字符的情况

static int current;
static char yytext[256];
static char *yytextLast;//指向yytext数组中下一个可用位置的指针，用于保存当前识别的词法单元的文本表示，初始值在yylex函数中被重置为yytext数组的开始位置

static Input input;

void lua_setinput (Input fn)//设置输入函数，fn是一个函数指针，指向一个函数，该函数用于从某个输入源（如文件、字符串等）读取数据，并返回下一个字符的ASCII码
{
  current = ' ';
  input = fn;
}

char *lua_lasttext (void)//返回最后读取的文本内容，通常在词法分析过程中，yytext数组会保存当前识别的词法单元的文本表示，调用这个函数可以获取这个文本内容，以供语法分析器使用或者进行错误报告等操作
{
  *yytextLast = 0;
  return yytext;
}


static struct //保留字列表，包含Lua语言中的关键字和它们对应的token值，这些保留字在词法分析过程中会被识别为特殊的token，而不是普通的标识符
  {
    char *name;
    int token;
  } reserved [] = {
      {"and", AND},
      {"do", DO},
      {"else", ELSE},
      {"elseif", ELSEIF},
      {"end", END},
      {"function", FUNCTION},
      {"if", IF},
      {"local", LOCAL},
      {"nil", NIL},
      {"not", NOT},
      {"or", OR},
      {"repeat", REPEAT},
      {"return", RETURN},
      {"then", THEN},
      {"until", UNTIL},
      {"while", WHILE} };

#define RESERVEDSIZE (sizeof(reserved)/sizeof(reserved[0]))


int findReserved (char *name)//在保留字列表中查找一个名字，name是要查找的字符串，函数使用二分查找算法在reserved数组中查找这个名字，如果找到则返回对应的token值，如果没有找到则返回0，表示这个名字不是一个保留字
{
  int l = 0;
  int h = RESERVEDSIZE - 1;
  while (l <= h)//二分查找算法的核心逻辑，通过不断将搜索范围缩小一半来快速定位目标字符串，直到找到或者确定不存在为止
  {
    int m = (l+h)/2;
    int comp = strcmp(name, reserved[m].name);//比较要查找的名字和当前中间位置的保留字名字，strcmp函数返回一个整数，表示两个字符串的大小关系
    if (comp < 0)
      h = m-1;
    else if (comp == 0)
      return reserved[m].token;
    else
      l = m+1;
  }
  return 0;
}


int yylex ()//词法分析函数，负责从输入中读取字符并识别出一个个的词法单元（token），根据输入的内容返回相应的token值，并将相关信息存储在yylval中，以供语法分析器使用
{
  while (1)
  {
    yytextLast = yytext;//每次进入循环时，重置yytextLast指针，使其指向yytext数组的开始位置，以便新的词法单元的文本可以从头开始保存
    switch (current)
    {
      case '\n': lua_linenumber++;//换行符只记录行数
      case ' ':
      case '\t'://利用穿透，换行符、空格和制表符公用处理逻辑，当遇到这些字符时，调用next()获取下一个字符，并继续循环
        next();
        continue;

      case '$'://调试开关区
	next();
	while (isalnum(current) || current == '_')
          save_and_next();
        *yytextLast = 0;
	if (strcmp(yytext, "debug") == 0)
	{
	  yylval.vInt = 1;
	  return DEBUG;
        }
	else if (strcmp(yytext, "nodebug") == 0)
	{
	  yylval.vInt = 0;
	  return DEBUG;
        }
	return WRONGTOKEN;
  
      case '-'://减号与注释‘-’是减号，‘--’是注释
        save_and_next();
        if (current != '-') return '-';
        // do { next(); } while (current != '\n' && current != 0);//当遇到注释时，继续读取字符直到遇到换行符'\n'或者输入结束0，跳过了注释内容
        // continue;
  
      case '<'://小于号与小于等于号，‘<’是小于号，‘<=’是小于等于号
        save_and_next();
        if (current != '=') return '<';
        else { save_and_next(); return LE; }
  
      case '>'://大于号与大于等于号，‘>’是大于号，‘>=’是大于等于号
        save_and_next();
        if (current != '=') return '>';
        else { save_and_next(); return GE; }
  
      case '~'://不等于号，‘~’是错误的token但是依旧截取，这里只负责分词，不负责语法分析；‘~=’是不等于号
        save_and_next();
        if (current != '=') return '~';
        else { save_and_next(); return NE; }

      case '"':
      case '\''://字符串提取，当遇到双引号或者单引号时，表示开始识别一个字符串常量，继续读取字符直到遇到相同的定界符为止，在这个过程中处理转义字符，并将识别到的字符串内容保存到yytext数组中，最后将这个字符串常量存入常量表，并返回STRING token
      {
        int del = current;//记录识别的字符串定界符，del变量用于在后续的识别过程中判断字符串是否结束
        next();  /* skip the delimiter */
        while (current != del) 
        {
          switch (current)
          {
            case 0: 
            case '\n': 
              return WRONGTOKEN;//如果在识别字符串的过程中遇到了输入结束符0或者换行符'\n'，说明字符串没有正确结束，返回WRONGTOKEN表示这是一个错误的token
            case '\\':
              next();  /* do not save the '\' */
              switch (current)
              {
                case 'n': save('\n'); next(); break;
                case 't': save('\t'); next(); break;
                case 'r': save('\r'); next(); break;
                default : save('\\'); break;
              }
              break;
            default: 
              save_and_next();
          }
        }
        next();  /* skip the delimiter */
        *yytextLast = 0;//加上字符串结束标志，表示当前识别的字符串常量已经完整地保存在yytext数组中，可以通过这个数组来访问这个字符串内容
        yylval.vWord = lua_findconstant (yytext);//存入常量表，返回常量的索引，并将这个索引存储在yylval.vWord中，以供语法分析器使用
        return STRING;
      }

      case 'a': case 'b': case 'c': case 'd': case 'e':
      case 'f': case 'g': case 'h': case 'i': case 'j':
      case 'k': case 'l': case 'm': case 'n': case 'o':
      case 'p': case 'q': case 'r': case 's': case 't':
      case 'u': case 'v': case 'w': case 'x': case 'y':
      case 'z':
      case 'A': case 'B': case 'C': case 'D': case 'E':
      case 'F': case 'G': case 'H': case 'I': case 'J':
      case 'K': case 'L': case 'M': case 'N': case 'O':
      case 'P': case 'Q': case 'R': case 'S': case 'T':
      case 'U': case 'V': case 'W': case 'X': case 'Y':
      case 'Z':
      case '_'://标识符提取，当遇到字母或者下划线时，表示开始识别一个标识符，读取字符直到遇到非字母、非数字和非下划线的字符为止，最后检查这个标识符是否是一个保留字，如果是则返回相应的token值，如果不是则将这个标识符存入符号表，并返回NAME token
      {
        int res;
        do { save_and_next(); } while (isalnum(current) || current == '_');//继续读取字符直到遇到非字母、非数字和非下划线的字符为止，构成一个完整的标识符
        *yytextLast = 0;
        res = findReserved(yytext);//检查这个标识符是否是一个保留字，如果是则返回相应的token值，这样在语法分析过程中就可以区分出保留字和普通标识符
        if (res) return res;
        yylval.pChar = yytext;//如果不是保留字，则将这个标识符存入符号表，并返回NAME token，yylval.pChar指向这个标识符的文本内容，以供语法分析器使用
        return NAME;
      }
   
      case '.'://连接符和数字的开始，‘.’是连接符，‘.’后面如果跟着一个数字，则表示这是一个浮点数的开始
        save_and_next();
        if (current == '.') 
        { 
          save_and_next(); 
          return CONC;//如果遇到两个连续的点，表示这是一个连接符‘..’，返回CONC token
        }
        else if (!isdigit(current)) return '.';//如果遇到一个点后面没有跟着一个数字，表示这是一个单独的点，返回'.' token
        /* current is a digit: goes through to number */
        goto fraction;//如果遇到一个点后面跟着一个数字，表示这是一个浮点数的开始，跳转到fraction标签继续识别这个数字常量

      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
      
        do { save_and_next(); } while (isdigit(current));//继续读取字符直到遇到非数字的字符为止，构成一个完整的整数部分，如果后面跟着一个点或者指数部分，则继续识别为一个浮点数，否则就是一个整数常量
        if (current == '.') save_and_next();
fraction: while (isdigit(current)) save_and_next();
        if (current == 'e' || current == 'E')//如果遇到'e'或者'E'，表示这是一个科学计数法的浮点数，继续读取指数部分
        {
          save_and_next();
          if (current == '+' || current == '-') save_and_next();//指数部分可以有一个可选的正负号
          if (!isdigit(current)) return WRONGTOKEN;//如果指数部分没有数字，说明这是一个错误的token，返回WRONGTOKEN
          do { save_and_next(); } while (isdigit(current));
        }
        *yytextLast = 0;
        yylval.vFloat = atof(yytext);//将识别到的数字常量转换为浮点数，并存储在yylval.vFloat中，以供语法分析器使用，最后返回NUMBER token
        return NUMBER;
      
      case '/':
          next();
          if( current == '/'){//单行注释//
            do{ next(); }while( current != '\n' && current != 0 ) ;
            continue;
          }
          else if(current == '*'){//多行注释/*
            while(1){
              next();
              if(current == 0){
                lua_reportbug("unterminated comment");//如果遇到文件结尾则报告错误
                break;
              }
              if( current == '*'){
                next();
                if( current == '/'){//遇到*/则表示多行注释结束
                  next();
                  break;
                }
              }
            }
            continue;
          }
          else{//如果单纯的/表示除法
            return '/';
          }

      default: 		/* also end of file *///当遇到其他字符时，表示这是一个单独的符号token，直接返回这个字符的ASCII码作为token值，同时保存这个字符到yytext数组中
      {
        save_and_next();
        return *yytext; //返回这个字符的ASCII码作为token值，这样在语法分析过程中就可以识别出这个符号，并且可以通过yytext数组来访问这个符号的文本内容     
      }
    }
  }
}
        
