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
    rwVOID = 269,                  /* rwVOID  */
    rwSTATIC = 270,                /* rwSTATIC  */
    rwIN = 271,                    /* rwIN  */
    rwOUT = 272,                   /* rwOUT  */
    rwINOUT = 273,                 /* rwINOUT  */
    rwTYPEDEF = 274,               /* rwTYPEDEF  */
    rwTRUE = 275,                  /* rwTRUE  */
    rwFALSE = 276,                 /* rwFALSE  */
    rwRETURN = 277,                /* rwRETURN  */
    rwCONST = 278,                 /* rwCONST  */
    OP_EQ = 279,                   /* OP_EQ  */
    OP_NEQ = 280,                  /* OP_NEQ  */
    OP_AND = 281,                  /* OP_AND  */
    OP_OR = 282,                   /* OP_OR  */
    OP_LE = 283,                   /* OP_LE  */
    OP_GE = 284,                   /* OP_GE  */
    OP_PLUSPLUS = 285,             /* OP_PLUSPLUS  */
    OP_MINUSMINUS = 286,           /* OP_MINUSMINUS  */
    OP_PLUS_ASS = 287,             /* OP_PLUS_ASS  */
    OP_MINUS_ASS = 288,            /* OP_MINUS_ASS  */
    OP_MUL_ASS = 289,              /* OP_MUL_ASS  */
    OP_DIV_ASS = 290,              /* OP_DIV_ASS  */
    OP_MOD_ASS = 291,              /* OP_MOD_ASS  */
    OP_AND_ASS = 292,              /* OP_AND_ASS  */
    OP_OR_ASS = 293,               /* OP_OR_ASS  */
    OP_XOR_ASS = 294,              /* OP_XOR_ASS  */
    OP_BIT_LEFT_ASS = 295,         /* OP_BIT_LEFT_ASS  */
    OP_BIT_RIGHT_ASS = 296,        /* OP_BIT_RIGHT_ASS  */
    OP_BIT_LEFT = 297,             /* OP_BIT_LEFT  */
    OP_BIT_RIGHT = 298,            /* OP_BIT_RIGHT  */
    INT_NUM = 299,                 /* INT_NUM  */
    FLOAT_NUM = 300,               /* FLOAT_NUM  */
    VAR_IDENT = 301,               /* VAR_IDENT  */
    STR_VAL = 302,                 /* STR_VAL  */
    TYPE_IDENT = 303,              /* TYPE_IDENT  */
    tSWIZZLE = 304,                /* tSWIZZLE  */
    tSTRUCT = 305,                 /* tSTRUCT  */
    tUNIFORM = 306,                /* tUNIFORM  */
    tCBUFFER = 307,                /* tCBUFFER  */
    tSHADERDECLARE = 308,          /* tSHADERDECLARE  */
    tVSSHADER = 309,               /* tVSSHADER  */
    tPSSHADER = 310,               /* tPSSHADER  */
    tGSSHADER = 311,               /* tGSSHADER  */
    tCSSHADER = 312,               /* tCSSHADER  */
    tDSSHADER = 313,               /* tDSSHADER  */
    tHSSHADER = 314,               /* tHSSHADER  */
    tFLOAT_TYPE = 315,             /* tFLOAT_TYPE  */
    tINT_TYPE = 316,               /* tINT_TYPE  */
    tBOOL_TYPE = 317,              /* tBOOL_TYPE  */
    tUINT_TYPE = 318,              /* tUINT_TYPE  */
    tFVEC2_TYPE = 319,             /* tFVEC2_TYPE  */
    tFVEC3_TYPE = 320,             /* tFVEC3_TYPE  */
    tFVEC4_TYPE = 321,             /* tFVEC4_TYPE  */
    tIVEC2_TYPE = 322,             /* tIVEC2_TYPE  */
    tIVEC3_TYPE = 323,             /* tIVEC3_TYPE  */
    tIVEC4_TYPE = 324,             /* tIVEC4_TYPE  */
    tBVEC2_TYPE = 325,             /* tBVEC2_TYPE  */
    tBVEC3_TYPE = 326,             /* tBVEC3_TYPE  */
    tBVEC4_TYPE = 327,             /* tBVEC4_TYPE  */
    tMAT4_TYPE = 328,              /* tMAT4_TYPE  */
    tMAT43_TYPE = 329,             /* tMAT43_TYPE  */
    tMAT34_TYPE = 330,             /* tMAT34_TYPE  */
    tMAT3_TYPE = 331,              /* tMAT3_TYPE  */
    tSEM_SVPOSITION = 332,         /* tSEM_SVPOSITION  */
    tSEM_POSITION = 333,           /* tSEM_POSITION  */
    tSEM_NORMAL = 334,             /* tSEM_NORMAL  */
    tSEM_BINORMAL = 335,           /* tSEM_BINORMAL  */
    tSEM_TANGENT = 336,            /* tSEM_TANGENT  */
    tSEM_PSIZE = 337,              /* tSEM_PSIZE  */
    tSEM_TESSFACTOR = 338,         /* tSEM_TESSFACTOR  */
    tSEM_ISFRONTFACE = 339,        /* tSEM_ISFRONTFACE  */
    tSEM_TEXCOORD = 340,           /* tSEM_TEXCOORD  */
    tSEM_COLOR = 341,              /* tSEM_COLOR  */
    tSEM_TARGET = 342,             /* tSEM_TARGET  */
    tSEM_DEPTH = 343               /* tSEM_DEPTH  */
  };
  typedef enum tshade_tokentype tshade_token_kind_t;
#endif

/* Value type.  */
#if ! defined TSHADE_STYPE && ! defined TSHADE_STYPE_IS_DECLARED
union TSHADE_STYPE
{
#line 33 "tshade.y"

  // source side ASTnode.
  tShadeNode* node;
  tStatementListNode* stmt_list_node;
  tExpressionListNode* exprListnode;
  tVarDeclNode* declNode;
  tStructNode* structNode;
  tFunctionNode* funcNode;
  tFunctionParamListNode* funcList;
  tFunctionParamNode* funcParam;
  // symbol specifics.
  double fVal;
  int intVal;
  const char* strVal;
  ShaderVarType varType;
  ParamModifier modifier;
  ShaderSemanticType semtype;

#line 179 "tshade.h"

};
typedef union TSHADE_STYPE TSHADE_STYPE;
# define TSHADE_STYPE_IS_TRIVIAL 1
# define TSHADE_STYPE_IS_DECLARED 1
#endif




int tshade_parse (yyscan_t scanner, tShadeAst* shadeAst);


#endif /* !YY_TSHADE_TSHADE_H_INCLUDED  */
