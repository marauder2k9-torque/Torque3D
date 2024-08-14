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

  extern int yylineno;

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
  YYSYMBOL_OP_EQ = 45,                     /* OP_EQ  */
  YYSYMBOL_OP_NEQ = 46,                    /* OP_NEQ  */
  YYSYMBOL_OP_AND = 47,                    /* OP_AND  */
  YYSYMBOL_OP_OR = 48,                     /* OP_OR  */
  YYSYMBOL_OP_LE = 49,                     /* OP_LE  */
  YYSYMBOL_OP_GE = 50,                     /* OP_GE  */
  YYSYMBOL_OP_PLUSPLUS = 51,               /* OP_PLUSPLUS  */
  YYSYMBOL_OP_MINUSMINUS = 52,             /* OP_MINUSMINUS  */
  YYSYMBOL_OP_PLUS_ASS = 53,               /* OP_PLUS_ASS  */
  YYSYMBOL_OP_MINUS_ASS = 54,              /* OP_MINUS_ASS  */
  YYSYMBOL_OP_MUL_ASS = 55,                /* OP_MUL_ASS  */
  YYSYMBOL_OP_DIV_ASS = 56,                /* OP_DIV_ASS  */
  YYSYMBOL_OP_MOD_ASS = 57,                /* OP_MOD_ASS  */
  YYSYMBOL_OP_AND_ASS = 58,                /* OP_AND_ASS  */
  YYSYMBOL_OP_OR_ASS = 59,                 /* OP_OR_ASS  */
  YYSYMBOL_OP_XOR_ASS = 60,                /* OP_XOR_ASS  */
  YYSYMBOL_OP_BIT_LEFT_ASS = 61,           /* OP_BIT_LEFT_ASS  */
  YYSYMBOL_OP_BIT_RIGHT_ASS = 62,          /* OP_BIT_RIGHT_ASS  */
  YYSYMBOL_OP_BIT_LEFT = 63,               /* OP_BIT_LEFT  */
  YYSYMBOL_OP_BIT_RIGHT = 64,              /* OP_BIT_RIGHT  */
  YYSYMBOL_INT_NUM = 65,                   /* INT_NUM  */
  YYSYMBOL_FLOAT_NUM = 66,                 /* FLOAT_NUM  */
  YYSYMBOL_VAR_IDENT = 67,                 /* VAR_IDENT  */
  YYSYMBOL_STR_VAL = 68,                   /* STR_VAL  */
  YYSYMBOL_TYPE_IDENT = 69,                /* TYPE_IDENT  */
  YYSYMBOL_tSWIZZLE = 70,                  /* tSWIZZLE  */
  YYSYMBOL_tSTRUCT = 71,                   /* tSTRUCT  */
  YYSYMBOL_tUNIFORM = 72,                  /* tUNIFORM  */
  YYSYMBOL_tCBUFFER = 73,                  /* tCBUFFER  */
  YYSYMBOL_tSHADERDECLARE = 74,            /* tSHADERDECLARE  */
  YYSYMBOL_tVSSHADER = 75,                 /* tVSSHADER  */
  YYSYMBOL_tPSSHADER = 76,                 /* tPSSHADER  */
  YYSYMBOL_tGSSHADER = 77,                 /* tGSSHADER  */
  YYSYMBOL_tCSSHADER = 78,                 /* tCSSHADER  */
  YYSYMBOL_tDSSHADER = 79,                 /* tDSSHADER  */
  YYSYMBOL_tHSSHADER = 80,                 /* tHSSHADER  */
  YYSYMBOL_tFLOAT_TYPE = 81,               /* tFLOAT_TYPE  */
  YYSYMBOL_tINT_TYPE = 82,                 /* tINT_TYPE  */
  YYSYMBOL_tBOOL_TYPE = 83,                /* tBOOL_TYPE  */
  YYSYMBOL_tUINT_TYPE = 84,                /* tUINT_TYPE  */
  YYSYMBOL_tFVEC2_TYPE = 85,               /* tFVEC2_TYPE  */
  YYSYMBOL_tFVEC3_TYPE = 86,               /* tFVEC3_TYPE  */
  YYSYMBOL_tFVEC4_TYPE = 87,               /* tFVEC4_TYPE  */
  YYSYMBOL_tIVEC2_TYPE = 88,               /* tIVEC2_TYPE  */
  YYSYMBOL_tIVEC3_TYPE = 89,               /* tIVEC3_TYPE  */
  YYSYMBOL_tIVEC4_TYPE = 90,               /* tIVEC4_TYPE  */
  YYSYMBOL_tBVEC2_TYPE = 91,               /* tBVEC2_TYPE  */
  YYSYMBOL_tBVEC3_TYPE = 92,               /* tBVEC3_TYPE  */
  YYSYMBOL_tBVEC4_TYPE = 93,               /* tBVEC4_TYPE  */
  YYSYMBOL_tMAT4_TYPE = 94,                /* tMAT4_TYPE  */
  YYSYMBOL_tMAT43_TYPE = 95,               /* tMAT43_TYPE  */
  YYSYMBOL_tMAT34_TYPE = 96,               /* tMAT34_TYPE  */
  YYSYMBOL_tMAT3_TYPE = 97,                /* tMAT3_TYPE  */
  YYSYMBOL_98_ = 98,                       /* '['  */
  YYSYMBOL_99_ = 99,                       /* ']'  */
  YYSYMBOL_YYACCEPT = 100,                 /* $accept  */
  YYSYMBOL_program = 101,                  /* program  */
  YYSYMBOL_program_globals = 102,          /* program_globals  */
  YYSYMBOL_shader_stage = 103,             /* shader_stage  */
  YYSYMBOL_shader_body = 104,              /* shader_body  */
  YYSYMBOL_uniform_decl = 105,             /* uniform_decl  */
  YYSYMBOL_var_decl = 106,                 /* var_decl  */
  YYSYMBOL_param_modifier = 107,           /* param_modifier  */
  YYSYMBOL_function_def = 108,             /* function_def  */
  YYSYMBOL_function_param_list = 109,      /* function_param_list  */
  YYSYMBOL_function_param = 110,           /* function_param  */
  YYSYMBOL_expression_list = 111,          /* expression_list  */
  YYSYMBOL_expression = 112,               /* expression  */
  YYSYMBOL_var_type = 113,                 /* var_type  */
  YYSYMBOL_statement_list = 114,           /* statement_list  */
  YYSYMBOL_statement = 115,                /* statement  */
  YYSYMBOL_if_statement = 116,             /* if_statement  */
  YYSYMBOL_while_statement = 117,          /* while_statement  */
  YYSYMBOL_continue_statement = 118,       /* continue_statement  */
  YYSYMBOL_break_statement = 119,          /* break_statement  */
  YYSYMBOL_return_statement = 120          /* return_statement  */
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
#define YYLAST   829

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  100
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  21
/* YYNRULES -- Number of rules.  */
#define YYNRULES  86
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  182

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   330


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
       2,    98,     2,    99,    21,     2,     2,     2,     2,     2,
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
      97
};

#if TSHADE_DEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   115,   115,   116,   122,   123,   125,   130,   132,   134,
     136,   141,   146,   154,   156,   158,   160,   162,   167,   169,
     171,   176,   178,   184,   185,   187,   192,   194,   199,   201,
     206,   208,   210,   212,   214,   216,   218,   220,   222,   224,
     226,   228,   230,   232,   234,   236,   238,   248,   257,   266,
     275,   277,   282,   284,   286,   288,   290,   292,   294,   296,
     298,   300,   302,   304,   306,   308,   310,   312,   314,   320,
     321,   326,   328,   330,   332,   334,   336,   338,   340,   345,
     347,   352,   354,   359,   364,   369,   371
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
  "OP_EQ", "OP_NEQ", "OP_AND", "OP_OR", "OP_LE", "OP_GE", "OP_PLUSPLUS",
  "OP_MINUSMINUS", "OP_PLUS_ASS", "OP_MINUS_ASS", "OP_MUL_ASS",
  "OP_DIV_ASS", "OP_MOD_ASS", "OP_AND_ASS", "OP_OR_ASS", "OP_XOR_ASS",
  "OP_BIT_LEFT_ASS", "OP_BIT_RIGHT_ASS", "OP_BIT_LEFT", "OP_BIT_RIGHT",
  "INT_NUM", "FLOAT_NUM", "VAR_IDENT", "STR_VAL", "TYPE_IDENT", "tSWIZZLE",
  "tSTRUCT", "tUNIFORM", "tCBUFFER", "tSHADERDECLARE", "tVSSHADER",
  "tPSSHADER", "tGSSHADER", "tCSSHADER", "tDSSHADER", "tHSSHADER",
  "tFLOAT_TYPE", "tINT_TYPE", "tBOOL_TYPE", "tUINT_TYPE", "tFVEC2_TYPE",
  "tFVEC3_TYPE", "tFVEC4_TYPE", "tIVEC2_TYPE", "tIVEC3_TYPE",
  "tIVEC4_TYPE", "tBVEC2_TYPE", "tBVEC3_TYPE", "tBVEC4_TYPE", "tMAT4_TYPE",
  "tMAT43_TYPE", "tMAT34_TYPE", "tMAT3_TYPE", "'['", "']'", "$accept",
  "program", "program_globals", "shader_stage", "shader_body",
  "uniform_decl", "var_decl", "param_modifier", "function_def",
  "function_param_list", "function_param", "expression_list", "expression",
  "var_type", "statement_list", "statement", "if_statement",
  "while_statement", "continue_statement", "break_statement",
  "return_statement", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-106)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     -64,   -51,    20,    12,  -106,   686,   -48,    14,    41,    48,
      51,  -106,  -106,  -106,  -106,  -106,  -106,  -106,  -106,  -106,
    -106,  -106,  -106,  -106,  -106,  -106,  -106,  -106,    52,  -106,
    -106,    22,    68,  -106,  -106,  -106,  -106,    72,    -3,  -106,
      73,   537,    74,    76,    77,  -106,   146,  -106,   146,  -106,
      78,    79,    82,    83,    88,    36,   112,   715,  -106,  -106,
    -106,    37,  -106,  -106,  -106,  -106,  -106,  -106,  -106,  -106,
    -106,   146,   146,   146,  -106,  -106,    -2,   135,    92,    75,
     146,  -106,  -106,  -106,   146,    94,  -106,   597,  -106,    -7,
    -106,   633,  -106,    43,   146,  -106,   146,   146,   146,   146,
     146,   146,   146,  -106,   146,   146,   146,   146,   146,   146,
       0,   646,   224,   659,   502,  -106,   502,  -106,  -106,    28,
     695,    29,    29,  -106,  -106,    71,    71,  -106,    19,    19,
     213,   707,    71,    31,    93,  -106,    95,    85,    96,  -106,
    -106,  -106,   732,    33,  -106,    50,    35,  -106,   146,  -106,
     146,  -106,   104,  -106,    57,   108,   502,  -106,   109,   695,
       1,   253,   146,   331,  -106,  -106,  -106,  -106,   111,  -106,
     682,    97,   360,   438,  -106,  -106,   113,  -106,  -106,  -106,
     467,  -106
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       2,     0,     0,     0,     1,     4,     0,     0,     0,     0,
       0,    65,    66,    68,    67,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    55,    53,    52,    54,     0,     6,
       5,     0,     0,    69,    69,    69,    69,     0,     0,    17,
       0,    11,     0,     0,     0,     3,     0,    13,     0,     7,
       0,     0,     0,     0,     0,     0,     0,     0,    72,    71,
      78,     0,    70,    73,    74,    75,    76,    77,     8,     9,
      10,     0,     0,     0,    50,    51,    48,     0,     0,     0,
       0,    69,    84,    83,     0,     0,    85,     0,    12,     0,
      43,     0,    42,     0,     0,    47,     0,     0,     0,     0,
       0,     0,     0,    14,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    23,    86,    23,    44,    49,     0,
      28,    30,    31,    32,    33,    37,    38,    34,    35,    36,
      40,    41,    39,     0,     0,    15,     0,     0,     0,    18,
      19,    20,     0,     0,    24,     0,     0,    46,     0,    45,
       0,    69,     0,    69,     0,     0,     0,    27,     0,    29,
       0,     0,     0,     0,    26,    69,    25,    69,     0,    81,
       0,    80,     0,     0,    16,    82,     0,    22,    21,    69,
       0,    79
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
    -106,  -106,  -106,  -106,     4,  -106,     9,  -106,  -106,    17,
     -22,  -105,   -43,    -5,   -80,  -106,  -106,  -106,  -106,  -106,
    -106
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,     2,    28,    29,    40,    58,    59,   142,    60,   143,
     144,   119,   120,    78,    41,    62,    63,    64,    65,    66,
      67
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      31,   112,    46,    77,   133,    79,    46,   116,    93,   134,
       1,    47,    94,    87,    30,    47,     3,   148,   135,    32,
       4,   168,    96,    97,    98,    99,   100,   101,    90,    91,
      92,     5,   102,    33,    98,    99,    61,   111,    42,    43,
      44,   113,   102,   147,   148,   160,   149,   148,   155,   156,
     158,   156,    31,   121,   122,   123,   124,   125,   126,   127,
      34,   128,   129,   130,   131,   132,    88,    35,    95,   108,
      36,   161,    37,   163,    96,    97,    98,    99,    96,    97,
      98,    99,   100,   101,   102,   172,    39,   173,   102,    38,
      45,    48,    80,    49,    68,    48,    69,    70,    81,   180,
      82,    83,    84,    85,    89,   159,   109,    61,   114,   145,
     118,   145,   150,   152,   151,   153,    71,   157,   162,   170,
     104,   105,   106,   107,   164,   108,    72,   165,   167,   174,
      86,   176,   179,   146,   166,    73,     0,   154,    96,    97,
      98,    99,   100,   101,     0,     0,     0,     0,   102,     0,
      71,   145,     0,   103,     0,     0,    61,     0,    61,     0,
      72,     0,     0,     0,     0,     0,     0,    61,    61,    73,
       0,     0,     0,     0,   110,    61,     0,    74,    75,    76,
     104,   105,   106,   107,     0,   108,     0,     0,     0,     0,
       0,     0,     0,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
       0,    74,    75,    76,     0,     0,    96,    97,    98,    99,
     100,   101,     0,     0,     0,     0,   102,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,   137,     0,     0,     0,     0,     0,
       0,     0,    50,    51,     0,    52,    53,    54,   104,   105,
      55,     0,     0,   108,     0,     0,     0,     0,    56,     0,
       0,     0,     0,   169,     0,     0,     0,     0,     0,     0,
       0,    50,    51,     0,    52,    53,    54,     0,     0,    55,
       0,     0,     0,     6,     0,     0,    57,    56,     0,     0,
       0,     0,     0,     0,     0,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,     6,     0,     0,    57,     0,     0,     0,     0,
       0,     0,     0,     0,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,   171,     0,     0,     0,     0,     0,     0,     0,    50,
      51,     0,    52,    53,    54,     0,     0,    55,     0,     0,
       0,     0,     0,     0,     0,    56,     0,     0,     0,     0,
     177,     0,     0,     0,     0,     0,     0,     0,    50,    51,
       0,    52,    53,    54,     0,     0,    55,     0,     0,     0,
       6,     0,     0,    57,    56,     0,     0,     0,     0,     0,
       0,     0,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,     6,
       0,     0,    57,     0,     0,     0,     0,     0,     0,     0,
       0,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,   178,     0,
       0,     0,     0,     0,     0,     0,    50,    51,     0,    52,
      53,    54,     0,     0,    55,     0,     0,     0,     0,     0,
       0,     0,    56,     0,     0,     0,     0,   181,     0,     0,
       0,     0,     0,     0,     0,    50,    51,     0,    52,    53,
      54,     0,     0,    55,     0,     0,     0,     6,     0,     0,
      57,    56,     0,     0,     0,     0,     0,     0,     0,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,     6,     0,     0,    57,
     139,   140,   141,     0,     0,     0,     0,     0,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    50,    51,     0,    52,    53,
      54,     0,     0,    55,     0,     0,     0,     0,     0,     0,
       0,    56,     0,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      96,    97,    98,    99,   100,   101,     6,     0,     0,    57,
     102,     0,     0,     0,     0,   115,     0,     0,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,     0,    96,    97,    98,    99,
     100,   101,   104,   105,   106,   107,   102,   108,   117,    96,
      97,    98,    99,   100,   101,     0,     0,     0,     0,   102,
       0,   136,    96,    97,    98,    99,   100,   101,     0,     0,
       0,     0,   102,     0,   138,     0,     0,     0,   104,   105,
     106,   107,     0,   108,     0,    96,    97,    98,    99,   100,
     101,   104,   105,   106,   107,   102,   108,   175,    96,    97,
      98,    99,   100,   101,   104,   105,   106,   107,   102,   108,
      96,    97,    98,    99,   100,   101,     0,     0,     0,     0,
     102,     0,     0,     0,     0,     0,     0,   104,   105,   106,
     107,     0,   108,     0,     0,     0,     0,     0,     0,     0,
     104,   105,   106,   107,     0,   108,     0,     0,     0,     0,
       0,     0,   104,   105,   106,     6,     0,   108,     0,     0,
       0,     7,     8,     9,    10,     0,     0,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,     6,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27
};

static const yytype_int16 yycheck[] =
{
       5,    81,     9,    46,   109,    48,     9,    14,    10,     9,
      74,    18,    14,    56,     5,    18,    67,    16,    18,    67,
       0,    20,     3,     4,     5,     6,     7,     8,    71,    72,
      73,    19,    13,    19,     5,     6,    41,    80,    34,    35,
      36,    84,    13,    15,    16,   150,    15,    16,    15,    16,
      15,    16,    57,    96,    97,    98,    99,   100,   101,   102,
      19,   104,   105,   106,   107,   108,    57,    19,    70,    50,
      19,   151,    20,   153,     3,     4,     5,     6,     3,     4,
       5,     6,     7,     8,    13,   165,    18,   167,    13,    67,
      18,    98,    14,    20,    20,    98,    20,    20,    19,   179,
      18,    18,    14,    67,    67,   148,    14,   112,    14,   114,
      67,   116,    19,    28,    19,    19,     4,    67,    14,   162,
      45,    46,    47,    48,    67,    50,    14,    19,    19,    18,
      18,    34,    19,   116,   156,    23,    -1,   142,     3,     4,
       5,     6,     7,     8,    -1,    -1,    -1,    -1,    13,    -1,
       4,   156,    -1,    18,    -1,    -1,   161,    -1,   163,    -1,
      14,    -1,    -1,    -1,    -1,    -1,    -1,   172,   173,    23,
      -1,    -1,    -1,    -1,    99,   180,    -1,    65,    66,    67,
      45,    46,    47,    48,    -1,    50,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      -1,    65,    66,    67,    -1,    -1,     3,     4,     5,     6,
       7,     8,    -1,    -1,    -1,    -1,    13,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    20,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    28,    29,    -1,    31,    32,    33,    45,    46,
      36,    -1,    -1,    50,    -1,    -1,    -1,    -1,    44,    -1,
      -1,    -1,    -1,    20,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    28,    29,    -1,    31,    32,    33,    -1,    -1,    36,
      -1,    -1,    -1,    69,    -1,    -1,    72,    44,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    69,    -1,    -1,    72,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    20,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    28,
      29,    -1,    31,    32,    33,    -1,    -1,    36,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    44,    -1,    -1,    -1,    -1,
      20,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    28,    29,
      -1,    31,    32,    33,    -1,    -1,    36,    -1,    -1,    -1,
      69,    -1,    -1,    72,    44,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    69,
      -1,    -1,    72,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    20,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    28,    29,    -1,    31,
      32,    33,    -1,    -1,    36,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    44,    -1,    -1,    -1,    -1,    20,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    28,    29,    -1,    31,    32,
      33,    -1,    -1,    36,    -1,    -1,    -1,    69,    -1,    -1,
      72,    44,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    69,    -1,    -1,    72,
      38,    39,    40,    -1,    -1,    -1,    -1,    -1,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    28,    29,    -1,    31,    32,
      33,    -1,    -1,    36,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    44,    -1,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
       3,     4,     5,     6,     7,     8,    69,    -1,    -1,    72,
      13,    -1,    -1,    -1,    -1,    18,    -1,    -1,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    -1,     3,     4,     5,     6,
       7,     8,    45,    46,    47,    48,    13,    50,    15,     3,
       4,     5,     6,     7,     8,    -1,    -1,    -1,    -1,    13,
      -1,    15,     3,     4,     5,     6,     7,     8,    -1,    -1,
      -1,    -1,    13,    -1,    15,    -1,    -1,    -1,    45,    46,
      47,    48,    -1,    50,    -1,     3,     4,     5,     6,     7,
       8,    45,    46,    47,    48,    13,    50,    15,     3,     4,
       5,     6,     7,     8,    45,    46,    47,    48,    13,    50,
       3,     4,     5,     6,     7,     8,    -1,    -1,    -1,    -1,
      13,    -1,    -1,    -1,    -1,    -1,    -1,    45,    46,    47,
      48,    -1,    50,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      45,    46,    47,    48,    -1,    50,    -1,    -1,    -1,    -1,
      -1,    -1,    45,    46,    47,    69,    -1,    50,    -1,    -1,
      -1,    75,    76,    77,    78,    -1,    -1,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    69,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,    74,   101,    67,     0,    19,    69,    75,    76,    77,
      78,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,   102,   103,
     106,   113,    67,    19,    19,    19,    19,    20,    67,    18,
     104,   114,   104,   104,   104,    18,     9,    18,    98,    20,
      28,    29,    31,    32,    33,    36,    44,    72,   105,   106,
     108,   113,   115,   116,   117,   118,   119,   120,    20,    20,
      20,     4,    14,    23,    65,    66,    67,   112,   113,   112,
      14,    19,    18,    18,    14,    67,    18,   112,   106,    67,
     112,   112,   112,    10,    14,    70,     3,     4,     5,     6,
       7,     8,    13,    18,    45,    46,    47,    48,    50,    14,
      99,   112,   114,   112,    14,    18,    14,    15,    67,   111,
     112,   112,   112,   112,   112,   112,   112,   112,   112,   112,
     112,   112,   112,   111,     9,    18,    15,    20,    15,    38,
      39,    40,   107,   109,   110,   113,   109,    15,    16,    15,
      19,    19,    28,    19,   113,    15,    16,    67,    15,   112,
     111,   114,    14,   114,    67,    19,   110,    19,    20,    20,
     112,    20,   114,   114,    18,    15,    34,    20,    20,    19,
     114,    20
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,   100,   101,   101,   102,   102,   102,   103,   103,   103,
     103,   104,   105,   106,   106,   106,   106,   106,   107,   107,
     107,   108,   108,   109,   109,   109,   110,   110,   111,   111,
     112,   112,   112,   112,   112,   112,   112,   112,   112,   112,
     112,   112,   112,   112,   112,   112,   112,   112,   112,   112,
     112,   112,   113,   113,   113,   113,   113,   113,   113,   113,
     113,   113,   113,   113,   113,   113,   113,   113,   113,   114,
     114,   115,   115,   115,   115,   115,   115,   115,   115,   116,
     116,   117,   117,   118,   119,   120,   120
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     6,     0,     1,     1,     4,     4,     4,
       4,     1,     2,     3,     5,     6,    10,     3,     1,     1,
       1,     8,     8,     0,     1,     3,     3,     2,     1,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     2,     2,     3,     4,     4,     2,     1,     3,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     0,
       2,     1,     1,     1,     1,     1,     1,     1,     1,    11,
       7,     7,     8,     2,     2,     2,     3
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
#line 115 "tshade.y"
    { (yyval.node) = nullptr;}
#line 1474 "tshade.cpp"
    break;

  case 3: /* program: tSHADERDECLARE VAR_IDENT '{' program_globals '}' ';'  */
#line 117 "tshade.y"
    {(yyval.node) = nullptr; shadeAst->shaderName = (yyvsp[-4].strVal); }
#line 1480 "tshade.cpp"
    break;

  case 4: /* program_globals: %empty  */
#line 122 "tshade.y"
    {(yyval.node) = nullptr; }
#line 1486 "tshade.cpp"
    break;

  case 5: /* program_globals: var_decl  */
#line 124 "tshade.y"
    {(yyval.node) = nullptr; shadeAst->mGlobalVars.push_back((yyvsp[0].declNode)); }
#line 1492 "tshade.cpp"
    break;

  case 6: /* program_globals: shader_stage  */
#line 126 "tshade.y"
    {}
#line 1498 "tshade.cpp"
    break;

  case 7: /* shader_stage: tVSSHADER '{' shader_body '}'  */
#line 131 "tshade.y"
    { shadeAst->mVertStage = new tStageNode(ShaderStageType::tSTAGE_VERTEX, (yyvsp[-1].node)); }
#line 1504 "tshade.cpp"
    break;

  case 8: /* shader_stage: tPSSHADER '{' shader_body '}'  */
#line 133 "tshade.y"
    {shadeAst->mPixStage = new tStageNode(ShaderStageType::tSTAGE_PIXEL, (yyvsp[-1].node));}
#line 1510 "tshade.cpp"
    break;

  case 9: /* shader_stage: tGSSHADER '{' shader_body '}'  */
#line 135 "tshade.y"
    {shadeAst->mPixStage = new tStageNode(ShaderStageType::tSTAGE_GEOMETRY, (yyvsp[-1].node));}
#line 1516 "tshade.cpp"
    break;

  case 10: /* shader_stage: tCSSHADER '{' shader_body '}'  */
#line 137 "tshade.y"
    {shadeAst->mPixStage = new tStageNode(ShaderStageType::tSTAGE_COMPUTE, (yyvsp[-1].node));}
#line 1522 "tshade.cpp"
    break;

  case 11: /* shader_body: statement_list  */
#line 142 "tshade.y"
    {(yyval.node) = (yyvsp[0].stmt_list_node);}
#line 1528 "tshade.cpp"
    break;

  case 12: /* uniform_decl: tUNIFORM var_decl  */
#line 147 "tshade.y"
  {
    (yyvsp[0].declNode)->isUniform = true;
    (yyval.declNode) = (yyvsp[0].declNode);
  }
#line 1537 "tshade.cpp"
    break;

  case 13: /* var_decl: var_type VAR_IDENT ';'  */
#line 155 "tshade.y"
    {(yyval.declNode) = new tVarDeclNode((yyvsp[-1].strVal), (yyvsp[-2].varType)); shadeAst->addVarDecl((yyval.declNode)); }
#line 1543 "tshade.cpp"
    break;

  case 14: /* var_decl: var_type VAR_IDENT '=' expression ';'  */
#line 157 "tshade.y"
    {(yyval.declNode) = new tVarDeclNode((yyvsp[-3].strVal), (yyvsp[-4].varType), (yyvsp[-1].node)); shadeAst->addVarDecl((yyval.declNode)); }
#line 1549 "tshade.cpp"
    break;

  case 15: /* var_decl: var_type VAR_IDENT '[' expression ']' ';'  */
#line 159 "tshade.y"
    {(yyval.declNode) = new tVarDeclNode((yyvsp[-4].strVal), (yyvsp[-5].varType), nullptr, (yyvsp[-2].node)); shadeAst->addVarDecl((yyval.declNode)); }
#line 1555 "tshade.cpp"
    break;

  case 16: /* var_decl: var_type VAR_IDENT '[' expression ']' '=' '{' expression_list '}' ';'  */
#line 161 "tshade.y"
    {(yyval.declNode) = new tVarDeclNode((yyvsp[-8].strVal), (yyvsp[-9].varType), (yyvsp[-2].exprListnode), (yyvsp[-6].node)); shadeAst->addVarDecl((yyval.declNode)); }
#line 1561 "tshade.cpp"
    break;

  case 17: /* var_decl: TYPE_IDENT VAR_IDENT ';'  */
#line 163 "tshade.y"
    {(yyval.declNode) = new tVarDeclNode((yyvsp[-1].strVal), ShaderVarType::tTYPE_STRUCT, nullptr, nullptr, true); shadeAst->addVarDecl((yyval.declNode)); }
#line 1567 "tshade.cpp"
    break;

  case 18: /* param_modifier: rwIN  */
#line 168 "tshade.y"
    { (yyval.modifier) = ParamModifier::PARAM_MOD_IN; }
#line 1573 "tshade.cpp"
    break;

  case 19: /* param_modifier: rwOUT  */
#line 170 "tshade.y"
    { (yyval.modifier) = ParamModifier::PARAM_MOD_OUT; }
#line 1579 "tshade.cpp"
    break;

  case 20: /* param_modifier: rwINOUT  */
#line 172 "tshade.y"
    { (yyval.modifier) = ParamModifier::PARAM_MOD_INOUT; }
#line 1585 "tshade.cpp"
    break;

  case 21: /* function_def: var_type VAR_IDENT '(' function_param_list ')' '{' statement_list '}'  */
#line 177 "tshade.y"
    { (yyval.funcNode) = new tFunctionNode((yyvsp[-6].strVal), (yyvsp[-7].varType), (yyvsp[-4].funcList), (yyvsp[-1].stmt_list_node)); shadeAst->addfunction((yyval.funcNode)); }
#line 1591 "tshade.cpp"
    break;

  case 22: /* function_def: rwVOID VAR_IDENT '(' function_param_list ')' '{' statement_list '}'  */
#line 179 "tshade.y"
    { (yyval.funcNode) = new tFunctionNode((yyvsp[-6].strVal), ShaderVarType::tTYPE_VOID, (yyvsp[-4].funcList), (yyvsp[-1].stmt_list_node)); shadeAst->addfunction((yyval.funcNode));}
#line 1597 "tshade.cpp"
    break;

  case 23: /* function_param_list: %empty  */
#line 184 "tshade.y"
    { (yyval.funcList) = new tFunctionParamListNode(); }
#line 1603 "tshade.cpp"
    break;

  case 24: /* function_param_list: function_param  */
#line 186 "tshade.y"
    { (yyval.funcList) = new tFunctionParamListNode(); (yyval.funcList)->addParam((yyvsp[0].funcParam)); }
#line 1609 "tshade.cpp"
    break;

  case 25: /* function_param_list: function_param_list ',' function_param  */
#line 188 "tshade.y"
    {(yyvsp[-2].funcList)->addParam((yyvsp[0].funcParam)); (yyval.funcList) = (yyvsp[-2].funcList); }
#line 1615 "tshade.cpp"
    break;

  case 26: /* function_param: param_modifier var_type VAR_IDENT  */
#line 193 "tshade.y"
    { (yyval.funcParam) = new tFunctionParamNode((yyvsp[0].strVal), (yyvsp[-1].varType), (yyvsp[-2].modifier)); }
#line 1621 "tshade.cpp"
    break;

  case 27: /* function_param: var_type VAR_IDENT  */
#line 195 "tshade.y"
    { (yyval.funcParam) = new tFunctionParamNode((yyvsp[0].strVal), (yyvsp[-1].varType), ParamModifier::PARAM_MOD_NONE); }
#line 1627 "tshade.cpp"
    break;

  case 28: /* expression_list: expression  */
#line 200 "tshade.y"
    { (yyval.exprListnode) = new tExpressionListNode(); (yyval.exprListnode)->addExpression((yyvsp[0].node)); }
#line 1633 "tshade.cpp"
    break;

  case 29: /* expression_list: expression_list ',' expression  */
#line 202 "tshade.y"
    { (yyval.exprListnode)->addExpression((yyvsp[0].node)); (yyval.exprListnode) = (yyvsp[-2].exprListnode); }
#line 1639 "tshade.cpp"
    break;

  case 30: /* expression: expression '+' expression  */
#line 207 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("+", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1645 "tshade.cpp"
    break;

  case 31: /* expression: expression '-' expression  */
#line 209 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("-", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1651 "tshade.cpp"
    break;

  case 32: /* expression: expression '*' expression  */
#line 211 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("*", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1657 "tshade.cpp"
    break;

  case 33: /* expression: expression '/' expression  */
#line 213 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("/", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1663 "tshade.cpp"
    break;

  case 34: /* expression: expression '%' expression  */
#line 215 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("%", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1669 "tshade.cpp"
    break;

  case 35: /* expression: expression OP_EQ expression  */
#line 217 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("==", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1675 "tshade.cpp"
    break;

  case 36: /* expression: expression OP_NEQ expression  */
#line 219 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("!=", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1681 "tshade.cpp"
    break;

  case 37: /* expression: expression '<' expression  */
#line 221 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("<", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1687 "tshade.cpp"
    break;

  case 38: /* expression: expression '>' expression  */
#line 223 "tshade.y"
    { (yyval.node) = new tBinaryOpNode(">", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1693 "tshade.cpp"
    break;

  case 39: /* expression: expression OP_GE expression  */
#line 225 "tshade.y"
    { (yyval.node) = new tBinaryOpNode(">=", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1699 "tshade.cpp"
    break;

  case 40: /* expression: expression OP_AND expression  */
#line 227 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("&&", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1705 "tshade.cpp"
    break;

  case 41: /* expression: expression OP_OR expression  */
#line 229 "tshade.y"
    { (yyval.node) = new tBinaryOpNode("||", (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1711 "tshade.cpp"
    break;

  case 42: /* expression: '!' expression  */
#line 231 "tshade.y"
    { (yyval.node) = new tUnaryOpNode("!", (yyvsp[0].node)); }
#line 1717 "tshade.cpp"
    break;

  case 43: /* expression: '-' expression  */
#line 233 "tshade.y"
    { (yyval.node) = new tUnaryOpNode("-", (yyvsp[0].node)); }
#line 1723 "tshade.cpp"
    break;

  case 44: /* expression: '(' expression ')'  */
#line 235 "tshade.y"
    { (yyval.node) = (yyvsp[-1].node); }
#line 1729 "tshade.cpp"
    break;

  case 45: /* expression: var_type '(' expression_list ')'  */
#line 237 "tshade.y"
    {}
#line 1735 "tshade.cpp"
    break;

  case 46: /* expression: VAR_IDENT '(' expression_list ')'  */
#line 239 "tshade.y"
    {
      tFunctionNode* funcDecl = shadeAst->findFunction((yyvsp[-3].strVal));
      if (funcDecl) {
          (yyval.node) = new tFunctionRefNode(funcDecl, (yyvsp[-1].exprListnode));
      } else {
          yyerror(scanner, shadeAst, "Undefined function");
          (yyval.node) = nullptr;  // Handle error appropriately
      }
    }
#line 1749 "tshade.cpp"
    break;

  case 47: /* expression: VAR_IDENT tSWIZZLE  */
#line 249 "tshade.y"
    { tVarDeclNode* varDecl = shadeAst->findVar((yyvsp[-1].strVal));
      if (varDecl) {
          (yyval.node) = new tVarRefNode(varDecl, (yyvsp[0].strVal));
      } else {
          yyerror(scanner, shadeAst, "Undefined variable");
          (yyval.node) = nullptr;  // Handle error appropriately
      } 
    }
#line 1762 "tshade.cpp"
    break;

  case 48: /* expression: VAR_IDENT  */
#line 258 "tshade.y"
    { tVarDeclNode* varDecl = shadeAst->findVar((yyvsp[0].strVal));
      if (varDecl) {
          (yyval.node) = new tVarRefNode(varDecl);
      } else {
          yyerror(scanner, shadeAst, "Undefined variable");
          (yyval.node) = nullptr;  // Handle error appropriately
      } 
    }
#line 1775 "tshade.cpp"
    break;

  case 49: /* expression: VAR_IDENT '.' VAR_IDENT  */
#line 267 "tshade.y"
    { tVarDeclNode* varDecl = shadeAst->findVar((yyvsp[-2].strVal));
      if (varDecl->isStruct) {
          (yyval.node) = new tVarRefNode(varDecl);
      } else {
          yyerror(scanner, shadeAst, "Unknown member");
          (yyval.node) = nullptr;  // Handle error appropriately
      } 
    }
#line 1788 "tshade.cpp"
    break;

  case 50: /* expression: INT_NUM  */
#line 276 "tshade.y"
    { (yyval.node) = new tIntLiteralNode((yyvsp[0].intVal)); }
#line 1794 "tshade.cpp"
    break;

  case 51: /* expression: FLOAT_NUM  */
#line 278 "tshade.y"
    { (yyval.node) = new tFloatLiteralNode((yyvsp[0].fVal)); }
#line 1800 "tshade.cpp"
    break;

  case 52: /* var_type: tMAT34_TYPE  */
#line 283 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_MAT34;}
#line 1806 "tshade.cpp"
    break;

  case 53: /* var_type: tMAT43_TYPE  */
#line 285 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_MAT43;}
#line 1812 "tshade.cpp"
    break;

  case 54: /* var_type: tMAT3_TYPE  */
#line 287 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_MAT33;}
#line 1818 "tshade.cpp"
    break;

  case 55: /* var_type: tMAT4_TYPE  */
#line 289 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_MAT44;}
#line 1824 "tshade.cpp"
    break;

  case 56: /* var_type: tFVEC2_TYPE  */
#line 291 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_FLOAT2;}
#line 1830 "tshade.cpp"
    break;

  case 57: /* var_type: tFVEC3_TYPE  */
#line 293 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_FLOAT3;}
#line 1836 "tshade.cpp"
    break;

  case 58: /* var_type: tFVEC4_TYPE  */
#line 295 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_FLOAT4;}
#line 1842 "tshade.cpp"
    break;

  case 59: /* var_type: tIVEC2_TYPE  */
#line 297 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_INT2;}
#line 1848 "tshade.cpp"
    break;

  case 60: /* var_type: tIVEC3_TYPE  */
#line 299 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_INT3;}
#line 1854 "tshade.cpp"
    break;

  case 61: /* var_type: tIVEC4_TYPE  */
#line 301 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_INT4;}
#line 1860 "tshade.cpp"
    break;

  case 62: /* var_type: tBVEC2_TYPE  */
#line 303 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_BOOL2;}
#line 1866 "tshade.cpp"
    break;

  case 63: /* var_type: tBVEC3_TYPE  */
#line 305 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_BOOL3;}
#line 1872 "tshade.cpp"
    break;

  case 64: /* var_type: tBVEC4_TYPE  */
#line 307 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_BOOL4;}
#line 1878 "tshade.cpp"
    break;

  case 65: /* var_type: tFLOAT_TYPE  */
#line 309 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_FLOAT;}
#line 1884 "tshade.cpp"
    break;

  case 66: /* var_type: tINT_TYPE  */
#line 311 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_INT;}
#line 1890 "tshade.cpp"
    break;

  case 67: /* var_type: tUINT_TYPE  */
#line 313 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_UINT;}
#line 1896 "tshade.cpp"
    break;

  case 68: /* var_type: tBOOL_TYPE  */
#line 315 "tshade.y"
    {(yyval.varType) = ShaderVarType::tTYPE_BOOL;}
#line 1902 "tshade.cpp"
    break;

  case 69: /* statement_list: %empty  */
#line 320 "tshade.y"
    { (yyval.stmt_list_node) = new tStatementListNode(); }
#line 1908 "tshade.cpp"
    break;

  case 70: /* statement_list: statement_list statement  */
#line 322 "tshade.y"
    { (yyvsp[-1].stmt_list_node)->addStatement((yyvsp[0].node)); (yyval.stmt_list_node) = (yyvsp[-1].stmt_list_node); }
#line 1914 "tshade.cpp"
    break;

  case 71: /* statement: var_decl  */
#line 327 "tshade.y"
    {(yyval.node) = (yyvsp[0].declNode);}
#line 1920 "tshade.cpp"
    break;

  case 72: /* statement: uniform_decl  */
#line 329 "tshade.y"
    {(yyval.node) = (yyvsp[0].declNode);}
#line 1926 "tshade.cpp"
    break;

  case 73: /* statement: if_statement  */
#line 331 "tshade.y"
    {(yyval.node) = (yyvsp[0].node);}
#line 1932 "tshade.cpp"
    break;

  case 74: /* statement: while_statement  */
#line 333 "tshade.y"
    {(yyval.node) = (yyvsp[0].node);}
#line 1938 "tshade.cpp"
    break;

  case 75: /* statement: continue_statement  */
#line 335 "tshade.y"
    {(yyval.node) = (yyvsp[0].node);}
#line 1944 "tshade.cpp"
    break;

  case 76: /* statement: break_statement  */
#line 337 "tshade.y"
    {(yyval.node) = (yyvsp[0].node);}
#line 1950 "tshade.cpp"
    break;

  case 77: /* statement: return_statement  */
#line 339 "tshade.y"
    {(yyval.node) = (yyvsp[0].node);}
#line 1956 "tshade.cpp"
    break;

  case 78: /* statement: function_def  */
#line 341 "tshade.y"
    {(yyval.node) = (yyvsp[0].funcNode); }
#line 1962 "tshade.cpp"
    break;

  case 79: /* if_statement: rwIF '(' expression ')' '{' statement_list '}' rwELSE '{' statement_list '}'  */
#line 346 "tshade.y"
    { (yyval.node) = new tIfNode((yyvsp[-8].node), (yyvsp[-5].stmt_list_node), (yyvsp[-1].stmt_list_node)); }
#line 1968 "tshade.cpp"
    break;

  case 80: /* if_statement: rwIF '(' expression ')' '{' statement_list '}'  */
#line 348 "tshade.y"
    { (yyval.node) = new tIfNode((yyvsp[-4].node), (yyvsp[-1].stmt_list_node)); }
#line 1974 "tshade.cpp"
    break;

  case 81: /* while_statement: rwWHILE '(' expression ')' '{' statement_list '}'  */
#line 353 "tshade.y"
    { (yyval.node) = new tWhileNode((yyvsp[-4].node), (yyvsp[-1].stmt_list_node)); }
#line 1980 "tshade.cpp"
    break;

  case 82: /* while_statement: rwDO '{' statement_list '}' rwWHILE '(' expression ')'  */
#line 355 "tshade.y"
    { (yyval.node) = new tWhileNode((yyvsp[-1].node), (yyvsp[-5].stmt_list_node), true); }
#line 1986 "tshade.cpp"
    break;

  case 83: /* continue_statement: rwCONTINUE ';'  */
#line 360 "tshade.y"
    { (yyval.node) = new tContinueNode(); }
#line 1992 "tshade.cpp"
    break;

  case 84: /* break_statement: rwBREAK ';'  */
#line 365 "tshade.y"
    { (yyval.node) = new tBreakNode(); }
#line 1998 "tshade.cpp"
    break;

  case 85: /* return_statement: rwRETURN ';'  */
#line 370 "tshade.y"
    { (yyval.node) = new tReturnNode(); }
#line 2004 "tshade.cpp"
    break;

  case 86: /* return_statement: rwRETURN expression ';'  */
#line 372 "tshade.y"
    { (yyval.node) = new tReturnNode((yyvsp[-1].node)); }
#line 2010 "tshade.cpp"
    break;


#line 2014 "tshade.cpp"

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

#line 375 "tshade.y"


void yyerror(yyscan_t yyscanner, const char* msg){
    Con::errorf("TorqueShader ERROR: %s Line: %d", msg, yylineno);
}

void yyerror(yyscan_t yyscanner, tShadeAst* shadeAst, char const *msg) {
	yyerror(yyscanner, msg);
}
