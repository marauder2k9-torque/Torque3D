/* A Bison parser, made by GNU Bison 3.8.2.  */

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
#define YYPURE 2

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Substitute the type names.  */
#define YYSTYPE         TSHADE_STYPE
#define YYLTYPE         TSHADE_LTYPE
/* Substitute the variable and function names.  */
#define yyparse         tshade_parse
#define yylex           tshade_lex
#define yyerror         tshade_error
#define yydebug         tshade_debug
#define yynerrs         tshade_nerrs

/* First part of user prologue.  */
#line 10 "tshade.y"

  #include <stdlib.h>
  #include <stdio.h>
  #include "platform/platform.h"
  #include "console/console.h"
  #include "core/strings/stringFunctions.h"
  #include "tshadeAst.h"

  #define nil 0

  typedef void* yyscan_t;
  struct YYLTYPE;

  void yyerror(YYLTYPE* yylloc, yyscan_t yyscanner, tShadeAst* shadeAst, char const *msg);
  void yyerror(YYLTYPE* yylloc, yyscan_t yyscanner, char const* msg);

  int yylex(union YYSTYPE * yylval_param, YYLTYPE* yylloc_param , yyscan_t yyscanner);


  extern int TShaderGetLineNo(yyscan_t);
  extern int TShaderGetColumnNo(yyscan_t);
  extern char* TShaderGetText(yyscan_t);

#line 102 "tshade.cpp"

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

#include "tshade.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_3_ = 3,                         /* '+'  */
  YYSYMBOL_4_ = 4,                         /* '-'  */
  YYSYMBOL_5_ = 5,                         /* '*'  */
  YYSYMBOL_6_ = 6,                         /* '/'  */
  YYSYMBOL_7_ = 7,                         /* '<'  */
  YYSYMBOL_8_ = 8,                         /* '>'  */
  YYSYMBOL_9_ = 9,                         /* '='  */
  YYSYMBOL_10_ = 10,                       /* '.'  */
  YYSYMBOL_11_ = 11,                       /* '|'  */
  YYSYMBOL_12_ = 12,                       /* '&'  */
  YYSYMBOL_13_ = 13,                       /* '%'  */
  YYSYMBOL_14_ = 14,                       /* '('  */
  YYSYMBOL_15_ = 15,                       /* ')'  */
  YYSYMBOL_16_ = 16,                       /* ','  */
  YYSYMBOL_17_ = 17,                       /* ':'  */
  YYSYMBOL_18_ = 18,                       /* ';'  */
  YYSYMBOL_19_ = 19,                       /* '{'  */
  YYSYMBOL_20_ = 20,                       /* '}'  */
  YYSYMBOL_21_ = 21,                       /* '^'  */
  YYSYMBOL_22_ = 22,                       /* '~'  */
  YYSYMBOL_23_ = 23,                       /* '!'  */
  YYSYMBOL_24_ = 24,                       /* '@'  */
  YYSYMBOL_rwSWITCH = 25,                  /* rwSWITCH  */
  YYSYMBOL_rwCASE = 26,                    /* rwCASE  */
  YYSYMBOL_rwDEFAULT = 27,                 /* rwDEFAULT  */
  YYSYMBOL_rwWHILE = 28,                   /* rwWHILE  */
  YYSYMBOL_rwDO = 29,                      /* rwDO  */
  YYSYMBOL_rwFOR = 30,                     /* rwFOR  */
  YYSYMBOL_rwBREAK = 31,                   /* rwBREAK  */
  YYSYMBOL_rwCONTINUE = 32,                /* rwCONTINUE  */
  YYSYMBOL_rwIF = 33,                      /* rwIF  */
  YYSYMBOL_rwELSE = 34,                    /* rwELSE  */
  YYSYMBOL_rwDISCARD = 35,                 /* rwDISCARD  */
  YYSYMBOL_rwVOID = 36,                    /* rwVOID  */
  YYSYMBOL_rwSTATIC = 37,                  /* rwSTATIC  */
  YYSYMBOL_rwIN = 38,                      /* rwIN  */
  YYSYMBOL_rwOUT = 39,                     /* rwOUT  */
  YYSYMBOL_rwINOUT = 40,                   /* rwINOUT  */
  YYSYMBOL_rwTYPEDEF = 41,                 /* rwTYPEDEF  */
  YYSYMBOL_rwTRUE = 42,                    /* rwTRUE  */
  YYSYMBOL_rwFALSE = 43,                   /* rwFALSE  */
  YYSYMBOL_rwRETURN = 44,                  /* rwRETURN  */
  YYSYMBOL_rwCONST = 45,                   /* rwCONST  */
  YYSYMBOL_OP_EQ = 46,                     /* OP_EQ  */
  YYSYMBOL_OP_NEQ = 47,                    /* OP_NEQ  */
  YYSYMBOL_OP_AND = 48,                    /* OP_AND  */
  YYSYMBOL_OP_OR = 49,                     /* OP_OR  */
  YYSYMBOL_OP_LE = 50,                     /* OP_LE  */
  YYSYMBOL_OP_GE = 51,                     /* OP_GE  */
  YYSYMBOL_OP_PLUSPLUS = 52,               /* OP_PLUSPLUS  */
  YYSYMBOL_OP_MINUSMINUS = 53,             /* OP_MINUSMINUS  */
  YYSYMBOL_OP_PLUS_ASS = 54,               /* OP_PLUS_ASS  */
  YYSYMBOL_OP_MINUS_ASS = 55,              /* OP_MINUS_ASS  */
  YYSYMBOL_OP_MUL_ASS = 56,                /* OP_MUL_ASS  */
  YYSYMBOL_OP_DIV_ASS = 57,                /* OP_DIV_ASS  */
  YYSYMBOL_OP_MOD_ASS = 58,                /* OP_MOD_ASS  */
  YYSYMBOL_OP_AND_ASS = 59,                /* OP_AND_ASS  */
  YYSYMBOL_OP_OR_ASS = 60,                 /* OP_OR_ASS  */
  YYSYMBOL_OP_XOR_ASS = 61,                /* OP_XOR_ASS  */
  YYSYMBOL_OP_BIT_LEFT_ASS = 62,           /* OP_BIT_LEFT_ASS  */
  YYSYMBOL_OP_BIT_RIGHT_ASS = 63,          /* OP_BIT_RIGHT_ASS  */
  YYSYMBOL_OP_BIT_LEFT = 64,               /* OP_BIT_LEFT  */
  YYSYMBOL_OP_BIT_RIGHT = 65,              /* OP_BIT_RIGHT  */
  YYSYMBOL_INT_NUM = 66,                   /* INT_NUM  */
  YYSYMBOL_FLOAT_NUM = 67,                 /* FLOAT_NUM  */
  YYSYMBOL_VAR_IDENT = 68,                 /* VAR_IDENT  */
  YYSYMBOL_STR_VAL = 69,                   /* STR_VAL  */
  YYSYMBOL_TYPE_IDENT = 70,                /* TYPE_IDENT  */
  YYSYMBOL_MEMBER_VAR = 71,                /* MEMBER_VAR  */
  YYSYMBOL_tSTRUCT = 72,                   /* tSTRUCT  */
  YYSYMBOL_tUNIFORM = 73,                  /* tUNIFORM  */
  YYSYMBOL_tCBUFFER = 74,                  /* tCBUFFER  */
  YYSYMBOL_tSHADERDECLARE = 75,            /* tSHADERDECLARE  */
  YYSYMBOL_tVSSHADER = 76,                 /* tVSSHADER  */
  YYSYMBOL_tPSSHADER = 77,                 /* tPSSHADER  */
  YYSYMBOL_tGSSHADER = 78,                 /* tGSSHADER  */
  YYSYMBOL_tCSSHADER = 79,                 /* tCSSHADER  */
  YYSYMBOL_tDSSHADER = 80,                 /* tDSSHADER  */
  YYSYMBOL_tHSSHADER = 81,                 /* tHSSHADER  */
  YYSYMBOL_tMULFUNC = 82,                  /* tMULFUNC  */
  YYSYMBOL_tFRACFUNC = 83,                 /* tFRACFUNC  */
  YYSYMBOL_tLERPFUNC = 84,                 /* tLERPFUNC  */
  YYSYMBOL_tSAMPLE = 85,                   /* tSAMPLE  */
  YYSYMBOL_tFLOAT_TYPE = 86,               /* tFLOAT_TYPE  */
  YYSYMBOL_tINT_TYPE = 87,                 /* tINT_TYPE  */
  YYSYMBOL_tBOOL_TYPE = 88,                /* tBOOL_TYPE  */
  YYSYMBOL_tUINT_TYPE = 89,                /* tUINT_TYPE  */
  YYSYMBOL_tSAMPLER2D_TYPE = 90,           /* tSAMPLER2D_TYPE  */
  YYSYMBOL_tFVEC2_TYPE = 91,               /* tFVEC2_TYPE  */
  YYSYMBOL_tFVEC3_TYPE = 92,               /* tFVEC3_TYPE  */
  YYSYMBOL_tFVEC4_TYPE = 93,               /* tFVEC4_TYPE  */
  YYSYMBOL_tIVEC2_TYPE = 94,               /* tIVEC2_TYPE  */
  YYSYMBOL_tIVEC3_TYPE = 95,               /* tIVEC3_TYPE  */
  YYSYMBOL_tIVEC4_TYPE = 96,               /* tIVEC4_TYPE  */
  YYSYMBOL_tBVEC2_TYPE = 97,               /* tBVEC2_TYPE  */
  YYSYMBOL_tBVEC3_TYPE = 98,               /* tBVEC3_TYPE  */
  YYSYMBOL_tBVEC4_TYPE = 99,               /* tBVEC4_TYPE  */
  YYSYMBOL_tMAT4_TYPE = 100,               /* tMAT4_TYPE  */
  YYSYMBOL_tMAT43_TYPE = 101,              /* tMAT43_TYPE  */
  YYSYMBOL_tMAT34_TYPE = 102,              /* tMAT34_TYPE  */
  YYSYMBOL_tMAT3_TYPE = 103,               /* tMAT3_TYPE  */
  YYSYMBOL_tSEM_SVPOSITION = 104,          /* tSEM_SVPOSITION  */
  YYSYMBOL_tSEM_POSITION = 105,            /* tSEM_POSITION  */
  YYSYMBOL_tSEM_NORMAL = 106,              /* tSEM_NORMAL  */
  YYSYMBOL_tSEM_BINORMAL = 107,            /* tSEM_BINORMAL  */
  YYSYMBOL_tSEM_TANGENT = 108,             /* tSEM_TANGENT  */
  YYSYMBOL_tSEM_PSIZE = 109,               /* tSEM_PSIZE  */
  YYSYMBOL_tSEM_TESSFACTOR = 110,          /* tSEM_TESSFACTOR  */
  YYSYMBOL_tSEM_ISFRONTFACE = 111,         /* tSEM_ISFRONTFACE  */
  YYSYMBOL_tSEM_TEXCOORD = 112,            /* tSEM_TEXCOORD  */
  YYSYMBOL_tSEM_COLOR = 113,               /* tSEM_COLOR  */
  YYSYMBOL_tSEM_TARGET = 114,              /* tSEM_TARGET  */
  YYSYMBOL_tSEM_DEPTH = 115,               /* tSEM_DEPTH  */
  YYSYMBOL_116_ = 116,                     /* '['  */
  YYSYMBOL_117_ = 117,                     /* ']'  */
  YYSYMBOL_YYACCEPT = 118,                 /* $accept  */
  YYSYMBOL_program = 119,                  /* program  */
  YYSYMBOL_program_globals = 120,          /* program_globals  */
  YYSYMBOL_program_global_list = 121,      /* program_global_list  */
  YYSYMBOL_shader_stage = 122,             /* shader_stage  */
  YYSYMBOL_shader_body = 123,              /* shader_body  */
  YYSYMBOL_struct_decl = 124,              /* struct_decl  */
  YYSYMBOL_structbody_list = 125,          /* structbody_list  */
  YYSYMBOL_struct_member = 126,            /* struct_member  */
  YYSYMBOL_uniform_decl = 127,             /* uniform_decl  */
  YYSYMBOL_static_const_decl = 128,        /* static_const_decl  */
  YYSYMBOL_var_decl = 129,                 /* var_decl  */
  YYSYMBOL_param_modifier = 130,           /* param_modifier  */
  YYSYMBOL_function_def = 131,             /* function_def  */
  YYSYMBOL_function_param_list = 132,      /* function_param_list  */
  YYSYMBOL_function_param = 133,           /* function_param  */
  YYSYMBOL_expression_list = 134,          /* expression_list  */
  YYSYMBOL_expression = 135,               /* expression  */
  YYSYMBOL_statement_list = 136,           /* statement_list  */
  YYSYMBOL_statement = 137,                /* statement  */
  YYSYMBOL_if_statement = 138,             /* if_statement  */
  YYSYMBOL_while_statement = 139,          /* while_statement  */
  YYSYMBOL_switch_statement = 140,         /* switch_statement  */
  YYSYMBOL_case_statements = 141,          /* case_statements  */
  YYSYMBOL_case_rule = 142,                /* case_rule  */
  YYSYMBOL_continue_statement = 143,       /* continue_statement  */
  YYSYMBOL_break_statement = 144,          /* break_statement  */
  YYSYMBOL_return_statement = 145,         /* return_statement  */
  YYSYMBOL_discard_statement = 146,        /* discard_statement  */
  YYSYMBOL_var_type = 147,                 /* var_type  */
  YYSYMBOL_struct_semantic = 148           /* struct_semantic  */
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
typedef yytype_int16 yy_state_t;

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

#if 1

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
#endif /* 1 */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined TSHADE_LTYPE_IS_TRIVIAL && TSHADE_LTYPE_IS_TRIVIAL \
             && defined TSHADE_STYPE_IS_TRIVIAL && TSHADE_STYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE) \
             + YYSIZEOF (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

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
#define YYFINAL  4
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1960

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  118
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  31
/* YYNRULES -- Number of rules.  */
#define YYNRULES  137
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  286

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   348


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
       2,     2,     2,    23,     2,     2,     2,    13,    12,     2,
      14,    15,     5,     3,    16,     4,    10,     6,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    17,    18,
       7,     9,     8,     2,    24,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   116,     2,   117,    21,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    19,    11,    20,    22,     2,     2,     2,
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
       2,     2,     2,     2,     2,     2,     1,     2,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115
};

#if TSHADE_DEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   141,   141,   146,   151,   155,   160,   166,   173,   180,
     187,   197,   202,   208,   209,   214,   216,   218,   223,   231,
     236,   242,   250,   252,   254,   256,   258,   263,   265,   267,
     272,   274,   276,   280,   288,   289,   291,   296,   298,   300,
     305,   310,   312,   317,   319,   321,   323,   325,   327,   329,
     331,   333,   335,   337,   339,   341,   343,   345,   347,   349,
     351,   353,   355,   357,   359,   361,   363,   365,   367,   369,
     371,   373,   375,   377,   387,   389,   398,   400,   406,   407,
     412,   414,   416,   418,   420,   422,   424,   426,   428,   430,
     432,   434,   436,   442,   444,   449,   451,   456,   462,   463,
     468,   470,   475,   480,   485,   487,   492,   497,   499,   501,
     503,   505,   507,   509,   511,   513,   515,   517,   519,   521,
     523,   525,   527,   529,   531,   533,   538,   540,   542,   544,
     546,   548,   550,   552,   554,   556,   558,   560
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if 1
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  static const char *const yy_sname[] =
  {
  "end of file", "error", "invalid token", "'+'", "'-'", "'*'", "'/'",
  "'<'", "'>'", "'='", "'.'", "'|'", "'&'", "'%'", "'('", "')'", "','",
  "':'", "';'", "'{'", "'}'", "'^'", "'~'", "'!'", "'@'", "rwSWITCH",
  "rwCASE", "rwDEFAULT", "rwWHILE", "rwDO", "rwFOR", "rwBREAK",
  "rwCONTINUE", "rwIF", "rwELSE", "rwDISCARD", "rwVOID", "rwSTATIC",
  "rwIN", "rwOUT", "rwINOUT", "rwTYPEDEF", "rwTRUE", "rwFALSE", "rwRETURN",
  "rwCONST", "OP_EQ", "OP_NEQ", "OP_AND", "OP_OR", "OP_LE", "OP_GE",
  "OP_PLUSPLUS", "OP_MINUSMINUS", "OP_PLUS_ASS", "OP_MINUS_ASS",
  "OP_MUL_ASS", "OP_DIV_ASS", "OP_MOD_ASS", "OP_AND_ASS", "OP_OR_ASS",
  "OP_XOR_ASS", "OP_BIT_LEFT_ASS", "OP_BIT_RIGHT_ASS", "OP_BIT_LEFT",
  "OP_BIT_RIGHT", "INT_NUM", "FLOAT_NUM", "VAR_IDENT", "STR_VAL",
  "TYPE_IDENT", "MEMBER_VAR", "tSTRUCT", "tUNIFORM", "tCBUFFER",
  "tSHADERDECLARE", "tVSSHADER", "tPSSHADER", "tGSSHADER", "tCSSHADER",
  "tDSSHADER", "tHSSHADER", "tMULFUNC", "tFRACFUNC", "tLERPFUNC",
  "tSAMPLE", "tFLOAT_TYPE", "tINT_TYPE", "tBOOL_TYPE", "tUINT_TYPE",
  "tSAMPLER2D_TYPE", "tFVEC2_TYPE", "tFVEC3_TYPE", "tFVEC4_TYPE",
  "tIVEC2_TYPE", "tIVEC3_TYPE", "tIVEC4_TYPE", "tBVEC2_TYPE",
  "tBVEC3_TYPE", "tBVEC4_TYPE", "tMAT4_TYPE", "tMAT43_TYPE", "tMAT34_TYPE",
  "tMAT3_TYPE", "tSEM_SVPOSITION", "tSEM_POSITION", "tSEM_NORMAL",
  "tSEM_BINORMAL", "tSEM_TANGENT", "tSEM_PSIZE", "tSEM_TESSFACTOR",
  "tSEM_ISFRONTFACE", "tSEM_TEXCOORD", "tSEM_COLOR", "tSEM_TARGET",
  "tSEM_DEPTH", "'['", "']'", "$accept", "program", "program_globals",
  "program_global_list", "shader_stage", "shader_body", "struct_decl",
  "structbody_list", "struct_member", "uniform_decl", "static_const_decl",
  "var_decl", "param_modifier", "function_def", "function_param_list",
  "function_param", "expression_list", "expression", "statement_list",
  "statement", "if_statement", "while_statement", "switch_statement",
  "case_statements", "case_rule", "continue_statement", "break_statement",
  "return_statement", "discard_statement", "var_type", "struct_semantic", YY_NULLPTR
  };
  return yy_sname[yysymbol];
}
#endif

#define YYPACT_NINF (-139)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     -26,    -6,    68,    67,  -139,  -139,    82,     1,  -139,    21,
      96,    98,   104,   105,  -139,  -139,   107,  -139,  -139,  -139,
    -139,  -139,   109,   843,   113,   114,   115,  1077,  -139,  1043,
    1043,  1043,   123,   124,   108,   121,   122,   127,   125,  -139,
    1787,   943,  1822,  1043,  1043,  -139,  -139,   128,    76,  1822,
     131,   132,   141,   147,  -139,  -139,  -139,  -139,  -139,  -139,
    -139,  -139,  -139,  -139,  -139,  -139,  -139,  -139,  -139,  -139,
    -139,  -139,  -139,  -139,  -139,  -139,  -139,  1178,  -139,  -139,
    -139,  -139,  -139,  -139,  -139,  -139,    -5,  -139,  -139,  -139,
    -139,    90,  -139,  -139,    95,    -2,   150,  1146,    -2,  1043,
    1043,  -139,  -139,  -139,  1043,  -139,  1822,    97,  -139,   100,
    -139,  1205,  -139,    -2,    -2,  1043,    -4,  -139,  1043,  1043,
    1043,  1043,  1043,  1043,  1043,  1043,  1043,  1043,  1043,  1043,
    -139,  1043,  1043,  1043,  1043,  1043,  -139,  -139,  1043,  1043,
    1043,  1043,  1043,  -139,  1043,     9,   152,    11,  -139,  1235,
    1262,   243,  1292,  -139,   151,     2,  -139,    49,  1763,  1752,
    -139,  1318,  1375,  1396,  1452,   646,   646,    -2,    -2,   249,
     249,    -1,    -2,   546,   546,   446,   346,   249,     3,     3,
       3,     3,     3,    51,  1043,  1752,  -139,  1043,   -72,  -139,
     153,   154,   142,   155,  -139,  1043,  -139,  -139,  -139,   112,
    1857,    69,  -139,   117,  1043,  -139,  1043,  1043,  -139,  1473,
      93,   144,  -139,  -139,  -139,  -139,  -139,  -139,  -139,  -139,
    -139,  -139,  -139,  -139,   163,  -139,  -139,   157,  -139,  1763,
    -139,   118,   119,   101,  1752,  -139,  1530,  1551,  1608,  -139,
     103,    12,  -139,    55,   343,  1043,   443,  -139,  -139,  -139,
    -139,  -139,  -139,  1043,  -139,  -139,  -139,   164,  -139,  -139,
    1043,   171,  -139,  -139,  1629,   160,   543,  1686,   643,  1043,
    1707,  -139,  -139,   170,  -139,  -139,  -139,    28,  -139,   843,
    -139,   185,   843,   743,  -139,  -139
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     0,     0,     0,     1,     6,     0,     3,     2,     0,
       0,     0,     0,     0,     5,     4,     0,    78,    78,    78,
      78,    13,     0,    11,     0,     0,     0,     0,     7,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   125,
       0,     0,     0,     0,     0,    76,    77,    75,     0,     0,
       0,     0,     0,     0,   120,   121,   123,   122,   124,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   110,   108,
     107,   109,    92,    81,    82,    80,    88,     0,    79,    83,
      84,    90,    85,    86,    87,    91,     0,     8,     9,    10,
      12,     0,    14,    17,     0,    62,     0,     0,    61,     0,
       0,    78,   103,   102,     0,   106,     0,     0,    19,     0,
     104,     0,    21,    63,    64,     0,     0,    18,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      89,     0,     0,     0,     0,     0,    65,    66,     0,     0,
       0,     0,     0,    74,     0,     0,     0,     0,    71,     0,
       0,     0,     0,    20,     0,     0,   105,     0,    41,    34,
      26,     0,     0,     0,     0,    43,    46,    48,    50,    56,
      57,    45,    52,    54,    55,    59,    60,    58,    44,    47,
      49,    51,    53,     0,     0,    34,    22,     0,     0,    16,
       0,     0,     0,     0,    73,     0,    27,    28,    29,     0,
       0,     0,    35,     0,     0,    68,     0,     0,    72,     0,
       0,     0,   126,   127,   128,   129,   130,   137,   136,   135,
     131,   132,   133,   134,     0,    98,    78,     0,    78,    42,
      39,     0,     0,     0,     0,    38,     0,     0,     0,    23,
       0,     0,    15,     0,     0,     0,     0,    40,    37,    33,
      78,    36,    67,     0,    70,    31,    78,     0,    24,    97,
       0,     0,    99,    95,     0,    94,     0,     0,     0,     0,
       0,    78,    96,     0,    32,    69,    30,     0,    78,   101,
      78,     0,   100,     0,    25,    93
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -139,  -139,  -139,  -139,  -139,    27,   197,  -139,  -139,  -139,
    -139,   -18,  -139,   178,    22,   -25,  -138,   -28,   -96,  -139,
    -139,  -139,  -139,  -139,  -139,  -139,  -139,  -139,  -139,   -23,
    -139
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     2,     6,     7,    14,    22,    72,    27,    92,    73,
      74,    75,   200,    76,   201,   202,   157,    77,    23,    78,
      79,    80,    81,   243,   262,    82,    83,    84,    85,    96,
     224
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      86,    95,    97,    98,    94,   151,   183,   128,   128,   144,
     159,   184,   128,   111,   160,   113,   114,   109,   184,   109,
     186,   257,   108,   185,   112,   185,   109,   186,   188,   189,
     258,   117,   212,   213,   214,   215,   216,   217,   218,   219,
     220,   221,   222,   223,   195,    24,    25,    26,   281,     1,
     136,   137,   138,   139,   140,   141,   142,   138,   139,   140,
     141,   142,     3,   145,   194,   195,   208,   195,     4,   143,
     143,   149,   150,     9,   143,   259,   152,    10,    11,    12,
      13,   260,   261,   109,   233,   234,     5,   158,   153,    16,
     161,   162,   163,   164,   165,   166,   167,   168,   169,   170,
     171,   172,     8,   173,   174,   175,   176,   177,   240,   234,
     178,   179,   180,   181,   182,    17,   158,    18,   187,   249,
     250,   255,   256,    19,    20,   187,    21,   101,    86,    28,
     244,   277,   246,    87,    88,    89,   203,    99,   100,   102,
     103,   104,   115,   105,   116,   118,   119,   122,   123,   124,
     125,   126,   127,   128,   266,   120,   209,   129,   146,   211,
     268,   121,   203,   147,   144,   154,   159,   229,   155,   160,
     227,   245,   225,   226,   228,   279,   236,   232,   237,   238,
     230,   242,   282,   269,   283,   235,   247,   248,   271,   280,
     131,   132,   133,   134,   273,   135,   136,   137,   138,   139,
     140,   141,   142,   284,    15,    93,     0,   210,     0,   251,
       0,   203,     0,     0,     0,   143,     0,   264,     0,     0,
       0,    86,     0,    86,     0,   267,     0,     0,     0,     0,
       0,     0,   270,     0,     0,     0,     0,     0,     0,     0,
       0,   158,     0,    86,     0,    86,     0,    29,     0,     0,
       0,     0,   122,   123,   124,   125,    86,    30,   128,    86,
      86,   241,   129,   192,     0,     0,    31,     0,    32,     0,
       0,    33,    34,     0,    35,    36,    37,     0,    38,    39,
      40,     0,     0,     0,     0,     0,     0,    41,    42,     0,
       0,     0,     0,     0,     0,    43,    44,     0,     0,     0,
       0,   136,   137,   138,   139,   140,   141,   142,     0,    45,
      46,    47,     0,    48,     0,     9,    49,     0,     0,     0,
     143,     0,     0,     0,     0,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    29,     0,   122,
     123,   124,   125,   126,   127,   128,     0,    30,     0,   129,
       0,     0,     0,   263,     0,     0,    31,     0,    32,     0,
       0,    33,    34,     0,    35,    36,    37,     0,    38,    39,
      40,     0,     0,     0,     0,     0,     0,    41,    42,     0,
       0,     0,   131,   132,   133,    43,    44,   135,   136,   137,
     138,   139,   140,   141,   142,     0,     0,     0,     0,    45,
      46,    47,     0,    48,     0,     9,    49,   143,     0,     0,
       0,     0,     0,     0,     0,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    29,     0,   122,
     123,   124,   125,   126,   127,   128,     0,    30,     0,   129,
       0,     0,     0,   265,     0,     0,    31,     0,    32,     0,
       0,    33,    34,     0,    35,    36,    37,     0,    38,    39,
      40,     0,     0,     0,     0,     0,     0,    41,    42,     0,
       0,     0,   131,   132,     0,    43,    44,   135,   136,   137,
     138,   139,   140,   141,   142,     0,     0,     0,     0,    45,
      46,    47,     0,    48,     0,     9,    49,   143,     0,     0,
       0,     0,     0,     0,     0,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    29,     0,   122,
     123,   124,   125,   126,   127,   128,     0,    30,     0,   129,
       0,     0,     0,   274,     0,     0,    31,     0,    32,     0,
       0,    33,    34,     0,    35,    36,    37,     0,    38,    39,
      40,     0,     0,     0,     0,     0,     0,    41,    42,     0,
       0,     0,     0,     0,     0,    43,    44,   135,   136,   137,
     138,   139,   140,   141,   142,     0,     0,     0,     0,    45,
      46,    47,     0,    48,     0,     9,    49,   143,     0,     0,
       0,     0,     0,     0,     0,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    29,     0,     0,
       0,   124,   125,     0,     0,   128,     0,    30,     0,   129,
       0,     0,     0,   276,     0,     0,    31,     0,    32,     0,
       0,    33,    34,     0,    35,    36,    37,     0,    38,    39,
      40,     0,     0,     0,     0,     0,     0,    41,    42,     0,
       0,     0,     0,     0,     0,    43,    44,     0,   136,   137,
     138,   139,   140,   141,   142,     0,     0,     0,     0,    45,
      46,    47,     0,    48,     0,     9,    49,   143,     0,     0,
       0,     0,     0,     0,     0,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    29,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    30,     0,     0,
       0,     0,     0,   285,     0,     0,    31,     0,    32,     0,
       0,    33,    34,     0,    35,    36,    37,     0,    38,    39,
      40,     0,     0,     0,     0,     0,     0,    41,    42,     0,
       0,     0,     0,     0,     0,    43,    44,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    45,
      46,    47,     0,    48,     0,     9,    49,     0,     0,     0,
       0,     0,     0,     0,     0,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    29,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    30,     0,     0,
       0,     0,     0,     0,     0,     0,    31,     0,    32,     0,
       0,    33,    34,     0,    35,    36,    37,     0,    38,    39,
      40,     0,     0,     0,     0,     0,     0,    41,    42,     0,
       0,     0,     0,     0,     0,    43,    44,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    45,
      46,    47,     0,    48,     0,     9,    49,     0,     0,     0,
       0,     0,     0,     0,     0,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    29,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    30,     0,     0,
       0,   110,     0,     0,     0,     0,    31,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    39,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    43,    44,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    45,
      46,    47,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    29,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    30,     0,     0,
       0,     0,     0,     0,     0,     0,    31,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    39,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    43,    44,    90,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    45,
      46,    47,     0,    39,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    91,     0,   122,
     123,   124,   125,   126,   127,   128,     0,     0,     0,   129,
       0,   148,     0,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,   122,   123,   124,   125,   126,   127,   128,     0,     0,
       0,   129,   131,   132,   133,   134,   130,   135,   136,   137,
     138,   139,   140,   141,   142,     0,     0,     0,   122,   123,
     124,   125,   126,   127,   128,     0,     0,   143,   129,     0,
       0,     0,     0,   156,   131,   132,   133,   134,     0,   135,
     136,   137,   138,   139,   140,   141,   142,     0,   122,   123,
     124,   125,   126,   127,   128,     0,     0,     0,   129,   143,
     190,   131,   132,   133,   134,     0,   135,   136,   137,   138,
     139,   140,   141,   142,     0,   122,   123,   124,   125,   126,
     127,   128,     0,     0,     0,   129,   143,   191,     0,     0,
       0,   131,   132,   133,   134,     0,   135,   136,   137,   138,
     139,   140,   141,   142,     0,   122,   123,   124,   125,   126,
     127,   128,     0,     0,     0,   129,   143,   193,   131,   132,
     133,   134,     0,   135,   136,   137,   138,   139,   140,   141,
     142,   122,   123,   124,   125,   126,   127,   128,     0,     0,
       0,   129,     0,   143,   204,     0,     0,     0,   131,   132,
     133,   134,     0,   135,   136,   137,   138,   139,   140,   141,
     142,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   143,   131,   132,   133,   134,     0,   135,
     136,   137,   138,   139,   140,   141,   142,     0,   122,   123,
     124,   125,   126,   127,   128,     0,     0,     0,   129,   143,
     205,     0,     0,     0,     0,     0,     0,     0,     0,   122,
     123,   124,   125,   126,   127,   128,     0,     0,     0,   129,
       0,     0,   206,     0,     0,     0,     0,     0,     0,     0,
       0,   131,   132,   133,   134,     0,   135,   136,   137,   138,
     139,   140,   141,   142,     0,     0,     0,     0,     0,     0,
       0,     0,   131,   132,   133,   134,   143,   135,   136,   137,
     138,   139,   140,   141,   142,   122,   123,   124,   125,   126,
     127,   128,     0,     0,     0,   129,     0,   143,   207,     0,
       0,     0,     0,     0,     0,     0,   122,   123,   124,   125,
     126,   127,   128,     0,     0,     0,   129,     0,     0,     0,
       0,   239,     0,     0,     0,     0,     0,     0,   131,   132,
     133,   134,     0,   135,   136,   137,   138,   139,   140,   141,
     142,     0,     0,     0,     0,     0,     0,     0,     0,   131,
     132,   133,   134,   143,   135,   136,   137,   138,   139,   140,
     141,   142,     0,   122,   123,   124,   125,   126,   127,   128,
       0,     0,     0,   129,   143,   252,     0,     0,     0,     0,
       0,     0,     0,     0,   122,   123,   124,   125,   126,   127,
     128,     0,     0,     0,   129,     0,     0,   253,     0,     0,
       0,     0,     0,     0,     0,     0,   131,   132,   133,   134,
       0,   135,   136,   137,   138,   139,   140,   141,   142,     0,
       0,     0,     0,     0,     0,     0,     0,   131,   132,   133,
     134,   143,   135,   136,   137,   138,   139,   140,   141,   142,
       0,   122,   123,   124,   125,   126,   127,   128,     0,     0,
       0,   129,   143,   254,     0,     0,     0,     0,     0,     0,
       0,     0,   122,   123,   124,   125,   126,   127,   128,     0,
       0,     0,   129,     0,   272,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   131,   132,   133,   134,     0,   135,
     136,   137,   138,   139,   140,   141,   142,     0,     0,     0,
       0,     0,     0,     0,     0,   131,   132,   133,   134,   143,
     135,   136,   137,   138,   139,   140,   141,   142,     0,   122,
     123,   124,   125,   126,   127,   128,     0,     0,     0,   129,
     143,   275,     0,     0,     0,     0,     0,     0,     0,     0,
     122,   123,   124,   125,   126,   127,   128,     0,     0,     0,
     129,     0,     0,     0,   278,     0,     0,     0,     0,     0,
       0,     0,   131,   132,   133,   134,     0,   135,   136,   137,
     138,   139,   140,   141,   142,     0,     0,     0,     0,     0,
       0,     0,     0,   131,   132,   133,   134,   143,   135,   136,
     137,   138,   139,   140,   141,   142,   122,   123,   124,   125,
     126,   127,   128,     0,     0,     0,   129,     0,   143,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    39,     0,
     196,   197,   198,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   131,
     132,   133,   134,     0,   135,   136,   137,   138,   139,   140,
     141,   142,   199,    39,     0,     0,     0,     0,     0,     0,
       0,     0,   106,     0,   143,     0,     0,     0,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,     0,   107,    39,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,     0,   107,    39,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,     0,   231,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71
};

static const yytype_int16 yycheck[] =
{
      23,    29,    30,    31,    27,   101,   144,     9,     9,    14,
      14,     9,     9,    41,    18,    43,    44,    40,     9,    42,
      18,     9,    40,    14,    42,    14,    49,    18,    17,    18,
      18,    49,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,    16,    18,    19,    20,    20,    75,
      52,    53,    54,    55,    56,    57,    58,    54,    55,    56,
      57,    58,    68,    68,    15,    16,    15,    16,     0,    71,
      71,    99,   100,    72,    71,    20,   104,    76,    77,    78,
      79,    26,    27,   106,    15,    16,    19,   115,   106,    68,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
     128,   129,    20,   131,   132,   133,   134,   135,    15,    16,
     138,   139,   140,   141,   142,    19,   144,    19,   116,    18,
      19,    18,    19,    19,    19,   116,    19,    19,   151,    20,
     226,   269,   228,    20,    20,    20,   159,    14,    14,    18,
      18,    14,    14,    18,    68,    14,    14,     3,     4,     5,
       6,     7,     8,     9,   250,    14,   184,    13,    68,   187,
     256,    14,   185,    68,    14,    68,    14,   195,    68,    18,
      28,    14,    19,    19,    19,   271,   204,   200,   206,   207,
      68,    18,   278,    19,   280,    68,    68,    68,    17,    19,
      46,    47,    48,    49,    34,    51,    52,    53,    54,    55,
      56,    57,    58,    18,     7,    27,    -1,   185,    -1,   234,
      -1,   234,    -1,    -1,    -1,    71,    -1,   245,    -1,    -1,
      -1,   244,    -1,   246,    -1,   253,    -1,    -1,    -1,    -1,
      -1,    -1,   260,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   269,    -1,   266,    -1,   268,    -1,     4,    -1,    -1,
      -1,    -1,     3,     4,     5,     6,   279,    14,     9,   282,
     283,   117,    13,    20,    -1,    -1,    23,    -1,    25,    -1,
      -1,    28,    29,    -1,    31,    32,    33,    -1,    35,    36,
      37,    -1,    -1,    -1,    -1,    -1,    -1,    44,    45,    -1,
      -1,    -1,    -1,    -1,    -1,    52,    53,    -1,    -1,    -1,
      -1,    52,    53,    54,    55,    56,    57,    58,    -1,    66,
      67,    68,    -1,    70,    -1,    72,    73,    -1,    -1,    -1,
      71,    -1,    -1,    -1,    -1,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,     4,    -1,     3,
       4,     5,     6,     7,     8,     9,    -1,    14,    -1,    13,
      -1,    -1,    -1,    20,    -1,    -1,    23,    -1,    25,    -1,
      -1,    28,    29,    -1,    31,    32,    33,    -1,    35,    36,
      37,    -1,    -1,    -1,    -1,    -1,    -1,    44,    45,    -1,
      -1,    -1,    46,    47,    48,    52,    53,    51,    52,    53,
      54,    55,    56,    57,    58,    -1,    -1,    -1,    -1,    66,
      67,    68,    -1,    70,    -1,    72,    73,    71,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,     4,    -1,     3,
       4,     5,     6,     7,     8,     9,    -1,    14,    -1,    13,
      -1,    -1,    -1,    20,    -1,    -1,    23,    -1,    25,    -1,
      -1,    28,    29,    -1,    31,    32,    33,    -1,    35,    36,
      37,    -1,    -1,    -1,    -1,    -1,    -1,    44,    45,    -1,
      -1,    -1,    46,    47,    -1,    52,    53,    51,    52,    53,
      54,    55,    56,    57,    58,    -1,    -1,    -1,    -1,    66,
      67,    68,    -1,    70,    -1,    72,    73,    71,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,     4,    -1,     3,
       4,     5,     6,     7,     8,     9,    -1,    14,    -1,    13,
      -1,    -1,    -1,    20,    -1,    -1,    23,    -1,    25,    -1,
      -1,    28,    29,    -1,    31,    32,    33,    -1,    35,    36,
      37,    -1,    -1,    -1,    -1,    -1,    -1,    44,    45,    -1,
      -1,    -1,    -1,    -1,    -1,    52,    53,    51,    52,    53,
      54,    55,    56,    57,    58,    -1,    -1,    -1,    -1,    66,
      67,    68,    -1,    70,    -1,    72,    73,    71,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,     4,    -1,    -1,
      -1,     5,     6,    -1,    -1,     9,    -1,    14,    -1,    13,
      -1,    -1,    -1,    20,    -1,    -1,    23,    -1,    25,    -1,
      -1,    28,    29,    -1,    31,    32,    33,    -1,    35,    36,
      37,    -1,    -1,    -1,    -1,    -1,    -1,    44,    45,    -1,
      -1,    -1,    -1,    -1,    -1,    52,    53,    -1,    52,    53,
      54,    55,    56,    57,    58,    -1,    -1,    -1,    -1,    66,
      67,    68,    -1,    70,    -1,    72,    73,    71,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,     4,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    14,    -1,    -1,
      -1,    -1,    -1,    20,    -1,    -1,    23,    -1,    25,    -1,
      -1,    28,    29,    -1,    31,    32,    33,    -1,    35,    36,
      37,    -1,    -1,    -1,    -1,    -1,    -1,    44,    45,    -1,
      -1,    -1,    -1,    -1,    -1,    52,    53,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    66,
      67,    68,    -1,    70,    -1,    72,    73,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,     4,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    14,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    23,    -1,    25,    -1,
      -1,    28,    29,    -1,    31,    32,    33,    -1,    35,    36,
      37,    -1,    -1,    -1,    -1,    -1,    -1,    44,    45,    -1,
      -1,    -1,    -1,    -1,    -1,    52,    53,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    66,
      67,    68,    -1,    70,    -1,    72,    73,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,     4,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    14,    -1,    -1,
      -1,    18,    -1,    -1,    -1,    -1,    23,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    36,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    52,    53,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    66,
      67,    68,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,     4,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    14,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    23,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    36,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    52,    53,    20,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    66,
      67,    68,    -1,    36,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,    70,    -1,     3,
       4,     5,     6,     7,     8,     9,    -1,    -1,    -1,    13,
      -1,    15,    -1,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,     3,     4,     5,     6,     7,     8,     9,    -1,    -1,
      -1,    13,    46,    47,    48,    49,    18,    51,    52,    53,
      54,    55,    56,    57,    58,    -1,    -1,    -1,     3,     4,
       5,     6,     7,     8,     9,    -1,    -1,    71,    13,    -1,
      -1,    -1,    -1,    18,    46,    47,    48,    49,    -1,    51,
      52,    53,    54,    55,    56,    57,    58,    -1,     3,     4,
       5,     6,     7,     8,     9,    -1,    -1,    -1,    13,    71,
      15,    46,    47,    48,    49,    -1,    51,    52,    53,    54,
      55,    56,    57,    58,    -1,     3,     4,     5,     6,     7,
       8,     9,    -1,    -1,    -1,    13,    71,    15,    -1,    -1,
      -1,    46,    47,    48,    49,    -1,    51,    52,    53,    54,
      55,    56,    57,    58,    -1,     3,     4,     5,     6,     7,
       8,     9,    -1,    -1,    -1,    13,    71,    15,    46,    47,
      48,    49,    -1,    51,    52,    53,    54,    55,    56,    57,
      58,     3,     4,     5,     6,     7,     8,     9,    -1,    -1,
      -1,    13,    -1,    71,    16,    -1,    -1,    -1,    46,    47,
      48,    49,    -1,    51,    52,    53,    54,    55,    56,    57,
      58,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    71,    46,    47,    48,    49,    -1,    51,
      52,    53,    54,    55,    56,    57,    58,    -1,     3,     4,
       5,     6,     7,     8,     9,    -1,    -1,    -1,    13,    71,
      15,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,
       4,     5,     6,     7,     8,     9,    -1,    -1,    -1,    13,
      -1,    -1,    16,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    46,    47,    48,    49,    -1,    51,    52,    53,    54,
      55,    56,    57,    58,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    46,    47,    48,    49,    71,    51,    52,    53,
      54,    55,    56,    57,    58,     3,     4,     5,     6,     7,
       8,     9,    -1,    -1,    -1,    13,    -1,    71,    16,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,     3,     4,     5,     6,
       7,     8,     9,    -1,    -1,    -1,    13,    -1,    -1,    -1,
      -1,    18,    -1,    -1,    -1,    -1,    -1,    -1,    46,    47,
      48,    49,    -1,    51,    52,    53,    54,    55,    56,    57,
      58,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    46,
      47,    48,    49,    71,    51,    52,    53,    54,    55,    56,
      57,    58,    -1,     3,     4,     5,     6,     7,     8,     9,
      -1,    -1,    -1,    13,    71,    15,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,     3,     4,     5,     6,     7,     8,
       9,    -1,    -1,    -1,    13,    -1,    -1,    16,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    46,    47,    48,    49,
      -1,    51,    52,    53,    54,    55,    56,    57,    58,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    46,    47,    48,
      49,    71,    51,    52,    53,    54,    55,    56,    57,    58,
      -1,     3,     4,     5,     6,     7,     8,     9,    -1,    -1,
      -1,    13,    71,    15,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,     3,     4,     5,     6,     7,     8,     9,    -1,
      -1,    -1,    13,    -1,    15,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    46,    47,    48,    49,    -1,    51,
      52,    53,    54,    55,    56,    57,    58,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    46,    47,    48,    49,    71,
      51,    52,    53,    54,    55,    56,    57,    58,    -1,     3,
       4,     5,     6,     7,     8,     9,    -1,    -1,    -1,    13,
      71,    15,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
       3,     4,     5,     6,     7,     8,     9,    -1,    -1,    -1,
      13,    -1,    -1,    -1,    17,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    46,    47,    48,    49,    -1,    51,    52,    53,
      54,    55,    56,    57,    58,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    46,    47,    48,    49,    71,    51,    52,
      53,    54,    55,    56,    57,    58,     3,     4,     5,     6,
       7,     8,     9,    -1,    -1,    -1,    13,    -1,    71,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    36,    -1,
      38,    39,    40,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    46,
      47,    48,    49,    -1,    51,    52,    53,    54,    55,    56,
      57,    58,    70,    36,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    45,    -1,    71,    -1,    -1,    -1,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,    -1,    70,    36,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,    -1,    70,    36,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,    -1,    70,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    75,   119,    68,     0,    19,   120,   121,    20,    72,
      76,    77,    78,    79,   122,   124,    68,    19,    19,    19,
      19,    19,   123,   136,   123,   123,   123,   125,    20,     4,
      14,    23,    25,    28,    29,    31,    32,    33,    35,    36,
      37,    44,    45,    52,    53,    66,    67,    68,    70,    73,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   124,   127,   128,   129,   131,   135,   137,   138,
     139,   140,   143,   144,   145,   146,   147,    20,    20,    20,
      20,    70,   126,   131,   147,   135,   147,   135,   135,    14,
      14,    19,    18,    18,    14,    18,    45,    70,   129,   147,
      18,   135,   129,   135,   135,    14,    68,   129,    14,    14,
      14,    14,     3,     4,     5,     6,     7,     8,     9,    13,
      18,    46,    47,    48,    49,    51,    52,    53,    54,    55,
      56,    57,    58,    71,    14,    68,    68,    68,    15,   135,
     135,   136,   135,   129,    68,    68,    18,   134,   135,    14,
      18,   135,   135,   135,   135,   135,   135,   135,   135,   135,
     135,   135,   135,   135,   135,   135,   135,   135,   135,   135,
     135,   135,   135,   134,     9,    14,    18,   116,    17,    18,
      15,    15,    20,    15,    15,    16,    38,    39,    40,    70,
     130,   132,   133,   147,    16,    15,    16,    16,    15,   135,
     132,   135,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   148,    19,    19,    28,    19,   135,
      68,    70,   147,    15,    16,    68,   135,   135,   135,    18,
      15,   117,    18,   141,   136,    14,   136,    68,    68,    18,
      19,   133,    15,    16,    15,    18,    19,     9,    18,    20,
      26,    27,   142,    20,   135,    20,   136,   135,   136,    19,
     135,    17,    15,    34,    20,    15,    20,   134,    17,   136,
      19,    20,   136,   136,    18,    20
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_uint8 yyr1[] =
{
       0,   118,   119,   120,   121,   121,   121,   122,   122,   122,
     122,   123,   124,   125,   125,   126,   126,   126,   127,   128,
     128,   128,   129,   129,   129,   129,   129,   130,   130,   130,
     131,   131,   131,   131,   132,   132,   132,   133,   133,   133,
     133,   134,   134,   135,   135,   135,   135,   135,   135,   135,
     135,   135,   135,   135,   135,   135,   135,   135,   135,   135,
     135,   135,   135,   135,   135,   135,   135,   135,   135,   135,
     135,   135,   135,   135,   135,   135,   135,   135,   136,   136,
     137,   137,   137,   137,   137,   137,   137,   137,   137,   137,
     137,   137,   137,   138,   138,   139,   139,   140,   141,   141,
     142,   142,   143,   144,   145,   145,   146,   147,   147,   147,
     147,   147,   147,   147,   147,   147,   147,   147,   147,   147,
     147,   147,   147,   147,   147,   147,   148,   148,   148,   148,
     148,   148,   148,   148,   148,   148,   148,   148
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     5,     1,     2,     2,     0,     4,     4,     4,
       4,     1,     5,     0,     2,     5,     3,     1,     2,     2,
       3,     2,     3,     5,     6,    10,     3,     1,     1,     1,
       8,     6,     8,     6,     0,     1,     3,     3,     2,     2,
       3,     1,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     2,     2,     2,     2,     2,     2,     6,     4,     8,
       6,     3,     4,     4,     2,     1,     1,     1,     0,     2,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     2,
       1,     1,     1,    11,     7,     7,     8,     7,     0,     2,
       4,     3,     2,     2,     2,     3,     2,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = TSHADE_EMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == TSHADE_EMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (&yylloc, scanner, shadeAst, YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use TSHADE_error or TSHADE_UNDEF. */
#define YYERRCODE TSHADE_UNDEF

/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;        \
          (Current).first_column = YYRHSLOC (Rhs, 1).first_column;      \
          (Current).last_line    = YYRHSLOC (Rhs, N).last_line;         \
          (Current).last_column  = YYRHSLOC (Rhs, N).last_column;       \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).first_line   = (Current).last_line   =              \
            YYRHSLOC (Rhs, 0).last_line;                                \
          (Current).first_column = (Current).last_column =              \
            YYRHSLOC (Rhs, 0).last_column;                              \
        }                                                               \
    while (0)
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K])


/* Enable debugging if requested.  */
#if TSHADE_DEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)


/* YYLOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

# ifndef YYLOCATION_PRINT

#  if defined YY_LOCATION_PRINT

   /* Temporary convenience wrapper in case some people defined the
      undocumented and private YY_LOCATION_PRINT macros.  */
#   define YYLOCATION_PRINT(File, Loc)  YY_LOCATION_PRINT(File, *(Loc))

#  elif defined TSHADE_LTYPE_IS_TRIVIAL && TSHADE_LTYPE_IS_TRIVIAL

/* Print *YYLOCP on YYO.  Private, do not rely on its existence. */

YY_ATTRIBUTE_UNUSED
static int
yy_location_print_ (FILE *yyo, YYLTYPE const * const yylocp)
{
  int res = 0;
  int end_col = 0 != yylocp->last_column ? yylocp->last_column - 1 : 0;
  if (0 <= yylocp->first_line)
    {
      res += YYFPRINTF (yyo, "%d", yylocp->first_line);
      if (0 <= yylocp->first_column)
        res += YYFPRINTF (yyo, ".%d", yylocp->first_column);
    }
  if (0 <= yylocp->last_line)
    {
      if (yylocp->first_line < yylocp->last_line)
        {
          res += YYFPRINTF (yyo, "-%d", yylocp->last_line);
          if (0 <= end_col)
            res += YYFPRINTF (yyo, ".%d", end_col);
        }
      else if (0 <= end_col && yylocp->first_column < end_col)
        res += YYFPRINTF (yyo, "-%d", end_col);
    }
  return res;
}

#   define YYLOCATION_PRINT  yy_location_print_

    /* Temporary convenience wrapper in case some people defined the
       undocumented and private YY_LOCATION_PRINT macros.  */
#   define YY_LOCATION_PRINT(File, Loc)  YYLOCATION_PRINT(File, &(Loc))

#  else

#   define YYLOCATION_PRINT(File, Loc) ((void) 0)
    /* Temporary convenience wrapper in case some people defined the
       undocumented and private YY_LOCATION_PRINT macros.  */
#   define YY_LOCATION_PRINT  YYLOCATION_PRINT

#  endif
# endif /* !defined YYLOCATION_PRINT */


# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value, Location, scanner, shadeAst); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, yyscan_t scanner, tShadeAst* shadeAst)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  YY_USE (yylocationp);
  YY_USE (scanner);
  YY_USE (shadeAst);
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
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, yyscan_t scanner, tShadeAst* shadeAst)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  YYLOCATION_PRINT (yyo, yylocationp);
  YYFPRINTF (yyo, ": ");
  yy_symbol_value_print (yyo, yykind, yyvaluep, yylocationp, scanner, shadeAst);
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
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp, YYLTYPE *yylsp,
                 int yyrule, yyscan_t scanner, tShadeAst* shadeAst)
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
                       &yyvsp[(yyi + 1) - (yynrhs)],
                       &(yylsp[(yyi + 1) - (yynrhs)]), scanner, shadeAst);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, yylsp, Rule, scanner, shadeAst); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !TSHADE_DEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !TSHADE_DEBUG */


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


/* Context of a parse error.  */
typedef struct
{
  yy_state_t *yyssp;
  yysymbol_kind_t yytoken;
  YYLTYPE *yylloc;
} yypcontext_t;

/* Put in YYARG at most YYARGN of the expected tokens given the
   current YYCTX, and return the number of tokens stored in YYARG.  If
   YYARG is null, return the number of expected tokens (guaranteed to
   be less than YYNTOKENS).  Return YYENOMEM on memory exhaustion.
   Return 0 if there are more than YYARGN expected tokens, yet fill
   YYARG up to YYARGN. */
static int
yypcontext_expected_tokens (const yypcontext_t *yyctx,
                            yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
  int yycount = 0;
  int yyn = yypact[+*yyctx->yyssp];
  if (!yypact_value_is_default (yyn))
    {
      /* Start YYX at -YYN if negative to avoid negative indexes in
         YYCHECK.  In other words, skip the first -YYN actions for
         this state because they are default actions.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;
      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yyx;
      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
        if (yycheck[yyx + yyn] == yyx && yyx != YYSYMBOL_YYerror
            && !yytable_value_is_error (yytable[yyx + yyn]))
          {
            if (!yyarg)
              ++yycount;
            else if (yycount == yyargn)
              return 0;
            else
              yyarg[yycount++] = YY_CAST (yysymbol_kind_t, yyx);
          }
    }
  if (yyarg && yycount == 0 && 0 < yyargn)
    yyarg[0] = YYSYMBOL_YYEMPTY;
  return yycount;
}




/* The kind of the lookahead of this context.  */
static yysymbol_kind_t
yypcontext_token (const yypcontext_t *yyctx) YY_ATTRIBUTE_UNUSED;

static yysymbol_kind_t
yypcontext_token (const yypcontext_t *yyctx)
{
  return yyctx->yytoken;
}

/* The location of the lookahead of this context.  */
static YYLTYPE *
yypcontext_location (const yypcontext_t *yyctx) YY_ATTRIBUTE_UNUSED;

static YYLTYPE *
yypcontext_location (const yypcontext_t *yyctx)
{
  return yyctx->yylloc;
}

/* User defined function to report a syntax error.  */
static int
yyreport_syntax_error (const yypcontext_t *yyctx, yyscan_t scanner, tShadeAst* shadeAst);

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep, YYLTYPE *yylocationp, yyscan_t scanner, tShadeAst* shadeAst)
{
  YY_USE (yyvaluep);
  YY_USE (yylocationp);
  YY_USE (scanner);
  YY_USE (shadeAst);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}






/*----------.
| yyparse.  |
`----------*/

int
yyparse (yyscan_t scanner, tShadeAst* shadeAst)
{
/* Lookahead token kind.  */
int yychar;


/* The semantic value of the lookahead symbol.  */
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
YY_INITIAL_VALUE (static YYSTYPE yyval_default;)
YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

/* Location data for the lookahead symbol.  */
static YYLTYPE yyloc_default
# if defined TSHADE_LTYPE_IS_TRIVIAL && TSHADE_LTYPE_IS_TRIVIAL
  = { 1, 1, 1, 1 }
# endif
;
YYLTYPE yylloc = yyloc_default;

    /* Number of syntax errors so far.  */
    int yynerrs = 0;

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

    /* The location stack: array, bottom, top.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls = yylsa;
    YYLTYPE *yylsp = yyls;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

  /* The locations where the error started and ended.  */
  YYLTYPE yyerror_range[3];



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = TSHADE_EMPTY; /* Cause a token to be read.  */

  yylsp[0] = yylloc;
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
        YYLTYPE *yyls1 = yyls;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yyls1, yysize * YYSIZEOF (*yylsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
        yyls = yyls1;
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
        YYSTACK_RELOCATE (yyls_alloc, yyls);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

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
  if (yychar == TSHADE_EMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex (&yylval, &yylloc, scanner);
    }

  if (yychar <= TSHADE_EOF)
    {
      yychar = TSHADE_EOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == TSHADE_error)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = TSHADE_UNDEF;
      yytoken = YYSYMBOL_YYerror;
      yyerror_range[1] = yylloc;
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
  *++yylsp = yylloc;

  /* Discard the shifted token.  */
  yychar = TSHADE_EMPTY;
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

  /* Default location. */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  yyerror_range[1] = yyloc;
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* program: tSHADERDECLARE VAR_IDENT '{' program_globals '}'  */
#line 142 "tshade.y"
    {(yyval.node) = nullptr; shadeAst->shaderName = (yyvsp[-3].strVal); }
#line 1976 "tshade.cpp"
    break;

  case 3: /* program_globals: program_global_list  */
#line 147 "tshade.y"
    {(yyval.node) = nullptr; }
#line 1982 "tshade.cpp"
    break;

  case 4: /* program_global_list: program_global_list struct_decl  */
#line 152 "tshade.y"
    {
      shadeAst->addDataStruct((yyvsp[0].structNode));
    }
#line 1990 "tshade.cpp"
    break;

  case 5: /* program_global_list: program_global_list shader_stage  */
#line 156 "tshade.y"
    {
      (yyval.node) = nullptr;
    }
#line 1998 "tshade.cpp"
    break;

  case 6: /* program_global_list: %empty  */
#line 160 "tshade.y"
    {
      (yyval.node) = nullptr;
    }
#line 2006 "tshade.cpp"
    break;

  case 7: /* shader_stage: tVSSHADER '{' shader_body '}'  */
#line 167 "tshade.y"
    { 
      shadeAst->currentStage = ShaderStageType::tSTAGE_VERTEX;
      shadeAst->mVertStage = new tStageNode(ShaderStageType::tSTAGE_VERTEX, (yyvsp[-1].node));
      shadeAst->currentStage = ShaderStageType::tSTAGE_GLOBAL;
      shadeAst->clearVarDecls();
    }
#line 2017 "tshade.cpp"
    break;

  case 8: /* shader_stage: tPSSHADER '{' shader_body '}'  */
#line 174 "tshade.y"
    {
      shadeAst->currentStage = ShaderStageType::tSTAGE_PIXEL;
      shadeAst->mPixStage = new tStageNode(ShaderStageType::tSTAGE_PIXEL, (yyvsp[-1].node));
      shadeAst->currentStage = ShaderStageType::tSTAGE_GLOBAL;
      shadeAst->clearVarDecls();
    }
#line 2028 "tshade.cpp"
    break;

  case 9: /* shader_stage: tGSSHADER '{' shader_body '}'  */
#line 181 "tshade.y"
    {
      shadeAst->currentStage = ShaderStageType::tSTAGE_GEOMETRY;
      shadeAst->mPixStage = new tStageNode(ShaderStageType::tSTAGE_GEOMETRY, (yyvsp[-1].node));
      shadeAst->currentStage = ShaderStageType::tSTAGE_GLOBAL;
      shadeAst->clearVarDecls();
    }
#line 2039 "tshade.cpp"
    break;

  case 10: /* shader_stage: tCSSHADER '{' shader_body '}'  */
#line 188 "tshade.y"
    {
      shadeAst->currentStage = ShaderStageType::tSTAGE_COMPUTE;
      shadeAst->mPixStage = new tStageNode(ShaderStageType::tSTAGE_COMPUTE, (yyvsp[-1].node));
      shadeAst->currentStage = ShaderStageType::tSTAGE_GLOBAL;
      shadeAst->clearVarDecls();
    }
#line 2050 "tshade.cpp"
    break;

  case 11: /* shader_body: statement_list  */
#line 198 "tshade.y"
    {(yyval.node) = (yyvsp[0].stmt_list_node);}
#line 2056 "tshade.cpp"
    break;

  case 12: /* struct_decl: tSTRUCT VAR_IDENT '{' structbody_list '}'  */
#line 203 "tshade.y"
    { (yyval.structNode) = new tStructNode((yyvsp[-3].strVal), (yyvsp[-1].stmt_list_node)); }
#line 2062 "tshade.cpp"
    break;

  case 13: /* structbody_list: %empty  */
#line 208 "tshade.y"
    { (yyval.stmt_list_node) = new tStatementListNode(); }
#line 2068 "tshade.cpp"
    break;

  case 14: /* structbody_list: structbody_list struct_member  */
#line 210 "tshade.y"
    { (yyvsp[-1].stmt_list_node)->addStatement((yyvsp[0].node)); (yyval.stmt_list_node) = (yyvsp[-1].stmt_list_node); }
#line 2074 "tshade.cpp"
    break;

  case 15: /* struct_member: var_type VAR_IDENT ':' struct_semantic ';'  */
#line 215 "tshade.y"
    {(yyval.node) = new tStructMemberNode((yyvsp[-3].strVal), (yyvsp[-4].varType), (yyvsp[-1].semtype), yylval.intVal); }
#line 2080 "tshade.cpp"
    break;

  case 16: /* struct_member: var_type VAR_IDENT ';'  */
#line 217 "tshade.y"
    {(yyval.node) = new tStructMemberNode((yyvsp[-1].strVal), (yyvsp[-2].varType)); }
#line 2086 "tshade.cpp"
    break;

  case 17: /* struct_member: function_def  */
#line 219 "tshade.y"
    {(yyval.node) = (yyvsp[0].funcNode);}
#line 2092 "tshade.cpp"
    break;

  case 18: /* uniform_decl: tUNIFORM var_decl  */
#line 224 "tshade.y"
  {
    (yyvsp[0].declNode)->isUniform = true;
    (yyval.declNode) = (yyvsp[0].declNode);
  }
#line 2101 "tshade.cpp"
    break;

  case 19: /* static_const_decl: rwSTATIC var_decl  */
#line 232 "tshade.y"
  {
    (yyvsp[0].declNode)->isStatic = true;
    (yyval.declNode) = (yyvsp[0].declNode);
  }
#line 2110 "tshade.cpp"
    break;

  case 20: /* static_const_decl: rwSTATIC rwCONST var_decl  */
#line 237 "tshade.y"
  {
    (yyvsp[0].declNode)->isStatic = true;
    (yyvsp[0].declNode)->isConst = true;
    (yyval.declNode) = (yyvsp[0].declNode);
  }
#line 2120 "tshade.cpp"
    break;

  case 21: /* static_const_decl: rwCONST var_decl  */
#line 243 "tshade.y"
  {
    (yyvsp[0].declNode)->isConst = true;
    (yyval.declNode) = (yyvsp[0].declNode);
  }
#line 2129 "tshade.cpp"
    break;

  case 22: /* var_decl: var_type VAR_IDENT ';'  */
#line 251 "tshade.y"
    {(yyval.declNode) = new tVarDeclNode((yyvsp[-1].strVal), (yyvsp[-2].varType)); shadeAst->addVarDecl((yyval.declNode)); }
#line 2135 "tshade.cpp"
    break;

  case 23: /* var_decl: var_type VAR_IDENT '=' expression ';'  */
#line 253 "tshade.y"
    {(yyval.declNode) = new tVarDeclNode((yyvsp[-3].strVal), (yyvsp[-4].varType), (yyvsp[-1].node)); shadeAst->addVarDecl((yyval.declNode)); }
#line 2141 "tshade.cpp"
    break;

  case 24: /* var_decl: var_type VAR_IDENT '[' expression ']' ';'  */
#line 255 "tshade.y"
    {(yyval.declNode) = new tVarDeclNode((yyvsp[-4].strVal), (yyvsp[-5].varType), nullptr, (yyvsp[-2].node)); shadeAst->addVarDecl((yyval.declNode)); }
#line 2147 "tshade.cpp"
    break;

  case 25: /* var_decl: var_type VAR_IDENT '[' expression ']' '=' '{' expression_list '}' ';'  */
#line 257 "tshade.y"
    {(yyval.declNode) = new tVarDeclNode((yyvsp[-8].strVal), (yyvsp[-9].varType), (yyvsp[-2].exprListnode), (yyvsp[-6].node)); shadeAst->addVarDecl((yyval.declNode)); }
#line 2153 "tshade.cpp"
    break;

  case 26: /* var_decl: TYPE_IDENT VAR_IDENT ';'  */
#line 259 "tshade.y"
    {(yyval.declNode) = new tVarDeclNode((yyvsp[-1].strVal), ShaderVarType::tTYPE_STRUCT, nullptr, nullptr, true); (yyval.declNode)->structName = (yyvsp[-2].strVal); shadeAst->addVarDecl((yyval.declNode)); }
#line 2159 "tshade.cpp"
    break;

  case 27: /* param_modifier: rwIN  */
#line 264 "tshade.y"
    { (yyval.modifier) = ParamModifier::PARAM_MOD_IN; }
#line 2165 "tshade.cpp"
    break;

  case 28: /* param_modifier: rwOUT  */
#line 266 "tshade.y"
    { (yyval.modifier) = ParamModifier::PARAM_MOD_OUT; }
#line 2171 "tshade.cpp"
    break;

  case 29: /* param_modifier: rwINOUT  */
#line 268 "tshade.y"
    { (yyval.modifier) = ParamModifier::PARAM_MOD_INOUT; }
#line 2177 "tshade.cpp"
    break;

  case 30: /* function_def: var_type VAR_IDENT '(' function_param_list ')' '{' statement_list '}'  */
#line 273 "tshade.y"
    { (yyval.funcNode) = new tFunctionDefNode((yyvsp[-6].strVal), (yyvsp[-7].varType), (yyvsp[-4].funcList), (yyvsp[-1].stmt_list_node)); shadeAst->addfunction((yyval.funcNode)); }
#line 2183 "tshade.cpp"
    break;

  case 31: /* function_def: var_type VAR_IDENT '(' function_param_list ')' ';'  */
#line 275 "tshade.y"
    { (yyval.funcNode) = new tFunctionDeclNode((yyvsp[-4].strVal), (yyvsp[-5].varType), (yyvsp[-2].funcList)); shadeAst->addfunction((yyval.funcNode)); }
#line 2189 "tshade.cpp"
    break;

  case 32: /* function_def: TYPE_IDENT VAR_IDENT '(' function_param_list ')' '{' statement_list '}'  */
#line 277 "tshade.y"
    { (yyval.funcNode) = new tFunctionDefNode((yyvsp[-6].strVal), ShaderVarType::tTYPE_STRUCT, (yyvsp[-4].funcList), (yyvsp[-1].stmt_list_node)); shadeAst->addfunction((yyval.funcNode)); 
      (yyval.funcNode)->structName = (yyvsp[-7].strVal);
    }
#line 2197 "tshade.cpp"
    break;

  case 33: /* function_def: TYPE_IDENT VAR_IDENT '(' function_param_list ')' ';'  */
#line 281 "tshade.y"
    { (yyval.funcNode) = new tFunctionDeclNode((yyvsp[-4].strVal), ShaderVarType::tTYPE_STRUCT, (yyvsp[-2].funcList)); shadeAst->addfunction((yyval.funcNode)); 
      (yyval.funcNode)->structName = (yyvsp[-5].strVal);
    }
#line 2205 "tshade.cpp"
    break;

  case 34: /* function_param_list: %empty  */
#line 288 "tshade.y"
    { (yyval.funcList) = new tFunctionParamListNode(); }
#line 2211 "tshade.cpp"
    break;

  case 35: /* function_param_list: function_param  */
#line 290 "tshade.y"
    { (yyval.funcList) = new tFunctionParamListNode(); (yyval.funcList)->addParam((yyvsp[0].funcParam)); }
#line 2217 "tshade.cpp"
    break;

  case 36: /* function_param_list: function_param_list ',' function_param  */
#line 292 "tshade.y"
    {(yyvsp[-2].funcList)->addParam((yyvsp[0].funcParam)); (yyval.funcList) = (yyvsp[-2].funcList); }
#line 2223 "tshade.cpp"
    break;

  case 37: /* function_param: param_modifier var_type VAR_IDENT  */
#line 297 "tshade.y"
    { (yyval.funcParam) = new tFunctionParamNode((yyvsp[0].strVal), (yyvsp[-1].varType), (yyvsp[-2].modifier)); }
#line 2229 "tshade.cpp"
    break;

  case 38: /* function_param: var_type VAR_IDENT  */
#line 299 "tshade.y"
    { (yyval.funcParam) = new tFunctionParamNode((yyvsp[0].strVal), (yyvsp[-1].varType), ParamModifier::PARAM_MOD_NONE); }
#line 2235 "tshade.cpp"
    break;

  case 39: /* function_param: TYPE_IDENT VAR_IDENT  */
#line 301 "tshade.y"
    { 
      (yyval.funcParam) = new tFunctionParamNode((yyvsp[0].strVal), ShaderVarType::tTYPE_STRUCT, ParamModifier::PARAM_MOD_NONE); (yyval.funcParam)->structName = (yyvsp[-1].strVal);  
      tVarDeclNode* paramDecl = new tVarDeclNode((yyvsp[0].strVal), ShaderVarType::tTYPE_STRUCT, nullptr, nullptr, true); paramDecl->structName = (yyvsp[-1].strVal); shadeAst->addVarDecl(paramDecl); 
    }
#line 2244 "tshade.cpp"
    break;

  case 40: /* function_param: param_modifier TYPE_IDENT VAR_IDENT  */
#line 306 "tshade.y"
    { (yyval.funcParam) = new tFunctionParamNode((yyvsp[0].strVal), ShaderVarType::tTYPE_STRUCT, (yyvsp[-2].modifier)); (yyval.funcParam)->structName = (yyvsp[-1].strVal); }
#line 2250 "tshade.cpp"
    break;

  case 41: /* expression_list: expression  */
#line 311 "tshade.y"
    { (yyval.exprListnode) = new tExpressionListNode(); (yyval.exprListnode)->addExpression((yyvsp[0].node)); }
#line 2256 "tshade.cpp"
    break;

  case 42: /* expression_list: expression_list ',' expression  */
#line 313 "tshade.y"
    { (yyval.exprListnode)->addExpression((yyvsp[0].node)); (yyval.exprListnode) = (yyvsp[-2].exprListnode); }
#line 2262 "tshade.cpp"
    break;

  case 43: /* expression: expression '+' expression  */
#line 318 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("+", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 2268 "tshade.cpp"
    break;

  case 44: /* expression: expression OP_PLUS_ASS expression  */
#line 320 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("+=", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 2274 "tshade.cpp"
    break;

  case 45: /* expression: expression '=' expression  */
#line 322 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("=", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 2280 "tshade.cpp"
    break;

  case 46: /* expression: expression '-' expression  */
#line 324 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("-", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 2286 "tshade.cpp"
    break;

  case 47: /* expression: expression OP_MINUS_ASS expression  */
#line 326 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("-=", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 2292 "tshade.cpp"
    break;

  case 48: /* expression: expression '*' expression  */
#line 328 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("*", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 2298 "tshade.cpp"
    break;

  case 49: /* expression: expression OP_MUL_ASS expression  */
#line 330 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("*=", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 2304 "tshade.cpp"
    break;

  case 50: /* expression: expression '/' expression  */
#line 332 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("/", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 2310 "tshade.cpp"
    break;

  case 51: /* expression: expression OP_DIV_ASS expression  */
#line 334 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("/=", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 2316 "tshade.cpp"
    break;

  case 52: /* expression: expression '%' expression  */
#line 336 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("%", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 2322 "tshade.cpp"
    break;

  case 53: /* expression: expression OP_MOD_ASS expression  */
#line 338 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("%=", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 2328 "tshade.cpp"
    break;

  case 54: /* expression: expression OP_EQ expression  */
#line 340 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("==", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 2334 "tshade.cpp"
    break;

  case 55: /* expression: expression OP_NEQ expression  */
#line 342 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("!=", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 2340 "tshade.cpp"
    break;

  case 56: /* expression: expression '<' expression  */
#line 344 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("<", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 2346 "tshade.cpp"
    break;

  case 57: /* expression: expression '>' expression  */
#line 346 "tshade.y"
    { (yyval.node) = new tBinaryOpNode(">", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 2352 "tshade.cpp"
    break;

  case 58: /* expression: expression OP_GE expression  */
#line 348 "tshade.y"
    { (yyval.node) = new tBinaryOpNode(">=", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 2358 "tshade.cpp"
    break;

  case 59: /* expression: expression OP_AND expression  */
#line 350 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("&&", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 2364 "tshade.cpp"
    break;

  case 60: /* expression: expression OP_OR expression  */
#line 352 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("||", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 2370 "tshade.cpp"
    break;

  case 61: /* expression: '!' expression  */
#line 354 "tshade.y"
    { (yyval.node) = new tUnaryOpNode("!", (yyvsp[0].node)); }
#line 2376 "tshade.cpp"
    break;

  case 62: /* expression: '-' expression  */
#line 356 "tshade.y"
    { (yyval.node) = new tUnaryOpNode("-", (yyvsp[0].node)); }
#line 2382 "tshade.cpp"
    break;

  case 63: /* expression: OP_PLUSPLUS expression  */
#line 358 "tshade.y"
    { (yyval.node) = new tUnaryOpNode("++", (yyvsp[0].node)); }
#line 2388 "tshade.cpp"
    break;

  case 64: /* expression: OP_MINUSMINUS expression  */
#line 360 "tshade.y"
    { (yyval.node) = new tUnaryOpNode("--", (yyvsp[0].node)); }
#line 2394 "tshade.cpp"
    break;

  case 65: /* expression: expression OP_PLUSPLUS  */
#line 362 "tshade.y"
    { (yyval.node) = new tUnaryOpNode("++", (yyvsp[-1].node), false); }
#line 2400 "tshade.cpp"
    break;

  case 66: /* expression: expression OP_MINUSMINUS  */
#line 364 "tshade.y"
    { (yyval.node) = new tUnaryOpNode("--", (yyvsp[-1].node), false); }
#line 2406 "tshade.cpp"
    break;

  case 67: /* expression: tMULFUNC '(' expression ',' expression ')'  */
#line 366 "tshade.y"
    {(yyval.node) = new tMulNode((yyvsp[-3].node), (yyvsp[-1].node)); }
#line 2412 "tshade.cpp"
    break;

  case 68: /* expression: tFRACFUNC '(' expression ')'  */
#line 368 "tshade.y"
    {(yyval.node) = new tFracNode((yyvsp[-1].node)); }
#line 2418 "tshade.cpp"
    break;

  case 69: /* expression: tLERPFUNC '(' expression ',' expression ',' expression ')'  */
#line 370 "tshade.y"
    {(yyval.node) = new tLerpNode((yyvsp[-5].node), (yyvsp[-3].node), (yyvsp[-1].node)); }
#line 2424 "tshade.cpp"
    break;

  case 70: /* expression: tSAMPLE '(' expression ',' expression ')'  */
#line 372 "tshade.y"
    {(yyval.node) = new tSampleNode((yyvsp[-3].node), (yyvsp[-1].node)); }
#line 2430 "tshade.cpp"
    break;

  case 71: /* expression: '(' expression ')'  */
#line 374 "tshade.y"
    { (yyval.node) = (yyvsp[-1].node); }
#line 2436 "tshade.cpp"
    break;

  case 72: /* expression: var_type '(' expression_list ')'  */
#line 376 "tshade.y"
    { (yyval.node) = new tTypeRefNode((yyvsp[-3].varType), (yyvsp[-1].exprListnode)); }
#line 2442 "tshade.cpp"
    break;

  case 73: /* expression: VAR_IDENT '(' expression_list ')'  */
#line 378 "tshade.y"
    {
      tFunctionNode* funcDecl = shadeAst->findFunction((yyvsp[-3].strVal));
      if (funcDecl) {
          (yyval.node) = new tFunctionRefNode(funcDecl, (yyvsp[-1].exprListnode));
      } else {
          yyerror(&yylloc, scanner, shadeAst, "Undefined function");
          (yyval.node) = nullptr;  // Handle error appropriately
      }
    }
#line 2456 "tshade.cpp"
    break;

  case 74: /* expression: expression MEMBER_VAR  */
#line 388 "tshade.y"
    { (yyval.node) = new tAccessNode((yyvsp[0].strVal));}
#line 2462 "tshade.cpp"
    break;

  case 75: /* expression: VAR_IDENT  */
#line 390 "tshade.y"
    { tVarDeclNode* varDecl = shadeAst->findVar((yyvsp[0].strVal));
      if (varDecl) {
          (yyval.node) = new tVarRefNode(varDecl);
      } else {
          yyerror(&yylloc, scanner, shadeAst, "Undefined variable");
          (yyval.node) = nullptr;  // Handle error appropriately
      } 
    }
#line 2475 "tshade.cpp"
    break;

  case 76: /* expression: INT_NUM  */
#line 399 "tshade.y"
    { (yyval.node) = new tIntLiteralNode((yyvsp[0].intVal)); }
#line 2481 "tshade.cpp"
    break;

  case 77: /* expression: FLOAT_NUM  */
#line 401 "tshade.y"
    { (yyval.node) = new tFloatLiteralNode((yyvsp[0].fVal)); }
#line 2487 "tshade.cpp"
    break;

  case 78: /* statement_list: %empty  */
#line 406 "tshade.y"
    { (yyval.stmt_list_node) = new tStatementListNode(); }
#line 2493 "tshade.cpp"
    break;

  case 79: /* statement_list: statement_list statement  */
#line 408 "tshade.y"
    { (yyvsp[-1].stmt_list_node)->addStatement((yyvsp[0].node)); (yyval.stmt_list_node) = (yyvsp[-1].stmt_list_node); }
#line 2499 "tshade.cpp"
    break;

  case 80: /* statement: var_decl  */
#line 413 "tshade.y"
    {(yyval.node) = (yyvsp[0].declNode);}
#line 2505 "tshade.cpp"
    break;

  case 81: /* statement: uniform_decl  */
#line 415 "tshade.y"
    {(yyval.node) = (yyvsp[0].declNode);}
#line 2511 "tshade.cpp"
    break;

  case 82: /* statement: static_const_decl  */
#line 417 "tshade.y"
    {(yyval.node) = (yyvsp[0].declNode);}
#line 2517 "tshade.cpp"
    break;

  case 83: /* statement: if_statement  */
#line 419 "tshade.y"
    {(yyval.node) = (yyvsp[0].node);}
#line 2523 "tshade.cpp"
    break;

  case 84: /* statement: while_statement  */
#line 421 "tshade.y"
    {(yyval.node) = (yyvsp[0].node);}
#line 2529 "tshade.cpp"
    break;

  case 85: /* statement: continue_statement  */
#line 423 "tshade.y"
    {(yyval.node) = (yyvsp[0].node);}
#line 2535 "tshade.cpp"
    break;

  case 86: /* statement: break_statement  */
#line 425 "tshade.y"
    {(yyval.node) = (yyvsp[0].node);}
#line 2541 "tshade.cpp"
    break;

  case 87: /* statement: return_statement  */
#line 427 "tshade.y"
    {(yyval.node) = (yyvsp[0].node);}
#line 2547 "tshade.cpp"
    break;

  case 88: /* statement: function_def  */
#line 429 "tshade.y"
    {(yyval.node) = (yyvsp[0].funcNode); }
#line 2553 "tshade.cpp"
    break;

  case 89: /* statement: expression ';'  */
#line 431 "tshade.y"
    {(yyval.node) = (yyvsp[-1].node);}
#line 2559 "tshade.cpp"
    break;

  case 90: /* statement: switch_statement  */
#line 433 "tshade.y"
    {(yyval.node) = (yyvsp[0].node);}
#line 2565 "tshade.cpp"
    break;

  case 91: /* statement: discard_statement  */
#line 435 "tshade.y"
    {(yyval.node) = (yyvsp[0].node);}
#line 2571 "tshade.cpp"
    break;

  case 92: /* statement: struct_decl  */
#line 437 "tshade.y"
    {(yyval.node) = (yyvsp[0].structNode);}
#line 2577 "tshade.cpp"
    break;

  case 93: /* if_statement: rwIF '(' expression ')' '{' statement_list '}' rwELSE '{' statement_list '}'  */
#line 443 "tshade.y"
    { (yyval.node) = new tIfNode((yyvsp[-8].node), (yyvsp[-5].stmt_list_node), (yyvsp[-1].stmt_list_node)); }
#line 2583 "tshade.cpp"
    break;

  case 94: /* if_statement: rwIF '(' expression ')' '{' statement_list '}'  */
#line 445 "tshade.y"
    { (yyval.node) = new tIfNode((yyvsp[-4].node), (yyvsp[-1].stmt_list_node)); }
#line 2589 "tshade.cpp"
    break;

  case 95: /* while_statement: rwWHILE '(' expression ')' '{' statement_list '}'  */
#line 450 "tshade.y"
    { (yyval.node) = new tWhileNode((yyvsp[-4].node), (yyvsp[-1].stmt_list_node)); }
#line 2595 "tshade.cpp"
    break;

  case 96: /* while_statement: rwDO '{' statement_list '}' rwWHILE '(' expression ')'  */
#line 452 "tshade.y"
    { (yyval.node) = new tWhileNode((yyvsp[-1].node), (yyvsp[-5].stmt_list_node), true); }
#line 2601 "tshade.cpp"
    break;

  case 97: /* switch_statement: rwSWITCH '(' expression ')' '{' case_statements '}'  */
#line 457 "tshade.y"
    {(yyval.node) = new tSwitchNode((yyvsp[-4].node), (yyvsp[-1].stmt_list_node)); }
#line 2607 "tshade.cpp"
    break;

  case 98: /* case_statements: %empty  */
#line 462 "tshade.y"
    { (yyval.stmt_list_node) = new tStatementListNode(); }
#line 2613 "tshade.cpp"
    break;

  case 99: /* case_statements: case_statements case_rule  */
#line 464 "tshade.y"
    { (yyvsp[-1].stmt_list_node)->addStatement((yyvsp[0].node)); (yyval.stmt_list_node) = (yyvsp[-1].stmt_list_node); }
#line 2619 "tshade.cpp"
    break;

  case 100: /* case_rule: rwCASE expression ':' statement_list  */
#line 469 "tshade.y"
    {(yyval.node) = new tCaseNode((yyvsp[-2].node), (yyvsp[0].stmt_list_node));}
#line 2625 "tshade.cpp"
    break;

  case 101: /* case_rule: rwDEFAULT ':' statement_list  */
#line 471 "tshade.y"
    { (yyval.node) = new tCaseNode(nullptr, (yyvsp[0].stmt_list_node), true); }
#line 2631 "tshade.cpp"
    break;

  case 102: /* continue_statement: rwCONTINUE ';'  */
#line 476 "tshade.y"
    { (yyval.node) = new tContinueNode(); }
#line 2637 "tshade.cpp"
    break;

  case 103: /* break_statement: rwBREAK ';'  */
#line 481 "tshade.y"
    { (yyval.node) = new tBreakNode(); }
#line 2643 "tshade.cpp"
    break;

  case 104: /* return_statement: rwRETURN ';'  */
#line 486 "tshade.y"
    { (yyval.node) = new tReturnNode(); }
#line 2649 "tshade.cpp"
    break;

  case 105: /* return_statement: rwRETURN expression ';'  */
#line 488 "tshade.y"
    { (yyval.node) = new tReturnNode((yyvsp[-1].node)); }
#line 2655 "tshade.cpp"
    break;

  case 106: /* discard_statement: rwDISCARD ';'  */
#line 493 "tshade.y"
    { (yyval.node) = new tDiscardNode(); }
#line 2661 "tshade.cpp"
    break;

  case 107: /* var_type: tMAT34_TYPE  */
#line 498 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_MAT34;}
#line 2667 "tshade.cpp"
    break;

  case 108: /* var_type: tMAT43_TYPE  */
#line 500 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_MAT43;}
#line 2673 "tshade.cpp"
    break;

  case 109: /* var_type: tMAT3_TYPE  */
#line 502 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_MAT33;}
#line 2679 "tshade.cpp"
    break;

  case 110: /* var_type: tMAT4_TYPE  */
#line 504 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_MAT44;}
#line 2685 "tshade.cpp"
    break;

  case 111: /* var_type: tFVEC2_TYPE  */
#line 506 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_FLOAT2;}
#line 2691 "tshade.cpp"
    break;

  case 112: /* var_type: tFVEC3_TYPE  */
#line 508 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_FLOAT3;}
#line 2697 "tshade.cpp"
    break;

  case 113: /* var_type: tFVEC4_TYPE  */
#line 510 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_FLOAT4;}
#line 2703 "tshade.cpp"
    break;

  case 114: /* var_type: tIVEC2_TYPE  */
#line 512 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_INT2;}
#line 2709 "tshade.cpp"
    break;

  case 115: /* var_type: tIVEC3_TYPE  */
#line 514 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_INT3;}
#line 2715 "tshade.cpp"
    break;

  case 116: /* var_type: tIVEC4_TYPE  */
#line 516 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_INT4;}
#line 2721 "tshade.cpp"
    break;

  case 117: /* var_type: tBVEC2_TYPE  */
#line 518 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_BOOL2;}
#line 2727 "tshade.cpp"
    break;

  case 118: /* var_type: tBVEC3_TYPE  */
#line 520 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_BOOL3;}
#line 2733 "tshade.cpp"
    break;

  case 119: /* var_type: tBVEC4_TYPE  */
#line 522 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_BOOL4;}
#line 2739 "tshade.cpp"
    break;

  case 120: /* var_type: tFLOAT_TYPE  */
#line 524 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_FLOAT;}
#line 2745 "tshade.cpp"
    break;

  case 121: /* var_type: tINT_TYPE  */
#line 526 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_INT;}
#line 2751 "tshade.cpp"
    break;

  case 122: /* var_type: tUINT_TYPE  */
#line 528 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_UINT;}
#line 2757 "tshade.cpp"
    break;

  case 123: /* var_type: tBOOL_TYPE  */
#line 530 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_BOOL;}
#line 2763 "tshade.cpp"
    break;

  case 124: /* var_type: tSAMPLER2D_TYPE  */
#line 532 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_SAMPLER2D;}
#line 2769 "tshade.cpp"
    break;

  case 125: /* var_type: rwVOID  */
#line 534 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_VOID;}
#line 2775 "tshade.cpp"
    break;

  case 126: /* struct_semantic: tSEM_SVPOSITION  */
#line 539 "tshade.y"
    { (yyval.semtype) = SEMANTIC_SV_POSITION; }
#line 2781 "tshade.cpp"
    break;

  case 127: /* struct_semantic: tSEM_POSITION  */
#line 541 "tshade.y"
    { (yyval.semtype) = SEMANTIC_POSITION; }
#line 2787 "tshade.cpp"
    break;

  case 128: /* struct_semantic: tSEM_NORMAL  */
#line 543 "tshade.y"
    { (yyval.semtype) = SEMANTIC_NORMAL; }
#line 2793 "tshade.cpp"
    break;

  case 129: /* struct_semantic: tSEM_BINORMAL  */
#line 545 "tshade.y"
    { (yyval.semtype) = SEMANTIC_BINORMAL; }
#line 2799 "tshade.cpp"
    break;

  case 130: /* struct_semantic: tSEM_TANGENT  */
#line 547 "tshade.y"
    { (yyval.semtype) = SEMANTIC_TANGENT; }
#line 2805 "tshade.cpp"
    break;

  case 131: /* struct_semantic: tSEM_TEXCOORD  */
#line 549 "tshade.y"
    { (yyval.semtype) = SEMANTIC_TEXCOORD; }
#line 2811 "tshade.cpp"
    break;

  case 132: /* struct_semantic: tSEM_COLOR  */
#line 551 "tshade.y"
   { (yyval.semtype) = SEMANTIC_COLOR; }
#line 2817 "tshade.cpp"
    break;

  case 133: /* struct_semantic: tSEM_TARGET  */
#line 553 "tshade.y"
    { (yyval.semtype) = SEMANTIC_SV_TARGET; }
#line 2823 "tshade.cpp"
    break;

  case 134: /* struct_semantic: tSEM_DEPTH  */
#line 555 "tshade.y"
    { (yyval.semtype) = SEMANTIC_SV_DEPTH; }
#line 2829 "tshade.cpp"
    break;

  case 135: /* struct_semantic: tSEM_ISFRONTFACE  */
#line 557 "tshade.y"
    { (yyval.semtype) = SEMANTIC_ISFRONTFACE; }
#line 2835 "tshade.cpp"
    break;

  case 136: /* struct_semantic: tSEM_TESSFACTOR  */
#line 559 "tshade.y"
    { (yyval.semtype) = SEMANTIC_TESSFACTOR; }
#line 2841 "tshade.cpp"
    break;

  case 137: /* struct_semantic: tSEM_PSIZE  */
#line 561 "tshade.y"
    { (yyval.semtype) = SEMANTIC_PSIZE; }
#line 2847 "tshade.cpp"
    break;


#line 2851 "tshade.cpp"

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
  *++yylsp = yyloc;

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
  yytoken = yychar == TSHADE_EMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      {
        yypcontext_t yyctx
          = {yyssp, yytoken, &yylloc};
        if (yyreport_syntax_error (&yyctx, scanner, shadeAst) == 2)
          YYNOMEM;
      }
    }

  yyerror_range[1] = yylloc;
  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= TSHADE_EOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == TSHADE_EOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, &yylloc, scanner, shadeAst);
          yychar = TSHADE_EMPTY;
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

      yyerror_range[1] = *yylsp;
      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp, yylsp, scanner, shadeAst);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  yyerror_range[2] = yylloc;
  ++yylsp;
  YYLLOC_DEFAULT (*yylsp, yyerror_range, 2);

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
  yyerror (&yylloc, scanner, shadeAst, YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != TSHADE_EMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, &yylloc, scanner, shadeAst);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp, yylsp, scanner, shadeAst);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 565 "tshade.y"


void yyerror(YYLTYPE* yylloc, yyscan_t yyscanner, tShadeAst* shadeAst, char const *msg){
  yyerror(yylloc, yyscanner, msg);
}

void yyerror(YYLTYPE* yylloc,yyscan_t yyscanner, const char* msg) {
   Con::errorf("TorqueShader ERROR: %s Line: %d column: %d \n %s",
      msg,
      TShaderGetLineNo(yyscanner),
      TShaderGetColumnNo(yyscanner),
      TShaderGetText(yyscanner));
}

int
yyreport_syntax_error  (const yypcontext_t *ctx, yyscan_t scanner, tShadeAst* shadeAst)
{
   int ret = 0;
   String output;
   output += "syntax error: ";
   YYLTYPE *loc = yypcontext_location (ctx);
   yysymbol_kind_t nxt = yypcontext_token(ctx);
   if (nxt != YYSYMBOL_YYEMPTY)
      output += String::ToString("unexpected: %s", yysymbol_name(nxt));

   enum { TOKENMAX = 10 };
   yysymbol_kind_t expected[TOKENMAX];

   int exp = yypcontext_expected_tokens(ctx, expected, TOKENMAX);
   if (exp < 0)
      ret = exp;
   else
   {
      for (int i = 0; i < exp; ++i)
         output += String::ToString("%s %s", i == 0 ? ": expected" : "or", yysymbol_name(expected[i]));
   }

   yyerror(loc, scanner, output.c_str());

   return ret;
}
