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

#ifndef YY_TSHADE_TSHADE_H_INCLUDED
# define YY_TSHADE_TSHADE_H_INCLUDED
/* Debug traces.  */
#ifndef TSHADE_DEBUG
# if defined YYDEBUG
#if YYDEBUG
#   define TSHADE_DEBUG 1
#  else
#   define TSHADE_DEBUG 0
#  endif
# else /* ! defined YYDEBUG */
#  define TSHADE_DEBUG 0
# endif /* ! defined YYDEBUG */
#endif  /* ! defined TSHADE_DEBUG */
#if TSHADE_DEBUG
extern int tshade_debug;
#endif

/* Token kinds.  */
#ifndef TSHADE_TOKENTYPE
# define TSHADE_TOKENTYPE
  enum tshade_tokentype
  {
    TSHADE_EMPTY = -2,
    TSHADE_EOF = 0,                /* "end of file"  */
    TSHADE_error = 256,            /* error  */
    TSHADE_UNDEF = 257,            /* "invalid token"  */
    rwSWITCH = 258,                /* rwSWITCH  */
    rwCASE = 259,                  /* rwCASE  */
    rwDEFAULT = 260,               /* rwDEFAULT  */
    rwWHILE = 261,                 /* rwWHILE  */
    rwDO = 262,                    /* rwDO  */
    rwFOR = 263,                   /* rwFOR  */
    rwBREAK = 264,                 /* rwBREAK  */
    rwCONTINUE = 265,              /* rwCONTINUE  */
    rwIF = 266,                    /* rwIF  */
    rwELSE = 267,                  /* rwELSE  */
    rwDISCARD = 268,               /* rwDISCARD  */
    OP_EQ = 269,                   /* OP_EQ  */
    OP_NEQ = 270,                  /* OP_NEQ  */
    OP_AND = 271,                  /* OP_AND  */
    OP_OR = 272,                   /* OP_OR  */
    OP_LE = 273,                   /* OP_LE  */
    OP_GE = 274,                   /* OP_GE  */
    INT_NUM = 275,                 /* INT_NUM  */
    FLOAT_NUM = 276,               /* FLOAT_NUM  */
    VAR_IDENT = 277,               /* VAR_IDENT  */
    STR_VAL = 278,                 /* STR_VAL  */
    TYPE_IDENT = 279,              /* TYPE_IDENT  */
    tSTRUCT = 280,                 /* tSTRUCT  */
    tUNIFORM = 281,                /* tUNIFORM  */
    tCBUFFER = 282,                /* tCBUFFER  */
    tSHADERDECLARE = 283,          /* tSHADERDECLARE  */
    tVSSHADER = 284,               /* tVSSHADER  */
    tFLOAT_TYPE = 285,             /* tFLOAT_TYPE  */
    tINT_TYPE = 286,               /* tINT_TYPE  */
    tBOOL_TYPE = 287,              /* tBOOL_TYPE  */
    tUINT_TYPE = 288,              /* tUINT_TYPE  */
    tFVEC2_TYPE = 289,             /* tFVEC2_TYPE  */
    tFVEC3_TYPE = 290,             /* tFVEC3_TYPE  */
    tFVEC4_TYPE = 291,             /* tFVEC4_TYPE  */
    tIVEC2_TYPE = 292,             /* tIVEC2_TYPE  */
    tIVEC3_TYPE = 293,             /* tIVEC3_TYPE  */
    tIVEC4_TYPE = 294,             /* tIVEC4_TYPE  */
    tBVEC2_TYPE = 295,             /* tBVEC2_TYPE  */
    tBVEC3_TYPE = 296,             /* tBVEC3_TYPE  */
    tBVEC4_TYPE = 297,             /* tBVEC4_TYPE  */
    tMAT4_TYPE = 298,              /* tMAT4_TYPE  */
    tMAT43_TYPE = 299,             /* tMAT43_TYPE  */
    tMAT34_TYPE = 300,             /* tMAT34_TYPE  */
    tMAT3_TYPE = 301               /* tMAT3_TYPE  */
  };
  typedef enum tshade_tokentype tshade_token_kind_t;
#endif

/* Value type.  */
#if ! defined TSHADE_STYPE && ! defined TSHADE_STYPE_IS_DECLARED
union TSHADE_STYPE
{
#line 28 "tshade.y"

  // source side ASTnode.
  struct tShadeNode* node;
  // symbol specifics.
  float fVal;
  int intVal;
  const char* strVal;

#line 127 "tshade.h"

};
typedef union TSHADE_STYPE TSHADE_STYPE;
# define TSHADE_STYPE_IS_TRIVIAL 1
# define TSHADE_STYPE_IS_DECLARED 1
#endif




int tshade_parse (yyscan_t scanner, tShadeAst* shadeAst);


#endif /* !YY_TSHADE_TSHADE_H_INCLUDED  */
