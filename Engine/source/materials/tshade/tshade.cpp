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
#line 9 "tshade.y"

  #include <stdlib.h>
  #include <stdio.h>
  #include "platform/platform.h"
  #include "console/console.h"
  #include "core/strings/stringFunctions.h"
  #include "tshadeAst.h"

  #define nil 0

  typedef void* yyscan_t;
  void yyerror(yyscan_t yyscanner, const char* msg);
  void yyerror(yyscan_t yyscanner, tShadeAst* shadeAst, char const *msg);
  #define YY_DECL int yylex(union YYSTYPE *, yyscan_t)
  YY_DECL;

  int yylineno;

#line 96 "tshade.cpp"

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
  YYSYMBOL_shader_stage = 116,             /* shader_stage  */
  YYSYMBOL_shader_body = 117,              /* shader_body  */
  YYSYMBOL_struct_decl = 118,              /* struct_decl  */
  YYSYMBOL_structbody_list = 119,          /* structbody_list  */
  YYSYMBOL_struct_member = 120,            /* struct_member  */
  YYSYMBOL_uniform_decl = 121,             /* uniform_decl  */
  YYSYMBOL_static_const_decl = 122,        /* static_const_decl  */
  YYSYMBOL_var_decl = 123,                 /* var_decl  */
  YYSYMBOL_param_modifier = 124,           /* param_modifier  */
  YYSYMBOL_function_def = 125,             /* function_def  */
  YYSYMBOL_function_param_list = 126,      /* function_param_list  */
  YYSYMBOL_function_param = 127,           /* function_param  */
  YYSYMBOL_expression_list = 128,          /* expression_list  */
  YYSYMBOL_expression = 129,               /* expression  */
  YYSYMBOL_statement_list = 130,           /* statement_list  */
  YYSYMBOL_statement = 131,                /* statement  */
  YYSYMBOL_if_statement = 132,             /* if_statement  */
  YYSYMBOL_while_statement = 133,          /* while_statement  */
  YYSYMBOL_switch_statement = 134,         /* switch_statement  */
  YYSYMBOL_case_statements = 135,          /* case_statements  */
  YYSYMBOL_case_rule = 136,                /* case_rule  */
  YYSYMBOL_continue_statement = 137,       /* continue_statement  */
  YYSYMBOL_break_statement = 138,          /* break_statement  */
  YYSYMBOL_return_statement = 139,         /* return_statement  */
  YYSYMBOL_discard_statement = 140,        /* discard_statement  */
  YYSYMBOL_var_type = 141,                 /* var_type  */
  YYSYMBOL_struct_semantic = 142           /* struct_semantic  */
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
#define YYLAST   1305

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  113
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  30
/* YYNRULES -- Number of rules.  */
#define YYNRULES  128
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  247

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
       0,   132,   132,   133,   139,   140,   142,   147,   153,   159,
     165,   174,   179,   185,   186,   191,   193,   195,   200,   208,
     213,   219,   227,   229,   231,   233,   235,   240,   242,   244,
     249,   251,   257,   258,   260,   265,   267,   272,   274,   279,
     281,   283,   285,   287,   289,   291,   293,   295,   297,   299,
     301,   303,   305,   307,   309,   311,   313,   315,   317,   319,
     321,   323,   325,   327,   329,   331,   341,   350,   359,   368,
     370,   376,   377,   382,   384,   386,   388,   390,   392,   394,
     396,   398,   400,   402,   404,   410,   412,   417,   419,   424,
     430,   431,   436,   438,   443,   448,   453,   455,   460,   465,
     467,   469,   471,   473,   475,   477,   479,   481,   483,   485,
     487,   489,   491,   493,   495,   497,   499,   504,   506,   508,
     510,   512,   514,   516,   518,   520,   522,   524,   526
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if TSHADE_DEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "'+'", "'-'", "'*'",
  "'/'", "'<'", "'>'", "'='", "'.'", "'|'", "'&'", "'%'", "'('", "')'",
  "','", "':'", "';'", "'{'", "'}'", "'^'", "'~'", "'!'", "'@'",
  "rwSWITCH", "rwCASE", "rwDEFAULT", "rwWHILE", "rwDO", "rwFOR", "rwBREAK",
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
  "$accept", "program", "program_globals", "shader_stage", "shader_body",
  "struct_decl", "structbody_list", "struct_member", "uniform_decl",
  "static_const_decl", "var_decl", "param_modifier", "function_def",
  "function_param_list", "function_param", "expression_list", "expression",
  "statement_list", "statement", "if_statement", "while_statement",
  "switch_statement", "case_statements", "case_rule", "continue_statement",
  "break_statement", "return_statement", "discard_statement", "var_type",
  "struct_semantic", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-131)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-18)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     -74,   -64,     9,    -4,  -131,    79,   -31,    22,    59,    70,
      73,    88,  -131,  -131,    99,  -131,  -131,  -131,  -131,  -131,
     100,   108,   684,   125,   127,   128,   130,   684,  -131,   874,
     874,   874,   140,   145,   142,   126,   146,   149,   174,  -131,
     -28,   779,    83,   874,   874,  -131,  -131,     0,   114,    83,
    -131,  -131,  -131,  -131,  -131,  -131,  -131,  -131,  -131,  -131,
    -131,  -131,  -131,  -131,  -131,  -131,  -131,  -131,  -131,  -131,
    -131,  1005,  -131,  -131,  -131,  -131,  -131,  -131,  -131,  -131,
       8,  -131,  -131,  -131,  -131,  -131,   176,    25,    -7,   180,
    1021,    -7,   874,   874,  -131,  -131,  -131,   874,  -131,    83,
    -131,   131,  -131,  1037,  -131,    -7,    -7,   132,   874,  -131,
     179,  -131,   874,   874,   874,   874,   874,   874,   874,   874,
    -131,   874,   874,   874,   874,   874,  -131,  -131,   874,   874,
     874,   874,   874,   874,    10,    26,  -131,  1093,  1106,   209,
    1119,  -131,    -2,  -131,  -131,    18,   847,  -131,   686,   686,
      -7,    -7,   591,   591,   192,    -7,   497,   497,   402,   307,
     591,   129,   129,   129,   129,   129,    37,   874,   892,  -131,
     874,   164,   182,   184,   185,   177,   189,  -131,   874,  -131,
    1175,  -131,  -131,  -131,   909,    57,  -131,   143,    78,  -131,
    -131,  -131,  -131,  -131,  -131,  -131,  -131,  -131,  -131,  -131,
    -131,   194,  -131,  -131,   196,  -131,   847,  -131,   147,   124,
     892,  -131,    12,  -131,    96,   304,   874,   399,  -131,  -131,
    -131,  -131,   197,  -131,  -131,   874,   200,  -131,  -131,  1191,
     190,   494,   874,  1247,  -131,  -131,   207,  -131,    16,  -131,
     684,  -131,   201,   684,   589,  -131,  -131
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,     0,     0,     0,     1,     4,     0,     0,     0,     0,
       0,     0,     6,     5,     0,    71,    71,    71,    71,     3,
      71,     0,    11,     0,     0,     0,     0,     0,     7,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   116,
       0,     0,     0,     0,     0,    69,    70,    67,     0,     0,
     112,   113,   115,   114,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   102,   100,    99,   101,    74,    75,    73,
      81,     0,    72,    76,    77,    83,    78,    79,    80,    84,
       0,     8,     9,    10,    12,    14,    81,     0,    58,     0,
       0,    57,     0,     0,    71,    95,    94,     0,    98,     0,
      19,     0,    96,     0,    21,    59,    60,     0,     0,    66,
       0,    18,     0,     0,     0,     0,     0,     0,     0,     0,
      82,     0,     0,     0,     0,     0,    61,    62,     0,     0,
       0,     0,     0,     0,     0,     0,    63,     0,     0,     0,
       0,    20,     0,    97,    68,     0,    37,    26,    39,    42,
      44,    46,    52,    53,    41,    48,    50,    51,    55,    56,
      54,    40,    43,    45,    47,    49,     0,     0,    32,    22,
       0,     0,    22,     0,     0,     0,     0,    65,     0,    64,
       0,    27,    28,    29,     0,     0,    33,     0,     0,   117,
     118,   119,   120,   121,   128,   127,   126,   122,   123,   124,
     125,     0,    90,    71,     0,    71,    38,    23,     0,     0,
       0,    36,     0,    15,     0,     0,     0,     0,    35,    31,
      71,    34,     0,    24,    89,     0,     0,    91,    87,     0,
      86,     0,     0,     0,    71,    88,     0,    30,     0,    71,
      93,    71,     0,    92,     0,    25,    85
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -131,  -131,  -131,  -131,   123,  -131,  -131,  -131,  -131,  -131,
     -11,  -131,   203,  -131,    21,  -130,   -18,   -14,  -131,  -131,
    -131,  -131,  -131,  -131,  -131,  -131,  -131,  -131,   -22,  -131
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,     2,    11,    12,    21,    13,    26,    85,    67,    68,
      69,   184,    70,   185,   186,   145,    71,    22,    72,    73,
      74,    75,   214,   227,    76,    77,    78,    79,    89,   201
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      80,     1,   118,   166,     3,    87,    27,   167,    39,     4,
     107,    88,    90,    91,   108,     5,   169,    99,   101,   167,
     101,   222,   133,   103,   168,   105,   106,   101,   169,   100,
     223,   104,   178,   177,   178,   167,   242,    14,   111,   133,
     168,    15,    48,   171,   172,   126,   127,   128,   129,   130,
     131,   132,   179,   178,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,   109,   209,   210,   137,   138,   134,   101,    16,   140,
     139,   112,   113,   114,   115,   116,   117,   118,   141,    17,
     146,   119,    18,   135,   148,   149,   150,   151,   152,   153,
     154,   155,   238,   156,   157,   158,   159,   160,    19,   170,
     161,   162,   163,   164,   165,   146,   224,    80,    20,    39,
     -13,   170,   225,   226,   121,   122,   123,   124,    28,   125,
     126,   127,   128,   129,   130,   131,   132,   170,   118,    23,
      24,    25,   219,   220,    95,    81,   187,    82,    83,   180,
      84,     6,   188,    48,    92,     7,     8,     9,    10,    93,
     206,    94,   208,    97,    96,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,   110,   128,   129,   130,   131,   132,   187,   215,
     212,   217,    98,    80,   133,    80,   -17,   147,   229,   142,
     144,   118,   -16,   202,   203,   204,   231,   233,   205,    80,
     216,   211,   213,    29,   146,   218,   232,   234,    80,   245,
     240,    80,    80,    30,   236,   243,   241,   244,     0,   175,
      86,   221,    31,     0,    32,     0,     0,    33,    34,     0,
      35,    36,    37,     0,    38,    39,    40,     0,     0,     0,
       0,     0,     0,    41,    42,     0,     0,     0,     0,     0,
       0,    43,    44,   189,   190,   191,   192,   193,   194,   195,
     196,   197,   198,   199,   200,    45,    46,    47,     0,    48,
       0,     0,    49,     0,     0,     0,     0,     0,     0,     0,
       0,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    29,     0,
     112,   113,   114,   115,   116,   117,   118,     0,    30,     0,
     119,     0,     0,     0,   228,     0,     0,    31,     0,    32,
       0,     0,    33,    34,     0,    35,    36,    37,     0,    38,
      39,    40,     0,     0,     0,     0,     0,     0,    41,    42,
       0,     0,     0,   121,   122,   123,    43,    44,   125,   126,
     127,   128,   129,   130,   131,   132,     0,     0,     0,     0,
      45,    46,    47,     0,    48,     0,     0,    49,     0,     0,
       0,     0,     0,     0,     0,     0,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    29,     0,   112,   113,   114,   115,   116,
     117,   118,     0,    30,     0,   119,     0,     0,     0,   230,
       0,     0,    31,     0,    32,     0,     0,    33,    34,     0,
      35,    36,    37,     0,    38,    39,    40,     0,     0,     0,
       0,     0,     0,    41,    42,     0,     0,     0,   121,   122,
       0,    43,    44,   125,   126,   127,   128,   129,   130,   131,
     132,     0,     0,     0,     0,    45,    46,    47,     0,    48,
       0,     0,    49,     0,     0,     0,     0,     0,     0,     0,
       0,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    29,     0,
     112,   113,   114,   115,   116,   117,   118,     0,    30,     0,
     119,     0,     0,     0,   237,     0,     0,    31,     0,    32,
       0,     0,    33,    34,     0,    35,    36,    37,     0,    38,
      39,    40,     0,     0,     0,     0,     0,     0,    41,    42,
       0,     0,     0,     0,     0,     0,    43,    44,   125,   126,
     127,   128,   129,   130,   131,   132,     0,     0,     0,     0,
      45,    46,    47,     0,    48,     0,     0,    49,     0,     0,
       0,     0,     0,     0,     0,     0,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    29,   112,   113,   114,   115,     0,     0,
     118,     0,     0,    30,   119,     0,     0,     0,     0,   246,
       0,     0,    31,     0,    32,     0,     0,    33,    34,     0,
      35,    36,    37,     0,    38,    39,    40,     0,     0,     0,
       0,     0,     0,    41,    42,     0,     0,     0,     0,     0,
       0,    43,    44,   126,   127,   128,   129,   130,   131,   132,
       0,     0,     0,     0,     0,    45,    46,    47,     0,    48,
       0,     0,    49,     0,     0,     0,     0,     0,     0,     0,
       0,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    29,     0,
       0,   114,   115,     0,     0,   118,     0,     0,    30,   119,
       0,     0,     0,     0,     0,     0,     0,    31,     0,    32,
       0,     0,    33,    34,     0,    35,    36,    37,     0,    38,
      39,    40,     0,     0,     0,     0,     0,     0,    41,    42,
       0,     0,     0,     0,     0,     0,    43,    44,   126,   127,
     128,   129,   130,   131,   132,     0,     0,     0,     0,     0,
      45,    46,    47,     0,    48,     0,     0,    49,     0,     0,
       0,     0,     0,     0,     0,     0,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    29,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    30,     0,     0,     0,   102,     0,     0,
       0,     0,    31,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    39,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    43,    44,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    45,    46,    47,     0,     0,
     112,   113,   114,   115,   116,   117,   118,     0,     0,     0,
     119,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    29,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    30,     0,
       0,     0,     0,   121,   122,   123,   124,    31,   125,   126,
     127,   128,   129,   130,   131,   132,     0,     0,     0,     0,
      39,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    43,    44,    39,     0,
     181,   182,   183,     0,     0,     0,     0,     0,     0,     0,
      45,    46,    47,     0,     0,    39,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,     0,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,   112,   113,
     114,   115,   116,   117,   118,     0,     0,     0,   119,     0,
       0,     0,     0,   120,   112,   113,   114,   115,   116,   117,
     118,     0,     0,     0,   119,     0,   136,     0,     0,     0,
     112,   113,   114,   115,   116,   117,   118,     0,     0,     0,
     119,   121,   122,   123,   124,   143,   125,   126,   127,   128,
     129,   130,   131,   132,     0,     0,     0,   121,   122,   123,
     124,     0,   125,   126,   127,   128,   129,   130,   131,   132,
       0,     0,     0,   121,   122,   123,   124,     0,   125,   126,
     127,   128,   129,   130,   131,   132,   112,   113,   114,   115,
     116,   117,   118,     0,     0,     0,   119,     0,   173,   112,
     113,   114,   115,   116,   117,   118,     0,     0,     0,   119,
       0,   174,   112,   113,   114,   115,   116,   117,   118,     0,
       0,     0,   119,     0,   176,     0,     0,     0,     0,   121,
     122,   123,   124,     0,   125,   126,   127,   128,   129,   130,
     131,   132,   121,   122,   123,   124,     0,   125,   126,   127,
     128,   129,   130,   131,   132,   121,   122,   123,   124,     0,
     125,   126,   127,   128,   129,   130,   131,   132,   112,   113,
     114,   115,   116,   117,   118,     0,     0,     0,   119,     0,
       0,     0,     0,   207,   112,   113,   114,   115,   116,   117,
     118,     0,     0,     0,   119,     0,   235,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   121,   122,   123,   124,     0,   125,   126,   127,   128,
     129,   130,   131,   132,     0,     0,     0,   121,   122,   123,
     124,     0,   125,   126,   127,   128,   129,   130,   131,   132,
     112,   113,   114,   115,   116,   117,   118,     0,     0,     0,
     119,     0,     0,     0,   239,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   121,   122,   123,   124,     0,   125,   126,
     127,   128,   129,   130,   131,   132
};

static const yytype_int16 yycheck[] =
{
      22,    75,     9,   133,    68,    27,    20,     9,    36,     0,
      10,    29,    30,    31,    14,    19,    18,    45,    40,     9,
      42,     9,    14,    41,    14,    43,    44,    49,    18,    40,
      18,    42,    16,    15,    16,     9,    20,    68,    49,    14,
      14,    19,    70,    17,    18,    52,    53,    54,    55,    56,
      57,    58,    15,    16,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    71,    15,    16,    92,    93,    68,    99,    19,    97,
      94,     3,     4,     5,     6,     7,     8,     9,    99,    19,
     108,    13,    19,    68,   112,   113,   114,   115,   116,   117,
     118,   119,   232,   121,   122,   123,   124,   125,    20,   111,
     128,   129,   130,   131,   132,   133,    20,   139,    19,    36,
      20,   111,    26,    27,    46,    47,    48,    49,    20,    51,
      52,    53,    54,    55,    56,    57,    58,   111,     9,    16,
      17,    18,    18,    19,    18,    20,   168,    20,    20,   167,
      20,    72,   170,    70,    14,    76,    77,    78,    79,    14,
     178,    19,   184,    14,    18,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    68,    54,    55,    56,    57,    58,   210,   203,
     112,   205,    18,   215,    14,   217,    20,    18,   216,    68,
      68,     9,    20,    19,    19,    28,   220,   225,    19,   231,
      14,    68,    18,     4,   232,    68,    19,    17,   240,    18,
     234,   243,   244,    14,    34,   239,    19,   241,    -1,    20,
      27,   210,    23,    -1,    25,    -1,    -1,    28,    29,    -1,
      31,    32,    33,    -1,    35,    36,    37,    -1,    -1,    -1,
      -1,    -1,    -1,    44,    45,    -1,    -1,    -1,    -1,    -1,
      -1,    52,    53,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,    66,    67,    68,    -1,    70,
      -1,    -1,    73,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,     4,    -1,
       3,     4,     5,     6,     7,     8,     9,    -1,    14,    -1,
      13,    -1,    -1,    -1,    20,    -1,    -1,    23,    -1,    25,
      -1,    -1,    28,    29,    -1,    31,    32,    33,    -1,    35,
      36,    37,    -1,    -1,    -1,    -1,    -1,    -1,    44,    45,
      -1,    -1,    -1,    46,    47,    48,    52,    53,    51,    52,
      53,    54,    55,    56,    57,    58,    -1,    -1,    -1,    -1,
      66,    67,    68,    -1,    70,    -1,    -1,    73,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,     4,    -1,     3,     4,     5,     6,     7,
       8,     9,    -1,    14,    -1,    13,    -1,    -1,    -1,    20,
      -1,    -1,    23,    -1,    25,    -1,    -1,    28,    29,    -1,
      31,    32,    33,    -1,    35,    36,    37,    -1,    -1,    -1,
      -1,    -1,    -1,    44,    45,    -1,    -1,    -1,    46,    47,
      -1,    52,    53,    51,    52,    53,    54,    55,    56,    57,
      58,    -1,    -1,    -1,    -1,    66,    67,    68,    -1,    70,
      -1,    -1,    73,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,     4,    -1,
       3,     4,     5,     6,     7,     8,     9,    -1,    14,    -1,
      13,    -1,    -1,    -1,    20,    -1,    -1,    23,    -1,    25,
      -1,    -1,    28,    29,    -1,    31,    32,    33,    -1,    35,
      36,    37,    -1,    -1,    -1,    -1,    -1,    -1,    44,    45,
      -1,    -1,    -1,    -1,    -1,    -1,    52,    53,    51,    52,
      53,    54,    55,    56,    57,    58,    -1,    -1,    -1,    -1,
      66,    67,    68,    -1,    70,    -1,    -1,    73,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,     4,     3,     4,     5,     6,    -1,    -1,
       9,    -1,    -1,    14,    13,    -1,    -1,    -1,    -1,    20,
      -1,    -1,    23,    -1,    25,    -1,    -1,    28,    29,    -1,
      31,    32,    33,    -1,    35,    36,    37,    -1,    -1,    -1,
      -1,    -1,    -1,    44,    45,    -1,    -1,    -1,    -1,    -1,
      -1,    52,    53,    52,    53,    54,    55,    56,    57,    58,
      -1,    -1,    -1,    -1,    -1,    66,    67,    68,    -1,    70,
      -1,    -1,    73,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,     4,    -1,
      -1,     5,     6,    -1,    -1,     9,    -1,    -1,    14,    13,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    23,    -1,    25,
      -1,    -1,    28,    29,    -1,    31,    32,    33,    -1,    35,
      36,    37,    -1,    -1,    -1,    -1,    -1,    -1,    44,    45,
      -1,    -1,    -1,    -1,    -1,    -1,    52,    53,    52,    53,
      54,    55,    56,    57,    58,    -1,    -1,    -1,    -1,    -1,
      66,    67,    68,    -1,    70,    -1,    -1,    73,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,     4,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    14,    -1,    -1,    -1,    18,    -1,    -1,
      -1,    -1,    23,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    36,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    52,    53,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    66,    67,    68,    -1,    -1,
       3,     4,     5,     6,     7,     8,     9,    -1,    -1,    -1,
      13,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,     4,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    14,    -1,
      -1,    -1,    -1,    46,    47,    48,    49,    23,    51,    52,
      53,    54,    55,    56,    57,    58,    -1,    -1,    -1,    -1,
      36,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    52,    53,    36,    -1,
      38,    39,    40,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      66,    67,    68,    -1,    -1,    36,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    -1,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,     3,     4,
       5,     6,     7,     8,     9,    -1,    -1,    -1,    13,    -1,
      -1,    -1,    -1,    18,     3,     4,     5,     6,     7,     8,
       9,    -1,    -1,    -1,    13,    -1,    15,    -1,    -1,    -1,
       3,     4,     5,     6,     7,     8,     9,    -1,    -1,    -1,
      13,    46,    47,    48,    49,    18,    51,    52,    53,    54,
      55,    56,    57,    58,    -1,    -1,    -1,    46,    47,    48,
      49,    -1,    51,    52,    53,    54,    55,    56,    57,    58,
      -1,    -1,    -1,    46,    47,    48,    49,    -1,    51,    52,
      53,    54,    55,    56,    57,    58,     3,     4,     5,     6,
       7,     8,     9,    -1,    -1,    -1,    13,    -1,    15,     3,
       4,     5,     6,     7,     8,     9,    -1,    -1,    -1,    13,
      -1,    15,     3,     4,     5,     6,     7,     8,     9,    -1,
      -1,    -1,    13,    -1,    15,    -1,    -1,    -1,    -1,    46,
      47,    48,    49,    -1,    51,    52,    53,    54,    55,    56,
      57,    58,    46,    47,    48,    49,    -1,    51,    52,    53,
      54,    55,    56,    57,    58,    46,    47,    48,    49,    -1,
      51,    52,    53,    54,    55,    56,    57,    58,     3,     4,
       5,     6,     7,     8,     9,    -1,    -1,    -1,    13,    -1,
      -1,    -1,    -1,    18,     3,     4,     5,     6,     7,     8,
       9,    -1,    -1,    -1,    13,    -1,    15,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    46,    47,    48,    49,    -1,    51,    52,    53,    54,
      55,    56,    57,    58,    -1,    -1,    -1,    46,    47,    48,
      49,    -1,    51,    52,    53,    54,    55,    56,    57,    58,
       3,     4,     5,     6,     7,     8,     9,    -1,    -1,    -1,
      13,    -1,    -1,    -1,    17,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    46,    47,    48,    49,    -1,    51,    52,
      53,    54,    55,    56,    57,    58
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    75,   114,    68,     0,    19,    72,    76,    77,    78,
      79,   115,   116,   118,    68,    19,    19,    19,    19,    20,
      19,   117,   130,   117,   117,   117,   119,   130,    20,     4,
      14,    23,    25,    28,    29,    31,    32,    33,    35,    36,
      37,    44,    45,    52,    53,    66,    67,    68,    70,    73,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,   121,   122,   123,
     125,   129,   131,   132,   133,   134,   137,   138,   139,   140,
     141,    20,    20,    20,    20,   120,   125,   141,   129,   141,
     129,   129,    14,    14,    19,    18,    18,    14,    18,    45,
     123,   141,    18,   129,   123,   129,   129,    10,    14,    71,
      68,   123,     3,     4,     5,     6,     7,     8,     9,    13,
      18,    46,    47,    48,    49,    51,    52,    53,    54,    55,
      56,    57,    58,    14,    68,    68,    15,   129,   129,   130,
     129,   123,    68,    18,    68,   128,   129,    18,   129,   129,
     129,   129,   129,   129,   129,   129,   129,   129,   129,   129,
     129,   129,   129,   129,   129,   129,   128,     9,    14,    18,
     111,    17,    18,    15,    15,    20,    15,    15,    16,    15,
     129,    38,    39,    40,   124,   126,   127,   141,   129,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   142,    19,    19,    28,    19,   129,    18,   141,    15,
      16,    68,   112,    18,   135,   130,    14,   130,    68,    18,
      19,   127,     9,    18,    20,    26,    27,   136,    20,   129,
      20,   130,    19,   129,    17,    15,    34,    20,   128,    17,
     130,    19,    20,   130,   130,    18,    20
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_uint8 yyr1[] =
{
       0,   113,   114,   114,   115,   115,   115,   116,   116,   116,
     116,   117,   118,   119,   119,   120,   120,   120,   121,   122,
     122,   122,   123,   123,   123,   123,   123,   124,   124,   124,
     125,   125,   126,   126,   126,   127,   127,   128,   128,   129,
     129,   129,   129,   129,   129,   129,   129,   129,   129,   129,
     129,   129,   129,   129,   129,   129,   129,   129,   129,   129,
     129,   129,   129,   129,   129,   129,   129,   129,   129,   129,
     129,   130,   130,   131,   131,   131,   131,   131,   131,   131,
     131,   131,   131,   131,   131,   132,   132,   133,   133,   134,
     135,   135,   136,   136,   137,   138,   139,   139,   140,   141,
     141,   141,   141,   141,   141,   141,   141,   141,   141,   141,
     141,   141,   141,   141,   141,   141,   141,   142,   142,   142,
     142,   142,   142,   142,   142,   142,   142,   142,   142
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     5,     0,     1,     1,     4,     4,     4,
       4,     1,     5,     0,     2,     5,     3,     1,     2,     2,
       3,     2,     3,     5,     6,    10,     3,     1,     1,     1,
       8,     6,     0,     1,     3,     3,     2,     1,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     2,     2,     2,
       2,     2,     2,     3,     4,     4,     2,     1,     3,     1,
       1,     0,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     2,     1,     1,    11,     7,     7,     8,     7,
       0,     2,     4,     3,     2,     2,     2,     3,     2,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1
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
  case 2: /* program: %empty  */
#line 132 "tshade.y"
    { (yyval.node) = nullptr;}
#line 1628 "tshade.cpp"
    break;

  case 3: /* program: tSHADERDECLARE VAR_IDENT '{' program_globals '}'  */
#line 134 "tshade.y"
    {(yyval.node) = nullptr; shadeAst->shaderName = (yyvsp[-3].strVal); }
#line 1634 "tshade.cpp"
    break;

  case 4: /* program_globals: %empty  */
#line 139 "tshade.y"
    {(yyval.node) = nullptr; }
#line 1640 "tshade.cpp"
    break;

  case 5: /* program_globals: struct_decl  */
#line 141 "tshade.y"
    {(yyval.node) = nullptr; shadeAst->addDataStruct((yyvsp[0].structNode)); }
#line 1646 "tshade.cpp"
    break;

  case 6: /* program_globals: shader_stage  */
#line 143 "tshade.y"
    { (yyval.node) = nullptr; }
#line 1652 "tshade.cpp"
    break;

  case 7: /* shader_stage: tVSSHADER '{' shader_body '}'  */
#line 148 "tshade.y"
    { 
      shadeAst->currentStage = ShaderStageType::tSTAGE_VERTEX;
      shadeAst->mVertStage = new tStageNode(ShaderStageType::tSTAGE_VERTEX, (yyvsp[-1].node));
      shadeAst->currentStage = ShaderStageType::tSTAGE_GLOBAL;
    }
#line 1662 "tshade.cpp"
    break;

  case 8: /* shader_stage: tPSSHADER '{' shader_body '}'  */
#line 154 "tshade.y"
    {
      shadeAst->currentStage = ShaderStageType::tSTAGE_PIXEL;
      shadeAst->mPixStage = new tStageNode(ShaderStageType::tSTAGE_PIXEL, (yyvsp[-1].node));
      shadeAst->currentStage = ShaderStageType::tSTAGE_GLOBAL;
    }
#line 1672 "tshade.cpp"
    break;

  case 9: /* shader_stage: tGSSHADER '{' shader_body '}'  */
#line 160 "tshade.y"
    {
      shadeAst->currentStage = ShaderStageType::tSTAGE_GEOMETRY;
      shadeAst->mPixStage = new tStageNode(ShaderStageType::tSTAGE_GEOMETRY, (yyvsp[-1].node));
      shadeAst->currentStage = ShaderStageType::tSTAGE_GLOBAL;
    }
#line 1682 "tshade.cpp"
    break;

  case 10: /* shader_stage: tCSSHADER '{' shader_body '}'  */
#line 166 "tshade.y"
    {
      shadeAst->currentStage = ShaderStageType::tSTAGE_COMPUTE;
      shadeAst->mPixStage = new tStageNode(ShaderStageType::tSTAGE_COMPUTE, (yyvsp[-1].node));
      shadeAst->currentStage = ShaderStageType::tSTAGE_GLOBAL;
    }
#line 1692 "tshade.cpp"
    break;

  case 11: /* shader_body: statement_list  */
#line 175 "tshade.y"
    {(yyval.node) = (yyvsp[0].stmt_list_node);}
#line 1698 "tshade.cpp"
    break;

  case 12: /* struct_decl: tSTRUCT VAR_IDENT '{' structbody_list '}'  */
#line 180 "tshade.y"
    { (yyval.structNode) = new tStructNode((yyvsp[-3].strVal), (yyvsp[-1].stmt_list_node)); }
#line 1704 "tshade.cpp"
    break;

  case 13: /* structbody_list: %empty  */
#line 185 "tshade.y"
    { (yyval.stmt_list_node) = new tStatementListNode(); }
#line 1710 "tshade.cpp"
    break;

  case 14: /* structbody_list: statement_list struct_member  */
#line 187 "tshade.y"
    { (yyvsp[-1].stmt_list_node)->addStatement((yyvsp[0].node)); (yyval.stmt_list_node) = (yyvsp[-1].stmt_list_node); }
#line 1716 "tshade.cpp"
    break;

  case 15: /* struct_member: var_type VAR_IDENT ':' struct_semantic ';'  */
#line 192 "tshade.y"
    {(yyval.node) = new tStructMemberNode((yyvsp[-3].strVal), (yyvsp[-4].varType), (yyvsp[-1].semtype), yylval.intVal); }
#line 1722 "tshade.cpp"
    break;

  case 16: /* struct_member: var_type VAR_IDENT ';'  */
#line 194 "tshade.y"
    {(yyval.node) = new tStructMemberNode((yyvsp[-1].strVal), (yyvsp[-2].varType)); }
#line 1728 "tshade.cpp"
    break;

  case 17: /* struct_member: function_def  */
#line 196 "tshade.y"
    {(yyval.node) = (yyvsp[0].funcNode);}
#line 1734 "tshade.cpp"
    break;

  case 18: /* uniform_decl: tUNIFORM var_decl  */
#line 201 "tshade.y"
  {
    (yyvsp[0].declNode)->isUniform = true;
    (yyval.declNode) = (yyvsp[0].declNode);
  }
#line 1743 "tshade.cpp"
    break;

  case 19: /* static_const_decl: rwSTATIC var_decl  */
#line 209 "tshade.y"
  {
    (yyvsp[0].declNode)->isStatic = true;
    (yyval.declNode) = (yyvsp[0].declNode);
  }
#line 1752 "tshade.cpp"
    break;

  case 20: /* static_const_decl: rwSTATIC rwCONST var_decl  */
#line 214 "tshade.y"
  {
    (yyvsp[0].declNode)->isStatic = true;
    (yyvsp[0].declNode)->isConst = true;
    (yyval.declNode) = (yyvsp[0].declNode);
  }
#line 1762 "tshade.cpp"
    break;

  case 21: /* static_const_decl: rwCONST var_decl  */
#line 220 "tshade.y"
  {
    (yyvsp[0].declNode)->isConst = true;
    (yyval.declNode) = (yyvsp[0].declNode);
  }
#line 1771 "tshade.cpp"
    break;

  case 22: /* var_decl: var_type VAR_IDENT ';'  */
#line 228 "tshade.y"
    {(yyval.declNode) = new tVarDeclNode((yyvsp[-1].strVal), (yyvsp[-2].varType)); shadeAst->addVarDecl((yyval.declNode)); }
#line 1777 "tshade.cpp"
    break;

  case 23: /* var_decl: var_type VAR_IDENT '=' expression ';'  */
#line 230 "tshade.y"
    {(yyval.declNode) = new tVarDeclNode((yyvsp[-3].strVal), (yyvsp[-4].varType), (yyvsp[-1].node)); shadeAst->addVarDecl((yyval.declNode)); }
#line 1783 "tshade.cpp"
    break;

  case 24: /* var_decl: var_type VAR_IDENT '[' expression ']' ';'  */
#line 232 "tshade.y"
    {(yyval.declNode) = new tVarDeclNode((yyvsp[-4].strVal), (yyvsp[-5].varType), nullptr, (yyvsp[-2].node)); shadeAst->addVarDecl((yyval.declNode)); }
#line 1789 "tshade.cpp"
    break;

  case 25: /* var_decl: var_type VAR_IDENT '[' expression ']' '=' '{' expression_list '}' ';'  */
#line 234 "tshade.y"
    {(yyval.declNode) = new tVarDeclNode((yyvsp[-8].strVal), (yyvsp[-9].varType), (yyvsp[-2].exprListnode), (yyvsp[-6].node)); shadeAst->addVarDecl((yyval.declNode)); }
#line 1795 "tshade.cpp"
    break;

  case 26: /* var_decl: TYPE_IDENT VAR_IDENT ';'  */
#line 236 "tshade.y"
    {(yyval.declNode) = new tVarDeclNode((yyvsp[-1].strVal), ShaderVarType::tTYPE_STRUCT, nullptr, nullptr, true); shadeAst->addVarDecl((yyval.declNode)); }
#line 1801 "tshade.cpp"
    break;

  case 27: /* param_modifier: rwIN  */
#line 241 "tshade.y"
    { (yyval.modifier) = ParamModifier::PARAM_MOD_IN; }
#line 1807 "tshade.cpp"
    break;

  case 28: /* param_modifier: rwOUT  */
#line 243 "tshade.y"
    { (yyval.modifier) = ParamModifier::PARAM_MOD_OUT; }
#line 1813 "tshade.cpp"
    break;

  case 29: /* param_modifier: rwINOUT  */
#line 245 "tshade.y"
    { (yyval.modifier) = ParamModifier::PARAM_MOD_INOUT; }
#line 1819 "tshade.cpp"
    break;

  case 30: /* function_def: var_type VAR_IDENT '(' function_param_list ')' '{' statement_list '}'  */
#line 250 "tshade.y"
    { (yyval.funcNode) = new tFunctionDefNode((yyvsp[-6].strVal), (yyvsp[-7].varType), (yyvsp[-4].funcList), (yyvsp[-1].stmt_list_node)); shadeAst->addfunction((yyval.funcNode)); }
#line 1825 "tshade.cpp"
    break;

  case 31: /* function_def: var_type VAR_IDENT '(' function_param_list ')' ';'  */
#line 252 "tshade.y"
    { (yyval.funcNode) = new tFunctionDeclNode((yyvsp[-4].strVal), (yyvsp[-5].varType), (yyvsp[-2].funcList)); shadeAst->addfunction((yyval.funcNode)); }
#line 1831 "tshade.cpp"
    break;

  case 32: /* function_param_list: %empty  */
#line 257 "tshade.y"
    { (yyval.funcList) = new tFunctionParamListNode(); }
#line 1837 "tshade.cpp"
    break;

  case 33: /* function_param_list: function_param  */
#line 259 "tshade.y"
    { (yyval.funcList) = new tFunctionParamListNode(); (yyval.funcList)->addParam((yyvsp[0].funcParam)); }
#line 1843 "tshade.cpp"
    break;

  case 34: /* function_param_list: function_param_list ',' function_param  */
#line 261 "tshade.y"
    {(yyvsp[-2].funcList)->addParam((yyvsp[0].funcParam)); (yyval.funcList) = (yyvsp[-2].funcList); }
#line 1849 "tshade.cpp"
    break;

  case 35: /* function_param: param_modifier var_type VAR_IDENT  */
#line 266 "tshade.y"
    { (yyval.funcParam) = new tFunctionParamNode((yyvsp[0].strVal), (yyvsp[-1].varType), (yyvsp[-2].modifier)); }
#line 1855 "tshade.cpp"
    break;

  case 36: /* function_param: var_type VAR_IDENT  */
#line 268 "tshade.y"
    { (yyval.funcParam) = new tFunctionParamNode((yyvsp[0].strVal), (yyvsp[-1].varType), ParamModifier::PARAM_MOD_NONE); }
#line 1861 "tshade.cpp"
    break;

  case 37: /* expression_list: expression  */
#line 273 "tshade.y"
    { (yyval.exprListnode) = new tExpressionListNode(); (yyval.exprListnode)->addExpression((yyvsp[0].node)); }
#line 1867 "tshade.cpp"
    break;

  case 38: /* expression_list: expression_list ',' expression  */
#line 275 "tshade.y"
    { (yyval.exprListnode)->addExpression((yyvsp[0].node)); (yyval.exprListnode) = (yyvsp[-2].exprListnode); }
#line 1873 "tshade.cpp"
    break;

  case 39: /* expression: expression '+' expression  */
#line 280 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("+", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1879 "tshade.cpp"
    break;

  case 40: /* expression: expression OP_PLUS_ASS expression  */
#line 282 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("+=", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1885 "tshade.cpp"
    break;

  case 41: /* expression: expression '=' expression  */
#line 284 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("=", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1891 "tshade.cpp"
    break;

  case 42: /* expression: expression '-' expression  */
#line 286 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("-", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1897 "tshade.cpp"
    break;

  case 43: /* expression: expression OP_MINUS_ASS expression  */
#line 288 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("-=", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1903 "tshade.cpp"
    break;

  case 44: /* expression: expression '*' expression  */
#line 290 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("*", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1909 "tshade.cpp"
    break;

  case 45: /* expression: expression OP_MUL_ASS expression  */
#line 292 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("*=", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1915 "tshade.cpp"
    break;

  case 46: /* expression: expression '/' expression  */
#line 294 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("/", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1921 "tshade.cpp"
    break;

  case 47: /* expression: expression OP_DIV_ASS expression  */
#line 296 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("/=", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1927 "tshade.cpp"
    break;

  case 48: /* expression: expression '%' expression  */
#line 298 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("%", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1933 "tshade.cpp"
    break;

  case 49: /* expression: expression OP_MOD_ASS expression  */
#line 300 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("%=", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1939 "tshade.cpp"
    break;

  case 50: /* expression: expression OP_EQ expression  */
#line 302 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("==", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1945 "tshade.cpp"
    break;

  case 51: /* expression: expression OP_NEQ expression  */
#line 304 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("!=", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1951 "tshade.cpp"
    break;

  case 52: /* expression: expression '<' expression  */
#line 306 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("<", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1957 "tshade.cpp"
    break;

  case 53: /* expression: expression '>' expression  */
#line 308 "tshade.y"
    { (yyval.node) = new tBinaryOpNode(">", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1963 "tshade.cpp"
    break;

  case 54: /* expression: expression OP_GE expression  */
#line 310 "tshade.y"
    { (yyval.node) = new tBinaryOpNode(">=", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1969 "tshade.cpp"
    break;

  case 55: /* expression: expression OP_AND expression  */
#line 312 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("&&", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1975 "tshade.cpp"
    break;

  case 56: /* expression: expression OP_OR expression  */
#line 314 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("||", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1981 "tshade.cpp"
    break;

  case 57: /* expression: '!' expression  */
#line 316 "tshade.y"
    { (yyval.node) = new tUnaryOpNode("!", (yyvsp[0].node)); }
#line 1987 "tshade.cpp"
    break;

  case 58: /* expression: '-' expression  */
#line 318 "tshade.y"
    { (yyval.node) = new tUnaryOpNode("-", (yyvsp[0].node)); }
#line 1993 "tshade.cpp"
    break;

  case 59: /* expression: OP_PLUSPLUS expression  */
#line 320 "tshade.y"
    { (yyval.node) = new tUnaryOpNode("++", (yyvsp[0].node)); }
#line 1999 "tshade.cpp"
    break;

  case 60: /* expression: OP_MINUSMINUS expression  */
#line 322 "tshade.y"
    { (yyval.node) = new tUnaryOpNode("--", (yyvsp[0].node)); }
#line 2005 "tshade.cpp"
    break;

  case 61: /* expression: expression OP_PLUSPLUS  */
#line 324 "tshade.y"
    { (yyval.node) = new tUnaryOpNode("++", (yyvsp[-1].node), false); }
#line 2011 "tshade.cpp"
    break;

  case 62: /* expression: expression OP_MINUSMINUS  */
#line 326 "tshade.y"
    { (yyval.node) = new tUnaryOpNode("--", (yyvsp[-1].node), false); }
#line 2017 "tshade.cpp"
    break;

  case 63: /* expression: '(' expression ')'  */
#line 328 "tshade.y"
    { (yyval.node) = (yyvsp[-1].node); }
#line 2023 "tshade.cpp"
    break;

  case 64: /* expression: var_type '(' expression_list ')'  */
#line 330 "tshade.y"
    { (yyval.node) = new tTypeRefNode((yyvsp[-3].varType), (yyvsp[-1].exprListnode)); }
#line 2029 "tshade.cpp"
    break;

  case 65: /* expression: VAR_IDENT '(' expression_list ')'  */
#line 332 "tshade.y"
    {
      tFunctionNode* funcDecl = shadeAst->findFunction((yyvsp[-3].strVal));
      if (funcDecl) {
          (yyval.node) = new tFunctionRefNode(funcDecl, (yyvsp[-1].exprListnode));
      } else {
          yyerror(scanner, shadeAst, "Undefined function");
          (yyval.node) = nullptr;  // Handle error appropriately
      }
    }
#line 2043 "tshade.cpp"
    break;

  case 66: /* expression: VAR_IDENT tSWIZZLE  */
#line 342 "tshade.y"
    { tVarDeclNode* varDecl = shadeAst->findVar((yyvsp[-1].strVal));
      if (varDecl) {
          (yyval.node) = new tVarRefNode(varDecl, (yyvsp[0].strVal));
      } else {
          yyerror(scanner, shadeAst, "Undefined variable");
          (yyval.node) = nullptr;  // Handle error appropriately
      } 
    }
#line 2056 "tshade.cpp"
    break;

  case 67: /* expression: VAR_IDENT  */
#line 351 "tshade.y"
    { tVarDeclNode* varDecl = shadeAst->findVar((yyvsp[0].strVal));
      if (varDecl) {
          (yyval.node) = new tVarRefNode(varDecl);
      } else {
          yyerror(scanner, shadeAst, "Undefined variable");
          (yyval.node) = nullptr;  // Handle error appropriately
      } 
    }
#line 2069 "tshade.cpp"
    break;

  case 68: /* expression: VAR_IDENT '.' VAR_IDENT  */
#line 360 "tshade.y"
    { tVarDeclNode* varDecl = shadeAst->findVar((yyvsp[-2].strVal));
      if (varDecl->isStruct) {
          (yyval.node) = new tVarRefNode(varDecl);
      } else {
          yyerror(scanner, shadeAst, "Unknown member");
          (yyval.node) = nullptr;  // Handle error appropriately
      } 
    }
#line 2082 "tshade.cpp"
    break;

  case 69: /* expression: INT_NUM  */
#line 369 "tshade.y"
    { (yyval.node) = new tIntLiteralNode((yyvsp[0].intVal)); }
#line 2088 "tshade.cpp"
    break;

  case 70: /* expression: FLOAT_NUM  */
#line 371 "tshade.y"
    { (yyval.node) = new tFloatLiteralNode((yyvsp[0].fVal)); }
#line 2094 "tshade.cpp"
    break;

  case 71: /* statement_list: %empty  */
#line 376 "tshade.y"
    { (yyval.stmt_list_node) = new tStatementListNode(); }
#line 2100 "tshade.cpp"
    break;

  case 72: /* statement_list: statement_list statement  */
#line 378 "tshade.y"
    { (yyvsp[-1].stmt_list_node)->addStatement((yyvsp[0].node)); (yyval.stmt_list_node) = (yyvsp[-1].stmt_list_node); }
#line 2106 "tshade.cpp"
    break;

  case 73: /* statement: var_decl  */
#line 383 "tshade.y"
    {(yyval.node) = (yyvsp[0].declNode);}
#line 2112 "tshade.cpp"
    break;

  case 74: /* statement: uniform_decl  */
#line 385 "tshade.y"
    {(yyval.node) = (yyvsp[0].declNode);}
#line 2118 "tshade.cpp"
    break;

  case 75: /* statement: static_const_decl  */
#line 387 "tshade.y"
    {(yyval.node) = (yyvsp[0].declNode);}
#line 2124 "tshade.cpp"
    break;

  case 76: /* statement: if_statement  */
#line 389 "tshade.y"
    {(yyval.node) = (yyvsp[0].node);}
#line 2130 "tshade.cpp"
    break;

  case 77: /* statement: while_statement  */
#line 391 "tshade.y"
    {(yyval.node) = (yyvsp[0].node);}
#line 2136 "tshade.cpp"
    break;

  case 78: /* statement: continue_statement  */
#line 393 "tshade.y"
    {(yyval.node) = (yyvsp[0].node);}
#line 2142 "tshade.cpp"
    break;

  case 79: /* statement: break_statement  */
#line 395 "tshade.y"
    {(yyval.node) = (yyvsp[0].node);}
#line 2148 "tshade.cpp"
    break;

  case 80: /* statement: return_statement  */
#line 397 "tshade.y"
    {(yyval.node) = (yyvsp[0].node);}
#line 2154 "tshade.cpp"
    break;

  case 81: /* statement: function_def  */
#line 399 "tshade.y"
    {(yyval.node) = (yyvsp[0].funcNode); }
#line 2160 "tshade.cpp"
    break;

  case 82: /* statement: expression ';'  */
#line 401 "tshade.y"
    {(yyval.node) = (yyvsp[-1].node);}
#line 2166 "tshade.cpp"
    break;

  case 83: /* statement: switch_statement  */
#line 403 "tshade.y"
    {(yyval.node) = (yyvsp[0].node);}
#line 2172 "tshade.cpp"
    break;

  case 84: /* statement: discard_statement  */
#line 405 "tshade.y"
    {(yyval.node) = (yyvsp[0].node);}
#line 2178 "tshade.cpp"
    break;

  case 85: /* if_statement: rwIF '(' expression ')' '{' statement_list '}' rwELSE '{' statement_list '}'  */
#line 411 "tshade.y"
    { (yyval.node) = new tIfNode((yyvsp[-8].node), (yyvsp[-5].stmt_list_node), (yyvsp[-1].stmt_list_node)); }
#line 2184 "tshade.cpp"
    break;

  case 86: /* if_statement: rwIF '(' expression ')' '{' statement_list '}'  */
#line 413 "tshade.y"
    { (yyval.node) = new tIfNode((yyvsp[-4].node), (yyvsp[-1].stmt_list_node)); }
#line 2190 "tshade.cpp"
    break;

  case 87: /* while_statement: rwWHILE '(' expression ')' '{' statement_list '}'  */
#line 418 "tshade.y"
    { (yyval.node) = new tWhileNode((yyvsp[-4].node), (yyvsp[-1].stmt_list_node)); }
#line 2196 "tshade.cpp"
    break;

  case 88: /* while_statement: rwDO '{' statement_list '}' rwWHILE '(' expression ')'  */
#line 420 "tshade.y"
    { (yyval.node) = new tWhileNode((yyvsp[-1].node), (yyvsp[-5].stmt_list_node), true); }
#line 2202 "tshade.cpp"
    break;

  case 89: /* switch_statement: rwSWITCH '(' expression ')' '{' case_statements '}'  */
#line 425 "tshade.y"
    {(yyval.node) = new tSwitchNode((yyvsp[-4].node), (yyvsp[-1].stmt_list_node)); }
#line 2208 "tshade.cpp"
    break;

  case 90: /* case_statements: %empty  */
#line 430 "tshade.y"
    { (yyval.stmt_list_node) = new tStatementListNode(); }
#line 2214 "tshade.cpp"
    break;

  case 91: /* case_statements: case_statements case_rule  */
#line 432 "tshade.y"
    { (yyvsp[-1].stmt_list_node)->addStatement((yyvsp[0].node)); (yyval.stmt_list_node) = (yyvsp[-1].stmt_list_node); }
#line 2220 "tshade.cpp"
    break;

  case 92: /* case_rule: rwCASE expression ':' statement_list  */
#line 437 "tshade.y"
    {(yyval.node) = new tCaseNode((yyvsp[-2].node), (yyvsp[0].stmt_list_node));}
#line 2226 "tshade.cpp"
    break;

  case 93: /* case_rule: rwDEFAULT ':' statement_list  */
#line 439 "tshade.y"
    { (yyval.node) = new tCaseNode(nullptr, (yyvsp[0].stmt_list_node), true); }
#line 2232 "tshade.cpp"
    break;

  case 94: /* continue_statement: rwCONTINUE ';'  */
#line 444 "tshade.y"
    { (yyval.node) = new tContinueNode(); }
#line 2238 "tshade.cpp"
    break;

  case 95: /* break_statement: rwBREAK ';'  */
#line 449 "tshade.y"
    { (yyval.node) = new tBreakNode(); }
#line 2244 "tshade.cpp"
    break;

  case 96: /* return_statement: rwRETURN ';'  */
#line 454 "tshade.y"
    { (yyval.node) = new tReturnNode(); }
#line 2250 "tshade.cpp"
    break;

  case 97: /* return_statement: rwRETURN expression ';'  */
#line 456 "tshade.y"
    { (yyval.node) = new tReturnNode((yyvsp[-1].node)); }
#line 2256 "tshade.cpp"
    break;

  case 98: /* discard_statement: rwDISCARD ';'  */
#line 461 "tshade.y"
    { (yyval.node) = new tDiscardNode(); }
#line 2262 "tshade.cpp"
    break;

  case 99: /* var_type: tMAT34_TYPE  */
#line 466 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_MAT34;}
#line 2268 "tshade.cpp"
    break;

  case 100: /* var_type: tMAT43_TYPE  */
#line 468 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_MAT43;}
#line 2274 "tshade.cpp"
    break;

  case 101: /* var_type: tMAT3_TYPE  */
#line 470 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_MAT33;}
#line 2280 "tshade.cpp"
    break;

  case 102: /* var_type: tMAT4_TYPE  */
#line 472 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_MAT44;}
#line 2286 "tshade.cpp"
    break;

  case 103: /* var_type: tFVEC2_TYPE  */
#line 474 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_FLOAT2;}
#line 2292 "tshade.cpp"
    break;

  case 104: /* var_type: tFVEC3_TYPE  */
#line 476 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_FLOAT3;}
#line 2298 "tshade.cpp"
    break;

  case 105: /* var_type: tFVEC4_TYPE  */
#line 478 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_FLOAT4;}
#line 2304 "tshade.cpp"
    break;

  case 106: /* var_type: tIVEC2_TYPE  */
#line 480 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_INT2;}
#line 2310 "tshade.cpp"
    break;

  case 107: /* var_type: tIVEC3_TYPE  */
#line 482 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_INT3;}
#line 2316 "tshade.cpp"
    break;

  case 108: /* var_type: tIVEC4_TYPE  */
#line 484 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_INT4;}
#line 2322 "tshade.cpp"
    break;

  case 109: /* var_type: tBVEC2_TYPE  */
#line 486 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_BOOL2;}
#line 2328 "tshade.cpp"
    break;

  case 110: /* var_type: tBVEC3_TYPE  */
#line 488 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_BOOL3;}
#line 2334 "tshade.cpp"
    break;

  case 111: /* var_type: tBVEC4_TYPE  */
#line 490 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_BOOL4;}
#line 2340 "tshade.cpp"
    break;

  case 112: /* var_type: tFLOAT_TYPE  */
#line 492 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_FLOAT;}
#line 2346 "tshade.cpp"
    break;

  case 113: /* var_type: tINT_TYPE  */
#line 494 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_INT;}
#line 2352 "tshade.cpp"
    break;

  case 114: /* var_type: tUINT_TYPE  */
#line 496 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_UINT;}
#line 2358 "tshade.cpp"
    break;

  case 115: /* var_type: tBOOL_TYPE  */
#line 498 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_BOOL;}
#line 2364 "tshade.cpp"
    break;

  case 116: /* var_type: rwVOID  */
#line 500 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_VOID;}
#line 2370 "tshade.cpp"
    break;

  case 117: /* struct_semantic: tSEM_SVPOSITION  */
#line 505 "tshade.y"
    { (yyval.semtype) = SEMANTIC_SV_POSITION; }
#line 2376 "tshade.cpp"
    break;

  case 118: /* struct_semantic: tSEM_POSITION  */
#line 507 "tshade.y"
    { (yyval.semtype) = SEMANTIC_POSITION; }
#line 2382 "tshade.cpp"
    break;

  case 119: /* struct_semantic: tSEM_NORMAL  */
#line 509 "tshade.y"
    { (yyval.semtype) = SEMANTIC_NORMAL; }
#line 2388 "tshade.cpp"
    break;

  case 120: /* struct_semantic: tSEM_BINORMAL  */
#line 511 "tshade.y"
    { (yyval.semtype) = SEMANTIC_BINORMAL; }
#line 2394 "tshade.cpp"
    break;

  case 121: /* struct_semantic: tSEM_TANGENT  */
#line 513 "tshade.y"
    { (yyval.semtype) = SEMANTIC_TANGENT; }
#line 2400 "tshade.cpp"
    break;

  case 122: /* struct_semantic: tSEM_TEXCOORD  */
#line 515 "tshade.y"
    { (yyval.semtype) = SEMANTIC_TEXCOORD; }
#line 2406 "tshade.cpp"
    break;

  case 123: /* struct_semantic: tSEM_COLOR  */
#line 517 "tshade.y"
   { (yyval.semtype) = SEMANTIC_COLOR; }
#line 2412 "tshade.cpp"
    break;

  case 124: /* struct_semantic: tSEM_TARGET  */
#line 519 "tshade.y"
    { (yyval.semtype) = SEMANTIC_SV_TARGET; }
#line 2418 "tshade.cpp"
    break;

  case 125: /* struct_semantic: tSEM_DEPTH  */
#line 521 "tshade.y"
    { (yyval.semtype) = SEMANTIC_SV_DEPTH; }
#line 2424 "tshade.cpp"
    break;

  case 126: /* struct_semantic: tSEM_ISFRONTFACE  */
#line 523 "tshade.y"
    { (yyval.semtype) = SEMANTIC_ISFRONTFACE; }
#line 2430 "tshade.cpp"
    break;

  case 127: /* struct_semantic: tSEM_TESSFACTOR  */
#line 525 "tshade.y"
    { (yyval.semtype) = SEMANTIC_TESSFACTOR; }
#line 2436 "tshade.cpp"
    break;

  case 128: /* struct_semantic: tSEM_PSIZE  */
#line 527 "tshade.y"
    { (yyval.semtype) = SEMANTIC_PSIZE; }
#line 2442 "tshade.cpp"
    break;


#line 2446 "tshade.cpp"

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
      yyerror (scanner, shadeAst, YY_("syntax error"));
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

#line 531 "tshade.y"


void yyerror(yyscan_t yyscanner, const char* msg){
    Con::errorf("TorqueShader ERROR: %s Line: %d", msg, yylineno);
}

void yyerror(yyscan_t yyscanner, tShadeAst* shadeAst, char const *msg) {
	yyerror(yyscanner, msg);
}
