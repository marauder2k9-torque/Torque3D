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
  void yyerror(const char* msg);
  void yyerror(yyscan_t yyscanner, const char* msg);
  void yyerror(yyscan_t yyscanner, tShadeAst* shadeAst, char const *msg);
  
  #define YY_DECL int yylex(union YYSTYPE *, yyscan_t)
  YY_DECL;

  extern int TShaderGetLineNo(yyscan_t);
  extern int TShaderGetColumnNo(yyscan_t);
  extern char* TShaderGetText(yyscan_t);

#line 100 "tshade.cpp"

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
  YYSYMBOL_tSWIZZLE = 71,                  /* tSWIZZLE  */
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
  YYSYMBOL_tFLOAT_TYPE = 82,               /* tFLOAT_TYPE  */
  YYSYMBOL_tINT_TYPE = 83,                 /* tINT_TYPE  */
  YYSYMBOL_tBOOL_TYPE = 84,                /* tBOOL_TYPE  */
  YYSYMBOL_tUINT_TYPE = 85,                /* tUINT_TYPE  */
  YYSYMBOL_tFVEC2_TYPE = 86,               /* tFVEC2_TYPE  */
  YYSYMBOL_tFVEC3_TYPE = 87,               /* tFVEC3_TYPE  */
  YYSYMBOL_tFVEC4_TYPE = 88,               /* tFVEC4_TYPE  */
  YYSYMBOL_tIVEC2_TYPE = 89,               /* tIVEC2_TYPE  */
  YYSYMBOL_tIVEC3_TYPE = 90,               /* tIVEC3_TYPE  */
  YYSYMBOL_tIVEC4_TYPE = 91,               /* tIVEC4_TYPE  */
  YYSYMBOL_tBVEC2_TYPE = 92,               /* tBVEC2_TYPE  */
  YYSYMBOL_tBVEC3_TYPE = 93,               /* tBVEC3_TYPE  */
  YYSYMBOL_tBVEC4_TYPE = 94,               /* tBVEC4_TYPE  */
  YYSYMBOL_tMAT4_TYPE = 95,                /* tMAT4_TYPE  */
  YYSYMBOL_tMAT43_TYPE = 96,               /* tMAT43_TYPE  */
  YYSYMBOL_tMAT34_TYPE = 97,               /* tMAT34_TYPE  */
  YYSYMBOL_tMAT3_TYPE = 98,                /* tMAT3_TYPE  */
  YYSYMBOL_tSEM_SVPOSITION = 99,           /* tSEM_SVPOSITION  */
  YYSYMBOL_tSEM_POSITION = 100,            /* tSEM_POSITION  */
  YYSYMBOL_tSEM_NORMAL = 101,              /* tSEM_NORMAL  */
  YYSYMBOL_tSEM_BINORMAL = 102,            /* tSEM_BINORMAL  */
  YYSYMBOL_tSEM_TANGENT = 103,             /* tSEM_TANGENT  */
  YYSYMBOL_tSEM_PSIZE = 104,               /* tSEM_PSIZE  */
  YYSYMBOL_tSEM_TESSFACTOR = 105,          /* tSEM_TESSFACTOR  */
  YYSYMBOL_tSEM_ISFRONTFACE = 106,         /* tSEM_ISFRONTFACE  */
  YYSYMBOL_tSEM_TEXCOORD = 107,            /* tSEM_TEXCOORD  */
  YYSYMBOL_tSEM_COLOR = 108,               /* tSEM_COLOR  */
  YYSYMBOL_tSEM_TARGET = 109,              /* tSEM_TARGET  */
  YYSYMBOL_tSEM_DEPTH = 110,               /* tSEM_DEPTH  */
  YYSYMBOL_111_ = 111,                     /* '['  */
  YYSYMBOL_112_ = 112,                     /* ']'  */
  YYSYMBOL_YYACCEPT = 113,                 /* $accept  */
  YYSYMBOL_program = 114,                  /* program  */
  YYSYMBOL_program_globals = 115,          /* program_globals  */
  YYSYMBOL_program_global_list = 116,      /* program_global_list  */
  YYSYMBOL_shader_stage = 117,             /* shader_stage  */
  YYSYMBOL_shader_body = 118,              /* shader_body  */
  YYSYMBOL_struct_decl = 119,              /* struct_decl  */
  YYSYMBOL_structbody_list = 120,          /* structbody_list  */
  YYSYMBOL_struct_member = 121,            /* struct_member  */
  YYSYMBOL_uniform_decl = 122,             /* uniform_decl  */
  YYSYMBOL_static_const_decl = 123,        /* static_const_decl  */
  YYSYMBOL_var_decl = 124,                 /* var_decl  */
  YYSYMBOL_param_modifier = 125,           /* param_modifier  */
  YYSYMBOL_function_def = 126,             /* function_def  */
  YYSYMBOL_function_param_list = 127,      /* function_param_list  */
  YYSYMBOL_function_param = 128,           /* function_param  */
  YYSYMBOL_expression_list = 129,          /* expression_list  */
  YYSYMBOL_expression = 130,               /* expression  */
  YYSYMBOL_statement_list = 131,           /* statement_list  */
  YYSYMBOL_statement = 132,                /* statement  */
  YYSYMBOL_if_statement = 133,             /* if_statement  */
  YYSYMBOL_while_statement = 134,          /* while_statement  */
  YYSYMBOL_switch_statement = 135,         /* switch_statement  */
  YYSYMBOL_case_statements = 136,          /* case_statements  */
  YYSYMBOL_case_rule = 137,                /* case_rule  */
  YYSYMBOL_continue_statement = 138,       /* continue_statement  */
  YYSYMBOL_break_statement = 139,          /* break_statement  */
  YYSYMBOL_return_statement = 140,         /* return_statement  */
  YYSYMBOL_discard_statement = 141,        /* discard_statement  */
  YYSYMBOL_var_type = 142,                 /* var_type  */
  YYSYMBOL_struct_semantic = 143           /* struct_semantic  */
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
         || (defined TSHADE_STYPE_IS_TRIVIAL && TSHADE_STYPE_IS_TRIVIAL)))

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
#define YYFINAL  4
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1391

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  113
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  31
/* YYNRULES -- Number of rules.  */
#define YYNRULES  129
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  248

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   343


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
       2,   111,     2,   112,    21,     2,     2,     2,     2,     2,
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
     107,   108,   109,   110
};

#if TSHADE_DEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   136,   136,   141,   146,   150,   155,   161,   167,   173,
     179,   188,   193,   199,   200,   205,   207,   209,   214,   222,
     227,   233,   241,   243,   245,   247,   249,   254,   256,   258,
     263,   265,   271,   272,   274,   279,   281,   286,   288,   293,
     295,   297,   299,   301,   303,   305,   307,   309,   311,   313,
     315,   317,   319,   321,   323,   325,   327,   329,   331,   333,
     335,   337,   339,   341,   343,   345,   355,   364,   373,   382,
     384,   390,   391,   396,   398,   400,   402,   404,   406,   408,
     410,   412,   414,   416,   418,   420,   426,   428,   433,   435,
     440,   446,   447,   452,   454,   459,   464,   469,   471,   476,
     481,   483,   485,   487,   489,   491,   493,   495,   497,   499,
     501,   503,   505,   507,   509,   511,   513,   515,   520,   522,
     524,   526,   528,   530,   532,   534,   536,   538,   540,   542
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
  "TYPE_IDENT", "tSWIZZLE", "tSTRUCT", "tUNIFORM", "tCBUFFER",
  "tSHADERDECLARE", "tVSSHADER", "tPSSHADER", "tGSSHADER", "tCSSHADER",
  "tDSSHADER", "tHSSHADER", "tFLOAT_TYPE", "tINT_TYPE", "tBOOL_TYPE",
  "tUINT_TYPE", "tFVEC2_TYPE", "tFVEC3_TYPE", "tFVEC4_TYPE", "tIVEC2_TYPE",
  "tIVEC3_TYPE", "tIVEC4_TYPE", "tBVEC2_TYPE", "tBVEC3_TYPE",
  "tBVEC4_TYPE", "tMAT4_TYPE", "tMAT43_TYPE", "tMAT34_TYPE", "tMAT3_TYPE",
  "tSEM_SVPOSITION", "tSEM_POSITION", "tSEM_NORMAL", "tSEM_BINORMAL",
  "tSEM_TANGENT", "tSEM_PSIZE", "tSEM_TESSFACTOR", "tSEM_ISFRONTFACE",
  "tSEM_TEXCOORD", "tSEM_COLOR", "tSEM_TARGET", "tSEM_DEPTH", "'['", "']'",
  "$accept", "program", "program_globals", "program_global_list",
  "shader_stage", "shader_body", "struct_decl", "structbody_list",
  "struct_member", "uniform_decl", "static_const_decl", "var_decl",
  "param_modifier", "function_def", "function_param_list",
  "function_param", "expression_list", "expression", "statement_list",
  "statement", "if_statement", "while_statement", "switch_statement",
  "case_statements", "case_rule", "continue_statement", "break_statement",
  "return_statement", "discard_statement", "var_type", "struct_semantic", YY_NULLPTR
  };
  return yy_sname[yysymbol];
}
#endif

#define YYPACT_NINF (-127)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     -52,   -38,    46,    41,  -127,  -127,    44,   -27,  -127,    12,
      52,    64,    65,    74,  -127,  -127,    80,  -127,  -127,  -127,
    -127,  -127,    88,   678,    89,    92,    95,   886,  -127,   868,
     868,   868,    86,   104,   100,   109,   110,   115,   121,  -127,
     966,   773,   995,   868,   868,  -127,  -127,    -4,    73,   995,
    -127,  -127,  -127,  -127,  -127,  -127,  -127,  -127,  -127,  -127,
    -127,  -127,  -127,  -127,  -127,  -127,  -127,  -127,  -127,  -127,
    -127,  -127,  1091,  -127,  -127,  -127,  -127,  -127,  -127,  -127,
    -127,    -7,  -127,  -127,  -127,  -127,  -127,  -127,    76,   101,
     133,  1107,   101,   868,   868,  -127,  -127,  -127,   868,  -127,
     995,  -127,    81,  -127,  1123,  -127,   101,   101,    93,   868,
    -127,   130,  -127,   868,   868,   868,   868,   868,   868,   868,
     868,  -127,   868,   868,   868,   868,   868,  -127,  -127,   868,
     868,   868,   868,   868,   868,     0,    11,  -127,  1179,  1192,
     203,  1205,  -127,     2,  -127,  -127,    43,   841,  -127,   490,
     490,   101,   101,   395,   395,   151,   101,   129,   129,   207,
     301,   395,    18,    18,    18,    18,    18,    47,   868,   903,
    -127,   868,   -67,  -127,   148,   157,   149,   159,  -127,   868,
    -127,  1261,  -127,  -127,  -127,   931,    53,  -127,   111,   117,
    -127,  -127,  -127,  -127,  -127,  -127,  -127,  -127,  -127,  -127,
    -127,  -127,   172,  -127,  -127,   177,  -127,   841,  -127,   124,
      60,   903,  -127,     3,  -127,    27,   298,   868,   393,  -127,
    -127,  -127,  -127,   175,  -127,  -127,   868,   179,  -127,  -127,
    1277,   163,   488,   868,  1333,  -127,  -127,   180,  -127,    28,
    -127,   678,  -127,   182,   678,   583,  -127,  -127
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     0,     0,     0,     1,     6,     0,     3,     2,     0,
       0,     0,     0,     0,     5,     4,     0,    71,    71,    71,
      71,    13,     0,    11,     0,     0,     0,     0,     7,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   117,
       0,     0,     0,     0,     0,    69,    70,    67,     0,     0,
     113,   114,   116,   115,   104,   105,   106,   107,   108,   109,
     110,   111,   112,   103,   101,   100,   102,    85,    74,    75,
      73,    81,     0,    72,    76,    77,    83,    78,    79,    80,
      84,     0,     8,     9,    10,    12,    14,    17,     0,    58,
       0,     0,    57,     0,     0,    71,    96,    95,     0,    99,
       0,    19,     0,    97,     0,    21,    59,    60,     0,     0,
      66,     0,    18,     0,     0,     0,     0,     0,     0,     0,
       0,    82,     0,     0,     0,     0,     0,    61,    62,     0,
       0,     0,     0,     0,     0,     0,     0,    63,     0,     0,
       0,     0,    20,     0,    98,    68,     0,    37,    26,    39,
      42,    44,    46,    52,    53,    41,    48,    50,    51,    55,
      56,    54,    40,    43,    45,    47,    49,     0,     0,    32,
      22,     0,     0,    16,     0,     0,     0,     0,    65,     0,
      64,     0,    27,    28,    29,     0,     0,    33,     0,     0,
     118,   119,   120,   121,   122,   129,   128,   127,   123,   124,
     125,   126,     0,    91,    71,     0,    71,    38,    23,     0,
       0,     0,    36,     0,    15,     0,     0,     0,     0,    35,
      31,    71,    34,     0,    24,    90,     0,     0,    92,    88,
       0,    87,     0,     0,     0,    71,    89,     0,    30,     0,
      71,    94,    71,     0,    93,     0,    25,    86
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -127,  -127,  -127,  -127,  -127,    37,   194,  -127,  -127,  -127,
    -127,   -18,  -127,   176,  -127,    -9,  -126,   -28,   -90,  -127,
    -127,  -127,  -127,  -127,  -127,  -127,  -127,  -127,  -127,   -23,
    -127
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,     2,     6,     7,    14,    22,    67,    27,    86,    68,
      69,    70,   185,    71,   186,   187,   146,    72,    23,    73,
      74,    75,    76,   215,   228,    77,    78,    79,    80,    90,
     202
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      81,    89,    91,    92,    88,   140,   108,   134,   167,   168,
     109,   168,   223,   104,   169,   106,   107,   102,   170,   102,
     170,   224,   101,     1,   105,   169,   102,   119,   172,   173,
       3,   112,   190,   191,   192,   193,   194,   195,   196,   197,
     198,   199,   200,   201,   179,     9,     4,   225,   243,    10,
      11,    12,    13,   226,   227,    24,    25,    26,   178,   179,
       5,   135,   180,   179,     8,   138,   139,   110,   210,   211,
     141,    17,   129,   130,   131,   132,   133,   102,   220,   221,
      16,   147,   142,    18,    19,   149,   150,   151,   152,   153,
     154,   155,   156,    20,   157,   158,   159,   160,   161,    21,
      93,   162,   163,   164,   165,   166,   147,   239,    28,    82,
     119,   171,    83,   171,   216,    84,   218,    81,    94,    95,
     113,   114,   115,   116,   117,   118,   119,    96,    97,    98,
     120,   232,   113,   114,   115,   116,   117,   118,   119,    99,
     181,   111,   120,   189,   136,   241,   188,   134,   148,   143,
     244,   207,   245,   127,   128,   129,   130,   131,   132,   133,
     119,   145,   209,   122,   123,   124,   125,   203,   126,   127,
     128,   129,   130,   131,   132,   133,   204,   205,   206,   212,
     126,   127,   128,   129,   130,   131,   132,   133,   188,   230,
     214,   217,   219,    81,   233,    81,   235,   237,   234,   242,
     246,    15,   222,    87,     0,   147,     0,    29,     0,    81,
     113,   114,   115,   116,   117,   118,   119,    30,    81,     0,
     120,    81,    81,   176,     0,     0,    31,     0,    32,   213,
       0,    33,    34,     0,    35,    36,    37,     0,    38,    39,
      40,     0,     0,     0,     0,     0,     0,    41,    42,     0,
       0,     0,     0,   122,   123,    43,    44,     0,   126,   127,
     128,   129,   130,   131,   132,   133,     0,     0,     0,    45,
      46,    47,     0,    48,     0,     9,    49,     0,     0,     0,
       0,     0,     0,     0,     0,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    29,     0,   113,   114,   115,   116,   117,   118,
     119,     0,    30,     0,   120,     0,     0,     0,   229,     0,
       0,    31,     0,    32,     0,     0,    33,    34,     0,    35,
      36,    37,     0,    38,    39,    40,     0,     0,     0,     0,
       0,     0,    41,    42,     0,     0,     0,   122,   123,   124,
      43,    44,   126,   127,   128,   129,   130,   131,   132,   133,
       0,     0,     0,     0,    45,    46,    47,     0,    48,     0,
       9,    49,     0,     0,     0,     0,     0,     0,     0,     0,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    29,   113,   114,
     115,   116,     0,     0,   119,     0,     0,    30,   120,     0,
       0,     0,     0,   231,     0,     0,    31,     0,    32,     0,
       0,    33,    34,     0,    35,    36,    37,     0,    38,    39,
      40,     0,     0,     0,     0,     0,     0,    41,    42,     0,
       0,     0,     0,     0,     0,    43,    44,   127,   128,   129,
     130,   131,   132,   133,     0,     0,     0,     0,     0,    45,
      46,    47,     0,    48,     0,     9,    49,     0,     0,     0,
       0,     0,     0,     0,     0,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    29,     0,     0,   115,   116,     0,     0,   119,
       0,     0,    30,   120,     0,     0,     0,     0,   238,     0,
       0,    31,     0,    32,     0,     0,    33,    34,     0,    35,
      36,    37,     0,    38,    39,    40,     0,     0,     0,     0,
       0,     0,    41,    42,     0,     0,     0,     0,     0,     0,
      43,    44,   127,   128,   129,   130,   131,   132,   133,     0,
       0,     0,     0,     0,    45,    46,    47,     0,    48,     0,
       9,    49,     0,     0,     0,     0,     0,     0,     0,     0,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    29,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    30,     0,     0,
       0,     0,     0,   247,     0,     0,    31,     0,    32,     0,
       0,    33,    34,     0,    35,    36,    37,     0,    38,    39,
      40,     0,     0,     0,     0,     0,     0,    41,    42,     0,
       0,     0,     0,     0,     0,    43,    44,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    45,
      46,    47,     0,    48,     0,     9,    49,     0,     0,     0,
       0,     0,     0,     0,     0,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    29,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    30,     0,     0,     0,     0,     0,     0,     0,
       0,    31,     0,    32,     0,     0,    33,    34,     0,    35,
      36,    37,     0,    38,    39,    40,     0,     0,     0,     0,
       0,     0,    41,    42,     0,     0,     0,     0,     0,     0,
      43,    44,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    45,    46,    47,     0,    48,     0,
       9,    49,     0,     0,     0,     0,     0,     0,     0,     0,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    29,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    30,     0,     0,
       0,   103,     0,     0,     0,     0,    31,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    39,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    43,    44,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    45,
      46,    47,     0,     0,   113,   114,   115,   116,   117,   118,
     119,     0,     0,     0,   120,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    29,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    30,     0,     0,     0,     0,   122,   123,   124,
     125,    31,   126,   127,   128,   129,   130,   131,   132,   133,
       0,     0,     0,     0,    39,     0,    85,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      43,    44,    39,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    45,    46,    47,     0,     0,    39,
       0,   182,   183,   184,     0,     0,     0,     0,     0,     0,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    39,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    39,     0,     0,     0,     0,     0,     0,     0,
       0,   100,     0,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
       0,    39,     0,     0,     0,     0,    48,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    48,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,   113,   114,   115,   116,   117,   118,
     119,     0,     0,     0,   120,     0,     0,     0,     0,   121,
     113,   114,   115,   116,   117,   118,   119,     0,     0,     0,
     120,     0,   137,     0,     0,     0,   113,   114,   115,   116,
     117,   118,   119,     0,     0,     0,   120,   122,   123,   124,
     125,   144,   126,   127,   128,   129,   130,   131,   132,   133,
       0,     0,     0,   122,   123,   124,   125,     0,   126,   127,
     128,   129,   130,   131,   132,   133,     0,     0,     0,   122,
     123,   124,   125,     0,   126,   127,   128,   129,   130,   131,
     132,   133,   113,   114,   115,   116,   117,   118,   119,     0,
       0,     0,   120,     0,   174,   113,   114,   115,   116,   117,
     118,   119,     0,     0,     0,   120,     0,   175,   113,   114,
     115,   116,   117,   118,   119,     0,     0,     0,   120,     0,
     177,     0,     0,     0,     0,   122,   123,   124,   125,     0,
     126,   127,   128,   129,   130,   131,   132,   133,   122,   123,
     124,   125,     0,   126,   127,   128,   129,   130,   131,   132,
     133,   122,   123,   124,   125,     0,   126,   127,   128,   129,
     130,   131,   132,   133,   113,   114,   115,   116,   117,   118,
     119,     0,     0,     0,   120,     0,     0,     0,     0,   208,
     113,   114,   115,   116,   117,   118,   119,     0,     0,     0,
     120,     0,   236,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   122,   123,   124,
     125,     0,   126,   127,   128,   129,   130,   131,   132,   133,
       0,     0,     0,   122,   123,   124,   125,     0,   126,   127,
     128,   129,   130,   131,   132,   133,   113,   114,   115,   116,
     117,   118,   119,     0,     0,     0,   120,     0,     0,     0,
     240,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   122,
     123,   124,   125,     0,   126,   127,   128,   129,   130,   131,
     132,   133
};

static const yytype_int16 yycheck[] =
{
      23,    29,    30,    31,    27,    95,    10,    14,   134,     9,
      14,     9,     9,    41,    14,    43,    44,    40,    18,    42,
      18,    18,    40,    75,    42,    14,    49,     9,    17,    18,
      68,    49,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,    16,    72,     0,    20,    20,    76,
      77,    78,    79,    26,    27,    18,    19,    20,    15,    16,
      19,    68,    15,    16,    20,    93,    94,    71,    15,    16,
      98,    19,    54,    55,    56,    57,    58,   100,    18,    19,
      68,   109,   100,    19,    19,   113,   114,   115,   116,   117,
     118,   119,   120,    19,   122,   123,   124,   125,   126,    19,
      14,   129,   130,   131,   132,   133,   134,   233,    20,    20,
       9,   111,    20,   111,   204,    20,   206,   140,    14,    19,
       3,     4,     5,     6,     7,     8,     9,    18,    18,    14,
      13,   221,     3,     4,     5,     6,     7,     8,     9,    18,
     168,    68,    13,   171,    68,   235,   169,    14,    18,    68,
     240,   179,   242,    52,    53,    54,    55,    56,    57,    58,
       9,    68,   185,    46,    47,    48,    49,    19,    51,    52,
      53,    54,    55,    56,    57,    58,    19,    28,    19,    68,
      51,    52,    53,    54,    55,    56,    57,    58,   211,   217,
      18,    14,    68,   216,    19,   218,    17,    34,   226,    19,
      18,     7,   211,    27,    -1,   233,    -1,     4,    -1,   232,
       3,     4,     5,     6,     7,     8,     9,    14,   241,    -1,
      13,   244,   245,    20,    -1,    -1,    23,    -1,    25,   112,
      -1,    28,    29,    -1,    31,    32,    33,    -1,    35,    36,
      37,    -1,    -1,    -1,    -1,    -1,    -1,    44,    45,    -1,
      -1,    -1,    -1,    46,    47,    52,    53,    -1,    51,    52,
      53,    54,    55,    56,    57,    58,    -1,    -1,    -1,    66,
      67,    68,    -1,    70,    -1,    72,    73,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,     4,    -1,     3,     4,     5,     6,     7,     8,
       9,    -1,    14,    -1,    13,    -1,    -1,    -1,    20,    -1,
      -1,    23,    -1,    25,    -1,    -1,    28,    29,    -1,    31,
      32,    33,    -1,    35,    36,    37,    -1,    -1,    -1,    -1,
      -1,    -1,    44,    45,    -1,    -1,    -1,    46,    47,    48,
      52,    53,    51,    52,    53,    54,    55,    56,    57,    58,
      -1,    -1,    -1,    -1,    66,    67,    68,    -1,    70,    -1,
      72,    73,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,     4,     3,     4,
       5,     6,    -1,    -1,     9,    -1,    -1,    14,    13,    -1,
      -1,    -1,    -1,    20,    -1,    -1,    23,    -1,    25,    -1,
      -1,    28,    29,    -1,    31,    32,    33,    -1,    35,    36,
      37,    -1,    -1,    -1,    -1,    -1,    -1,    44,    45,    -1,
      -1,    -1,    -1,    -1,    -1,    52,    53,    52,    53,    54,
      55,    56,    57,    58,    -1,    -1,    -1,    -1,    -1,    66,
      67,    68,    -1,    70,    -1,    72,    73,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,     4,    -1,    -1,     5,     6,    -1,    -1,     9,
      -1,    -1,    14,    13,    -1,    -1,    -1,    -1,    20,    -1,
      -1,    23,    -1,    25,    -1,    -1,    28,    29,    -1,    31,
      32,    33,    -1,    35,    36,    37,    -1,    -1,    -1,    -1,
      -1,    -1,    44,    45,    -1,    -1,    -1,    -1,    -1,    -1,
      52,    53,    52,    53,    54,    55,    56,    57,    58,    -1,
      -1,    -1,    -1,    -1,    66,    67,    68,    -1,    70,    -1,
      72,    73,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,     4,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    14,    -1,    -1,
      -1,    -1,    -1,    20,    -1,    -1,    23,    -1,    25,    -1,
      -1,    28,    29,    -1,    31,    32,    33,    -1,    35,    36,
      37,    -1,    -1,    -1,    -1,    -1,    -1,    44,    45,    -1,
      -1,    -1,    -1,    -1,    -1,    52,    53,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    66,
      67,    68,    -1,    70,    -1,    72,    73,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,     4,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    14,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    23,    -1,    25,    -1,    -1,    28,    29,    -1,    31,
      32,    33,    -1,    35,    36,    37,    -1,    -1,    -1,    -1,
      -1,    -1,    44,    45,    -1,    -1,    -1,    -1,    -1,    -1,
      52,    53,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    66,    67,    68,    -1,    70,    -1,
      72,    73,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,     4,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    14,    -1,    -1,
      -1,    18,    -1,    -1,    -1,    -1,    23,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    36,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    52,    53,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    66,
      67,    68,    -1,    -1,     3,     4,     5,     6,     7,     8,
       9,    -1,    -1,    -1,    13,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,     4,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    14,    -1,    -1,    -1,    -1,    46,    47,    48,
      49,    23,    51,    52,    53,    54,    55,    56,    57,    58,
      -1,    -1,    -1,    -1,    36,    -1,    20,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      52,    53,    36,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    66,    67,    68,    -1,    -1,    36,
      -1,    38,    39,    40,    -1,    -1,    -1,    -1,    -1,    -1,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    36,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    36,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    45,    -1,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      -1,    36,    -1,    -1,    -1,    -1,    70,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    70,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,     3,     4,     5,     6,     7,     8,
       9,    -1,    -1,    -1,    13,    -1,    -1,    -1,    -1,    18,
       3,     4,     5,     6,     7,     8,     9,    -1,    -1,    -1,
      13,    -1,    15,    -1,    -1,    -1,     3,     4,     5,     6,
       7,     8,     9,    -1,    -1,    -1,    13,    46,    47,    48,
      49,    18,    51,    52,    53,    54,    55,    56,    57,    58,
      -1,    -1,    -1,    46,    47,    48,    49,    -1,    51,    52,
      53,    54,    55,    56,    57,    58,    -1,    -1,    -1,    46,
      47,    48,    49,    -1,    51,    52,    53,    54,    55,    56,
      57,    58,     3,     4,     5,     6,     7,     8,     9,    -1,
      -1,    -1,    13,    -1,    15,     3,     4,     5,     6,     7,
       8,     9,    -1,    -1,    -1,    13,    -1,    15,     3,     4,
       5,     6,     7,     8,     9,    -1,    -1,    -1,    13,    -1,
      15,    -1,    -1,    -1,    -1,    46,    47,    48,    49,    -1,
      51,    52,    53,    54,    55,    56,    57,    58,    46,    47,
      48,    49,    -1,    51,    52,    53,    54,    55,    56,    57,
      58,    46,    47,    48,    49,    -1,    51,    52,    53,    54,
      55,    56,    57,    58,     3,     4,     5,     6,     7,     8,
       9,    -1,    -1,    -1,    13,    -1,    -1,    -1,    -1,    18,
       3,     4,     5,     6,     7,     8,     9,    -1,    -1,    -1,
      13,    -1,    15,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    46,    47,    48,
      49,    -1,    51,    52,    53,    54,    55,    56,    57,    58,
      -1,    -1,    -1,    46,    47,    48,    49,    -1,    51,    52,
      53,    54,    55,    56,    57,    58,     3,     4,     5,     6,
       7,     8,     9,    -1,    -1,    -1,    13,    -1,    -1,    -1,
      17,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    46,
      47,    48,    49,    -1,    51,    52,    53,    54,    55,    56,
      57,    58
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    75,   114,    68,     0,    19,   115,   116,    20,    72,
      76,    77,    78,    79,   117,   119,    68,    19,    19,    19,
      19,    19,   118,   131,   118,   118,   118,   120,    20,     4,
      14,    23,    25,    28,    29,    31,    32,    33,    35,    36,
      37,    44,    45,    52,    53,    66,    67,    68,    70,    73,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,   119,   122,   123,
     124,   126,   130,   132,   133,   134,   135,   138,   139,   140,
     141,   142,    20,    20,    20,    20,   121,   126,   142,   130,
     142,   130,   130,    14,    14,    19,    18,    18,    14,    18,
      45,   124,   142,    18,   130,   124,   130,   130,    10,    14,
      71,    68,   124,     3,     4,     5,     6,     7,     8,     9,
      13,    18,    46,    47,    48,    49,    51,    52,    53,    54,
      55,    56,    57,    58,    14,    68,    68,    15,   130,   130,
     131,   130,   124,    68,    18,    68,   129,   130,    18,   130,
     130,   130,   130,   130,   130,   130,   130,   130,   130,   130,
     130,   130,   130,   130,   130,   130,   130,   129,     9,    14,
      18,   111,    17,    18,    15,    15,    20,    15,    15,    16,
      15,   130,    38,    39,    40,   125,   127,   128,   142,   130,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   143,    19,    19,    28,    19,   130,    18,   142,
      15,    16,    68,   112,    18,   136,   131,    14,   131,    68,
      18,    19,   128,     9,    18,    20,    26,    27,   137,    20,
     130,    20,   131,    19,   130,    17,    15,    34,    20,   129,
      17,   131,    19,    20,   131,   131,    18,    20
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_uint8 yyr1[] =
{
       0,   113,   114,   115,   116,   116,   116,   117,   117,   117,
     117,   118,   119,   120,   120,   121,   121,   121,   122,   123,
     123,   123,   124,   124,   124,   124,   124,   125,   125,   125,
     126,   126,   127,   127,   127,   128,   128,   129,   129,   130,
     130,   130,   130,   130,   130,   130,   130,   130,   130,   130,
     130,   130,   130,   130,   130,   130,   130,   130,   130,   130,
     130,   130,   130,   130,   130,   130,   130,   130,   130,   130,
     130,   131,   131,   132,   132,   132,   132,   132,   132,   132,
     132,   132,   132,   132,   132,   132,   133,   133,   134,   134,
     135,   136,   136,   137,   137,   138,   139,   140,   140,   141,
     142,   142,   142,   142,   142,   142,   142,   142,   142,   142,
     142,   142,   142,   142,   142,   142,   142,   142,   143,   143,
     143,   143,   143,   143,   143,   143,   143,   143,   143,   143
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     5,     1,     2,     2,     0,     4,     4,     4,
       4,     1,     5,     0,     2,     5,     3,     1,     2,     2,
       3,     2,     3,     5,     6,    10,     3,     1,     1,     1,
       8,     6,     0,     1,     3,     3,     2,     1,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     2,     2,     2,
       2,     2,     2,     3,     4,     4,     2,     1,     3,     1,
       1,     0,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     2,     1,     1,     1,    11,     7,     7,     8,
       7,     0,     2,     4,     3,     2,     2,     2,     3,     2,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1
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
        yyerror (scanner, shadeAst, YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use TSHADE_error or TSHADE_UNDEF. */
#define YYERRCODE TSHADE_UNDEF


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




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value, scanner, shadeAst); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, yyscan_t scanner, tShadeAst* shadeAst)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
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
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, yyscan_t scanner, tShadeAst* shadeAst)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep, scanner, shadeAst);
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
                       &yyvsp[(yyi + 1) - (yynrhs)], scanner, shadeAst);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule, scanner, shadeAst); \
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



/* User defined function to report a syntax error.  */
static int
yyreport_syntax_error (const yypcontext_t *yyctx, yyscan_t scanner, tShadeAst* shadeAst);

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep, yyscan_t scanner, tShadeAst* shadeAst)
{
  YY_USE (yyvaluep);
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

  yychar = TSHADE_EMPTY; /* Cause a token to be read.  */

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
  if (yychar == TSHADE_EMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex (&yylval, scanner);
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


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* program: tSHADERDECLARE VAR_IDENT '{' program_globals '}'  */
#line 137 "tshade.y"
    {(yyval.node) = nullptr; shadeAst->shaderName = (yyvsp[-3].strVal); }
#line 1712 "tshade.cpp"
    break;

  case 3: /* program_globals: program_global_list  */
#line 142 "tshade.y"
    {(yyval.node) = nullptr; }
#line 1718 "tshade.cpp"
    break;

  case 4: /* program_global_list: program_global_list struct_decl  */
#line 147 "tshade.y"
    {
      shadeAst->addDataStruct((yyvsp[0].structNode));
    }
#line 1726 "tshade.cpp"
    break;

  case 5: /* program_global_list: program_global_list shader_stage  */
#line 151 "tshade.y"
    {
      (yyval.node) = nullptr;
    }
#line 1734 "tshade.cpp"
    break;

  case 6: /* program_global_list: %empty  */
#line 155 "tshade.y"
    {
      (yyval.node) = nullptr;
    }
#line 1742 "tshade.cpp"
    break;

  case 7: /* shader_stage: tVSSHADER '{' shader_body '}'  */
#line 162 "tshade.y"
    { 
      shadeAst->currentStage = ShaderStageType::tSTAGE_VERTEX;
      shadeAst->mVertStage = new tStageNode(ShaderStageType::tSTAGE_VERTEX, (yyvsp[-1].node));
      shadeAst->currentStage = ShaderStageType::tSTAGE_GLOBAL;
    }
#line 1752 "tshade.cpp"
    break;

  case 8: /* shader_stage: tPSSHADER '{' shader_body '}'  */
#line 168 "tshade.y"
    {
      shadeAst->currentStage = ShaderStageType::tSTAGE_PIXEL;
      shadeAst->mPixStage = new tStageNode(ShaderStageType::tSTAGE_PIXEL, (yyvsp[-1].node));
      shadeAst->currentStage = ShaderStageType::tSTAGE_GLOBAL;
    }
#line 1762 "tshade.cpp"
    break;

  case 9: /* shader_stage: tGSSHADER '{' shader_body '}'  */
#line 174 "tshade.y"
    {
      shadeAst->currentStage = ShaderStageType::tSTAGE_GEOMETRY;
      shadeAst->mPixStage = new tStageNode(ShaderStageType::tSTAGE_GEOMETRY, (yyvsp[-1].node));
      shadeAst->currentStage = ShaderStageType::tSTAGE_GLOBAL;
    }
#line 1772 "tshade.cpp"
    break;

  case 10: /* shader_stage: tCSSHADER '{' shader_body '}'  */
#line 180 "tshade.y"
    {
      shadeAst->currentStage = ShaderStageType::tSTAGE_COMPUTE;
      shadeAst->mPixStage = new tStageNode(ShaderStageType::tSTAGE_COMPUTE, (yyvsp[-1].node));
      shadeAst->currentStage = ShaderStageType::tSTAGE_GLOBAL;
    }
#line 1782 "tshade.cpp"
    break;

  case 11: /* shader_body: statement_list  */
#line 189 "tshade.y"
    {(yyval.node) = (yyvsp[0].stmt_list_node);}
#line 1788 "tshade.cpp"
    break;

  case 12: /* struct_decl: tSTRUCT VAR_IDENT '{' structbody_list '}'  */
#line 194 "tshade.y"
    { (yyval.structNode) = new tStructNode((yyvsp[-3].strVal), (yyvsp[-1].stmt_list_node)); }
#line 1794 "tshade.cpp"
    break;

  case 13: /* structbody_list: %empty  */
#line 199 "tshade.y"
    { (yyval.stmt_list_node) = new tStatementListNode(); }
#line 1800 "tshade.cpp"
    break;

  case 14: /* structbody_list: structbody_list struct_member  */
#line 201 "tshade.y"
    { (yyvsp[-1].stmt_list_node)->addStatement((yyvsp[0].node)); (yyval.stmt_list_node) = (yyvsp[-1].stmt_list_node); }
#line 1806 "tshade.cpp"
    break;

  case 15: /* struct_member: var_type VAR_IDENT ':' struct_semantic ';'  */
#line 206 "tshade.y"
    {(yyval.node) = new tStructMemberNode((yyvsp[-3].strVal), (yyvsp[-4].varType), (yyvsp[-1].semtype), yylval.intVal); }
#line 1812 "tshade.cpp"
    break;

  case 16: /* struct_member: var_type VAR_IDENT ';'  */
#line 208 "tshade.y"
    {(yyval.node) = new tStructMemberNode((yyvsp[-1].strVal), (yyvsp[-2].varType)); }
#line 1818 "tshade.cpp"
    break;

  case 17: /* struct_member: function_def  */
#line 210 "tshade.y"
    {(yyval.node) = (yyvsp[0].funcNode);}
#line 1824 "tshade.cpp"
    break;

  case 18: /* uniform_decl: tUNIFORM var_decl  */
#line 215 "tshade.y"
  {
    (yyvsp[0].declNode)->isUniform = true;
    (yyval.declNode) = (yyvsp[0].declNode);
  }
#line 1833 "tshade.cpp"
    break;

  case 19: /* static_const_decl: rwSTATIC var_decl  */
#line 223 "tshade.y"
  {
    (yyvsp[0].declNode)->isStatic = true;
    (yyval.declNode) = (yyvsp[0].declNode);
  }
#line 1842 "tshade.cpp"
    break;

  case 20: /* static_const_decl: rwSTATIC rwCONST var_decl  */
#line 228 "tshade.y"
  {
    (yyvsp[0].declNode)->isStatic = true;
    (yyvsp[0].declNode)->isConst = true;
    (yyval.declNode) = (yyvsp[0].declNode);
  }
#line 1852 "tshade.cpp"
    break;

  case 21: /* static_const_decl: rwCONST var_decl  */
#line 234 "tshade.y"
  {
    (yyvsp[0].declNode)->isConst = true;
    (yyval.declNode) = (yyvsp[0].declNode);
  }
#line 1861 "tshade.cpp"
    break;

  case 22: /* var_decl: var_type VAR_IDENT ';'  */
#line 242 "tshade.y"
    {(yyval.declNode) = new tVarDeclNode((yyvsp[-1].strVal), (yyvsp[-2].varType)); shadeAst->addVarDecl((yyval.declNode)); }
#line 1867 "tshade.cpp"
    break;

  case 23: /* var_decl: var_type VAR_IDENT '=' expression ';'  */
#line 244 "tshade.y"
    {(yyval.declNode) = new tVarDeclNode((yyvsp[-3].strVal), (yyvsp[-4].varType), (yyvsp[-1].node)); shadeAst->addVarDecl((yyval.declNode)); }
#line 1873 "tshade.cpp"
    break;

  case 24: /* var_decl: var_type VAR_IDENT '[' expression ']' ';'  */
#line 246 "tshade.y"
    {(yyval.declNode) = new tVarDeclNode((yyvsp[-4].strVal), (yyvsp[-5].varType), nullptr, (yyvsp[-2].node)); shadeAst->addVarDecl((yyval.declNode)); }
#line 1879 "tshade.cpp"
    break;

  case 25: /* var_decl: var_type VAR_IDENT '[' expression ']' '=' '{' expression_list '}' ';'  */
#line 248 "tshade.y"
    {(yyval.declNode) = new tVarDeclNode((yyvsp[-8].strVal), (yyvsp[-9].varType), (yyvsp[-2].exprListnode), (yyvsp[-6].node)); shadeAst->addVarDecl((yyval.declNode)); }
#line 1885 "tshade.cpp"
    break;

  case 26: /* var_decl: TYPE_IDENT VAR_IDENT ';'  */
#line 250 "tshade.y"
    {(yyval.declNode) = new tVarDeclNode((yyvsp[-1].strVal), ShaderVarType::tTYPE_STRUCT, nullptr, nullptr, true); (yyval.declNode)->structName = (yyvsp[-2].strVal); shadeAst->addVarDecl((yyval.declNode)); }
#line 1891 "tshade.cpp"
    break;

  case 27: /* param_modifier: rwIN  */
#line 255 "tshade.y"
    { (yyval.modifier) = ParamModifier::PARAM_MOD_IN; }
#line 1897 "tshade.cpp"
    break;

  case 28: /* param_modifier: rwOUT  */
#line 257 "tshade.y"
    { (yyval.modifier) = ParamModifier::PARAM_MOD_OUT; }
#line 1903 "tshade.cpp"
    break;

  case 29: /* param_modifier: rwINOUT  */
#line 259 "tshade.y"
    { (yyval.modifier) = ParamModifier::PARAM_MOD_INOUT; }
#line 1909 "tshade.cpp"
    break;

  case 30: /* function_def: var_type VAR_IDENT '(' function_param_list ')' '{' statement_list '}'  */
#line 264 "tshade.y"
    { (yyval.funcNode) = new tFunctionDefNode((yyvsp[-6].strVal), (yyvsp[-7].varType), (yyvsp[-4].funcList), (yyvsp[-1].stmt_list_node)); shadeAst->addfunction((yyval.funcNode)); }
#line 1915 "tshade.cpp"
    break;

  case 31: /* function_def: var_type VAR_IDENT '(' function_param_list ')' ';'  */
#line 266 "tshade.y"
    { (yyval.funcNode) = new tFunctionDeclNode((yyvsp[-4].strVal), (yyvsp[-5].varType), (yyvsp[-2].funcList)); shadeAst->addfunction((yyval.funcNode)); }
#line 1921 "tshade.cpp"
    break;

  case 32: /* function_param_list: %empty  */
#line 271 "tshade.y"
    { (yyval.funcList) = new tFunctionParamListNode(); }
#line 1927 "tshade.cpp"
    break;

  case 33: /* function_param_list: function_param  */
#line 273 "tshade.y"
    { (yyval.funcList) = new tFunctionParamListNode(); (yyval.funcList)->addParam((yyvsp[0].funcParam)); }
#line 1933 "tshade.cpp"
    break;

  case 34: /* function_param_list: function_param_list ',' function_param  */
#line 275 "tshade.y"
    {(yyvsp[-2].funcList)->addParam((yyvsp[0].funcParam)); (yyval.funcList) = (yyvsp[-2].funcList); }
#line 1939 "tshade.cpp"
    break;

  case 35: /* function_param: param_modifier var_type VAR_IDENT  */
#line 280 "tshade.y"
    { (yyval.funcParam) = new tFunctionParamNode((yyvsp[0].strVal), (yyvsp[-1].varType), (yyvsp[-2].modifier)); }
#line 1945 "tshade.cpp"
    break;

  case 36: /* function_param: var_type VAR_IDENT  */
#line 282 "tshade.y"
    { (yyval.funcParam) = new tFunctionParamNode((yyvsp[0].strVal), (yyvsp[-1].varType), ParamModifier::PARAM_MOD_NONE); }
#line 1951 "tshade.cpp"
    break;

  case 37: /* expression_list: expression  */
#line 287 "tshade.y"
    { (yyval.exprListnode) = new tExpressionListNode(); (yyval.exprListnode)->addExpression((yyvsp[0].node)); }
#line 1957 "tshade.cpp"
    break;

  case 38: /* expression_list: expression_list ',' expression  */
#line 289 "tshade.y"
    { (yyval.exprListnode)->addExpression((yyvsp[0].node)); (yyval.exprListnode) = (yyvsp[-2].exprListnode); }
#line 1963 "tshade.cpp"
    break;

  case 39: /* expression: expression '+' expression  */
#line 294 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("+", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1969 "tshade.cpp"
    break;

  case 40: /* expression: expression OP_PLUS_ASS expression  */
#line 296 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("+=", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1975 "tshade.cpp"
    break;

  case 41: /* expression: expression '=' expression  */
#line 298 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("=", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1981 "tshade.cpp"
    break;

  case 42: /* expression: expression '-' expression  */
#line 300 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("-", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1987 "tshade.cpp"
    break;

  case 43: /* expression: expression OP_MINUS_ASS expression  */
#line 302 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("-=", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1993 "tshade.cpp"
    break;

  case 44: /* expression: expression '*' expression  */
#line 304 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("*", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1999 "tshade.cpp"
    break;

  case 45: /* expression: expression OP_MUL_ASS expression  */
#line 306 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("*=", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 2005 "tshade.cpp"
    break;

  case 46: /* expression: expression '/' expression  */
#line 308 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("/", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 2011 "tshade.cpp"
    break;

  case 47: /* expression: expression OP_DIV_ASS expression  */
#line 310 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("/=", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 2017 "tshade.cpp"
    break;

  case 48: /* expression: expression '%' expression  */
#line 312 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("%", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 2023 "tshade.cpp"
    break;

  case 49: /* expression: expression OP_MOD_ASS expression  */
#line 314 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("%=", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 2029 "tshade.cpp"
    break;

  case 50: /* expression: expression OP_EQ expression  */
#line 316 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("==", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 2035 "tshade.cpp"
    break;

  case 51: /* expression: expression OP_NEQ expression  */
#line 318 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("!=", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 2041 "tshade.cpp"
    break;

  case 52: /* expression: expression '<' expression  */
#line 320 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("<", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 2047 "tshade.cpp"
    break;

  case 53: /* expression: expression '>' expression  */
#line 322 "tshade.y"
    { (yyval.node) = new tBinaryOpNode(">", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 2053 "tshade.cpp"
    break;

  case 54: /* expression: expression OP_GE expression  */
#line 324 "tshade.y"
    { (yyval.node) = new tBinaryOpNode(">=", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 2059 "tshade.cpp"
    break;

  case 55: /* expression: expression OP_AND expression  */
#line 326 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("&&", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 2065 "tshade.cpp"
    break;

  case 56: /* expression: expression OP_OR expression  */
#line 328 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("||", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 2071 "tshade.cpp"
    break;

  case 57: /* expression: '!' expression  */
#line 330 "tshade.y"
    { (yyval.node) = new tUnaryOpNode("!", (yyvsp[0].node)); }
#line 2077 "tshade.cpp"
    break;

  case 58: /* expression: '-' expression  */
#line 332 "tshade.y"
    { (yyval.node) = new tUnaryOpNode("-", (yyvsp[0].node)); }
#line 2083 "tshade.cpp"
    break;

  case 59: /* expression: OP_PLUSPLUS expression  */
#line 334 "tshade.y"
    { (yyval.node) = new tUnaryOpNode("++", (yyvsp[0].node)); }
#line 2089 "tshade.cpp"
    break;

  case 60: /* expression: OP_MINUSMINUS expression  */
#line 336 "tshade.y"
    { (yyval.node) = new tUnaryOpNode("--", (yyvsp[0].node)); }
#line 2095 "tshade.cpp"
    break;

  case 61: /* expression: expression OP_PLUSPLUS  */
#line 338 "tshade.y"
    { (yyval.node) = new tUnaryOpNode("++", (yyvsp[-1].node), false); }
#line 2101 "tshade.cpp"
    break;

  case 62: /* expression: expression OP_MINUSMINUS  */
#line 340 "tshade.y"
    { (yyval.node) = new tUnaryOpNode("--", (yyvsp[-1].node), false); }
#line 2107 "tshade.cpp"
    break;

  case 63: /* expression: '(' expression ')'  */
#line 342 "tshade.y"
    { (yyval.node) = (yyvsp[-1].node); }
#line 2113 "tshade.cpp"
    break;

  case 64: /* expression: var_type '(' expression_list ')'  */
#line 344 "tshade.y"
    { (yyval.node) = new tTypeRefNode((yyvsp[-3].varType), (yyvsp[-1].exprListnode)); }
#line 2119 "tshade.cpp"
    break;

  case 65: /* expression: VAR_IDENT '(' expression_list ')'  */
#line 346 "tshade.y"
    {
      tFunctionNode* funcDecl = shadeAst->findFunction((yyvsp[-3].strVal));
      if (funcDecl) {
          (yyval.node) = new tFunctionRefNode(funcDecl, (yyvsp[-1].exprListnode));
      } else {
          yyerror(scanner, shadeAst, "Undefined function");
          (yyval.node) = nullptr;  // Handle error appropriately
      }
    }
#line 2133 "tshade.cpp"
    break;

  case 66: /* expression: VAR_IDENT tSWIZZLE  */
#line 356 "tshade.y"
    { tVarDeclNode* varDecl = shadeAst->findVar((yyvsp[-1].strVal));
      if (varDecl) {
          (yyval.node) = new tVarRefNode(varDecl, (yyvsp[0].strVal));
      } else {
          yyerror(scanner, shadeAst, "Undefined variable");
          (yyval.node) = nullptr;  // Handle error appropriately
      } 
    }
#line 2146 "tshade.cpp"
    break;

  case 67: /* expression: VAR_IDENT  */
#line 365 "tshade.y"
    { tVarDeclNode* varDecl = shadeAst->findVar((yyvsp[0].strVal));
      if (varDecl) {
          (yyval.node) = new tVarRefNode(varDecl);
      } else {
          yyerror(scanner, shadeAst, "Undefined variable");
          (yyval.node) = nullptr;  // Handle error appropriately
      } 
    }
#line 2159 "tshade.cpp"
    break;

  case 68: /* expression: VAR_IDENT '.' VAR_IDENT  */
#line 374 "tshade.y"
    { tVarDeclNode* varDecl = shadeAst->findVar((yyvsp[-2].strVal));
      if (varDecl->isStruct) {
          (yyval.node) = new tVarRefNode(varDecl, (yyvsp[0].strVal));
      } else {
          yyerror(scanner, shadeAst, "Unknown struct");
          (yyval.node) = nullptr;  // Handle error appropriately
      } 
    }
#line 2172 "tshade.cpp"
    break;

  case 69: /* expression: INT_NUM  */
#line 383 "tshade.y"
    { (yyval.node) = new tIntLiteralNode((yyvsp[0].intVal)); }
#line 2178 "tshade.cpp"
    break;

  case 70: /* expression: FLOAT_NUM  */
#line 385 "tshade.y"
    { (yyval.node) = new tFloatLiteralNode((yyvsp[0].fVal)); }
#line 2184 "tshade.cpp"
    break;

  case 71: /* statement_list: %empty  */
#line 390 "tshade.y"
    { (yyval.stmt_list_node) = new tStatementListNode(); }
#line 2190 "tshade.cpp"
    break;

  case 72: /* statement_list: statement_list statement  */
#line 392 "tshade.y"
    { (yyvsp[-1].stmt_list_node)->addStatement((yyvsp[0].node)); (yyval.stmt_list_node) = (yyvsp[-1].stmt_list_node); }
#line 2196 "tshade.cpp"
    break;

  case 73: /* statement: var_decl  */
#line 397 "tshade.y"
    {(yyval.node) = (yyvsp[0].declNode);}
#line 2202 "tshade.cpp"
    break;

  case 74: /* statement: uniform_decl  */
#line 399 "tshade.y"
    {(yyval.node) = (yyvsp[0].declNode);}
#line 2208 "tshade.cpp"
    break;

  case 75: /* statement: static_const_decl  */
#line 401 "tshade.y"
    {(yyval.node) = (yyvsp[0].declNode);}
#line 2214 "tshade.cpp"
    break;

  case 76: /* statement: if_statement  */
#line 403 "tshade.y"
    {(yyval.node) = (yyvsp[0].node);}
#line 2220 "tshade.cpp"
    break;

  case 77: /* statement: while_statement  */
#line 405 "tshade.y"
    {(yyval.node) = (yyvsp[0].node);}
#line 2226 "tshade.cpp"
    break;

  case 78: /* statement: continue_statement  */
#line 407 "tshade.y"
    {(yyval.node) = (yyvsp[0].node);}
#line 2232 "tshade.cpp"
    break;

  case 79: /* statement: break_statement  */
#line 409 "tshade.y"
    {(yyval.node) = (yyvsp[0].node);}
#line 2238 "tshade.cpp"
    break;

  case 80: /* statement: return_statement  */
#line 411 "tshade.y"
    {(yyval.node) = (yyvsp[0].node);}
#line 2244 "tshade.cpp"
    break;

  case 81: /* statement: function_def  */
#line 413 "tshade.y"
    {(yyval.node) = (yyvsp[0].funcNode); }
#line 2250 "tshade.cpp"
    break;

  case 82: /* statement: expression ';'  */
#line 415 "tshade.y"
    {(yyval.node) = (yyvsp[-1].node);}
#line 2256 "tshade.cpp"
    break;

  case 83: /* statement: switch_statement  */
#line 417 "tshade.y"
    {(yyval.node) = (yyvsp[0].node);}
#line 2262 "tshade.cpp"
    break;

  case 84: /* statement: discard_statement  */
#line 419 "tshade.y"
    {(yyval.node) = (yyvsp[0].node);}
#line 2268 "tshade.cpp"
    break;

  case 85: /* statement: struct_decl  */
#line 421 "tshade.y"
    {(yyval.node) = (yyvsp[0].structNode);}
#line 2274 "tshade.cpp"
    break;

  case 86: /* if_statement: rwIF '(' expression ')' '{' statement_list '}' rwELSE '{' statement_list '}'  */
#line 427 "tshade.y"
    { (yyval.node) = new tIfNode((yyvsp[-8].node), (yyvsp[-5].stmt_list_node), (yyvsp[-1].stmt_list_node)); }
#line 2280 "tshade.cpp"
    break;

  case 87: /* if_statement: rwIF '(' expression ')' '{' statement_list '}'  */
#line 429 "tshade.y"
    { (yyval.node) = new tIfNode((yyvsp[-4].node), (yyvsp[-1].stmt_list_node)); }
#line 2286 "tshade.cpp"
    break;

  case 88: /* while_statement: rwWHILE '(' expression ')' '{' statement_list '}'  */
#line 434 "tshade.y"
    { (yyval.node) = new tWhileNode((yyvsp[-4].node), (yyvsp[-1].stmt_list_node)); }
#line 2292 "tshade.cpp"
    break;

  case 89: /* while_statement: rwDO '{' statement_list '}' rwWHILE '(' expression ')'  */
#line 436 "tshade.y"
    { (yyval.node) = new tWhileNode((yyvsp[-1].node), (yyvsp[-5].stmt_list_node), true); }
#line 2298 "tshade.cpp"
    break;

  case 90: /* switch_statement: rwSWITCH '(' expression ')' '{' case_statements '}'  */
#line 441 "tshade.y"
    {(yyval.node) = new tSwitchNode((yyvsp[-4].node), (yyvsp[-1].stmt_list_node)); }
#line 2304 "tshade.cpp"
    break;

  case 91: /* case_statements: %empty  */
#line 446 "tshade.y"
    { (yyval.stmt_list_node) = new tStatementListNode(); }
#line 2310 "tshade.cpp"
    break;

  case 92: /* case_statements: case_statements case_rule  */
#line 448 "tshade.y"
    { (yyvsp[-1].stmt_list_node)->addStatement((yyvsp[0].node)); (yyval.stmt_list_node) = (yyvsp[-1].stmt_list_node); }
#line 2316 "tshade.cpp"
    break;

  case 93: /* case_rule: rwCASE expression ':' statement_list  */
#line 453 "tshade.y"
    {(yyval.node) = new tCaseNode((yyvsp[-2].node), (yyvsp[0].stmt_list_node));}
#line 2322 "tshade.cpp"
    break;

  case 94: /* case_rule: rwDEFAULT ':' statement_list  */
#line 455 "tshade.y"
    { (yyval.node) = new tCaseNode(nullptr, (yyvsp[0].stmt_list_node), true); }
#line 2328 "tshade.cpp"
    break;

  case 95: /* continue_statement: rwCONTINUE ';'  */
#line 460 "tshade.y"
    { (yyval.node) = new tContinueNode(); }
#line 2334 "tshade.cpp"
    break;

  case 96: /* break_statement: rwBREAK ';'  */
#line 465 "tshade.y"
    { (yyval.node) = new tBreakNode(); }
#line 2340 "tshade.cpp"
    break;

  case 97: /* return_statement: rwRETURN ';'  */
#line 470 "tshade.y"
    { (yyval.node) = new tReturnNode(); }
#line 2346 "tshade.cpp"
    break;

  case 98: /* return_statement: rwRETURN expression ';'  */
#line 472 "tshade.y"
    { (yyval.node) = new tReturnNode((yyvsp[-1].node)); }
#line 2352 "tshade.cpp"
    break;

  case 99: /* discard_statement: rwDISCARD ';'  */
#line 477 "tshade.y"
    { (yyval.node) = new tDiscardNode(); }
#line 2358 "tshade.cpp"
    break;

  case 100: /* var_type: tMAT34_TYPE  */
#line 482 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_MAT34;}
#line 2364 "tshade.cpp"
    break;

  case 101: /* var_type: tMAT43_TYPE  */
#line 484 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_MAT43;}
#line 2370 "tshade.cpp"
    break;

  case 102: /* var_type: tMAT3_TYPE  */
#line 486 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_MAT33;}
#line 2376 "tshade.cpp"
    break;

  case 103: /* var_type: tMAT4_TYPE  */
#line 488 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_MAT44;}
#line 2382 "tshade.cpp"
    break;

  case 104: /* var_type: tFVEC2_TYPE  */
#line 490 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_FLOAT2;}
#line 2388 "tshade.cpp"
    break;

  case 105: /* var_type: tFVEC3_TYPE  */
#line 492 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_FLOAT3;}
#line 2394 "tshade.cpp"
    break;

  case 106: /* var_type: tFVEC4_TYPE  */
#line 494 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_FLOAT4;}
#line 2400 "tshade.cpp"
    break;

  case 107: /* var_type: tIVEC2_TYPE  */
#line 496 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_INT2;}
#line 2406 "tshade.cpp"
    break;

  case 108: /* var_type: tIVEC3_TYPE  */
#line 498 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_INT3;}
#line 2412 "tshade.cpp"
    break;

  case 109: /* var_type: tIVEC4_TYPE  */
#line 500 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_INT4;}
#line 2418 "tshade.cpp"
    break;

  case 110: /* var_type: tBVEC2_TYPE  */
#line 502 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_BOOL2;}
#line 2424 "tshade.cpp"
    break;

  case 111: /* var_type: tBVEC3_TYPE  */
#line 504 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_BOOL3;}
#line 2430 "tshade.cpp"
    break;

  case 112: /* var_type: tBVEC4_TYPE  */
#line 506 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_BOOL4;}
#line 2436 "tshade.cpp"
    break;

  case 113: /* var_type: tFLOAT_TYPE  */
#line 508 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_FLOAT;}
#line 2442 "tshade.cpp"
    break;

  case 114: /* var_type: tINT_TYPE  */
#line 510 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_INT;}
#line 2448 "tshade.cpp"
    break;

  case 115: /* var_type: tUINT_TYPE  */
#line 512 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_UINT;}
#line 2454 "tshade.cpp"
    break;

  case 116: /* var_type: tBOOL_TYPE  */
#line 514 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_BOOL;}
#line 2460 "tshade.cpp"
    break;

  case 117: /* var_type: rwVOID  */
#line 516 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_VOID;}
#line 2466 "tshade.cpp"
    break;

  case 118: /* struct_semantic: tSEM_SVPOSITION  */
#line 521 "tshade.y"
    { (yyval.semtype) = SEMANTIC_SV_POSITION; }
#line 2472 "tshade.cpp"
    break;

  case 119: /* struct_semantic: tSEM_POSITION  */
#line 523 "tshade.y"
    { (yyval.semtype) = SEMANTIC_POSITION; }
#line 2478 "tshade.cpp"
    break;

  case 120: /* struct_semantic: tSEM_NORMAL  */
#line 525 "tshade.y"
    { (yyval.semtype) = SEMANTIC_NORMAL; }
#line 2484 "tshade.cpp"
    break;

  case 121: /* struct_semantic: tSEM_BINORMAL  */
#line 527 "tshade.y"
    { (yyval.semtype) = SEMANTIC_BINORMAL; }
#line 2490 "tshade.cpp"
    break;

  case 122: /* struct_semantic: tSEM_TANGENT  */
#line 529 "tshade.y"
    { (yyval.semtype) = SEMANTIC_TANGENT; }
#line 2496 "tshade.cpp"
    break;

  case 123: /* struct_semantic: tSEM_TEXCOORD  */
#line 531 "tshade.y"
    { (yyval.semtype) = SEMANTIC_TEXCOORD; }
#line 2502 "tshade.cpp"
    break;

  case 124: /* struct_semantic: tSEM_COLOR  */
#line 533 "tshade.y"
   { (yyval.semtype) = SEMANTIC_COLOR; }
#line 2508 "tshade.cpp"
    break;

  case 125: /* struct_semantic: tSEM_TARGET  */
#line 535 "tshade.y"
    { (yyval.semtype) = SEMANTIC_SV_TARGET; }
#line 2514 "tshade.cpp"
    break;

  case 126: /* struct_semantic: tSEM_DEPTH  */
#line 537 "tshade.y"
    { (yyval.semtype) = SEMANTIC_SV_DEPTH; }
#line 2520 "tshade.cpp"
    break;

  case 127: /* struct_semantic: tSEM_ISFRONTFACE  */
#line 539 "tshade.y"
    { (yyval.semtype) = SEMANTIC_ISFRONTFACE; }
#line 2526 "tshade.cpp"
    break;

  case 128: /* struct_semantic: tSEM_TESSFACTOR  */
#line 541 "tshade.y"
    { (yyval.semtype) = SEMANTIC_TESSFACTOR; }
#line 2532 "tshade.cpp"
    break;

  case 129: /* struct_semantic: tSEM_PSIZE  */
#line 543 "tshade.y"
    { (yyval.semtype) = SEMANTIC_PSIZE; }
#line 2538 "tshade.cpp"
    break;


#line 2542 "tshade.cpp"

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
  yytoken = yychar == TSHADE_EMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      {
        yypcontext_t yyctx
          = {yyssp, yytoken};
        if (yyreport_syntax_error (&yyctx, scanner, shadeAst) == 2)
          YYNOMEM;
      }
    }

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
                      yytoken, &yylval, scanner, shadeAst);
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


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp, scanner, shadeAst);
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
  yyerror (scanner, shadeAst, YY_("memory exhausted"));
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
                  yytoken, &yylval, scanner, shadeAst);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp, scanner, shadeAst);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 547 "tshade.y"


void yyerror(yyscan_t yyscanner, tShadeAst* shadeAst, char const *msg) {
	yyerror(yyscanner, msg);
}

void yyerror(yyscan_t yyscanner, const char* msg) {
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

   yyerror(scanner, output.c_str());

   return ret;
}
