%define api.header.include {"tshade.h"}
%define api.pure full
%define api.prefix {tshade_}

%lex-param {yyscan_t scanner}
%parse-param {yyscan_t scanner}
%parse-param {tshadeAst* shadeAst}

%{
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
%} 

%union{
  // source side ASTnode.
  struct tShadeNode* node;
  // symbol specifics.
  float fVal;
  int intVal;
  const char* strVal;
}

// Language agnostic tokens.
%token '+' '-' '*' '/' '<' '>' '=' '.' '|' '&' '%'
%token '(' ')' ',' ':' ';' '{' '}' '^' '~' '!' '@'

// flow control keyword tokens
%token rwSWITCH rwCASE rwDEFAULT rwWHILE rwDO 
%token rwFOR rwBREAK rwCONTINUE rwIF rwELSE rwDISCARD
%token rwVOID rwSTATIC rwIN rwOUT rwINOUT rwTYPEDEF
%token rwTRUE rwFALSE

// Conditional OPS.
%token OP_EQ OP_NEQ OP_AND OP_OR OP_LE OP_GE

// Assignment OPS.
// incremental assignment.
%token OP_PLUSPLUS OP_MINUSMINUS
%token OP_PLUS_ASS OP_MINUS_ASS OP_MUL_ASS OP_DIV_ASS OP_MOD_ASS
%token OP_AND_ASS OP_OR_ASS OP_XOR_ASS OP_BIT_LEFT_ASS OP_BIT_RIGHT_ASS

// bitshift OPS.
%token OP_BIT_LEFT OP_BIT_RIGHT

%token <intVal> INT_NUM
%token <fVal> FLOAT_NUM
%token <strVal> VAR_IDENT STR_VAL TYPE_IDENT

// Everything after this point is specific to shader language. 
// shader specific keywords
%token tSTRUCT tUNIFORM tCBUFFER tSHADERDECLARE

// shader stages
%token tVSSHADER tPSSHADER tGSSHADER tCSSHADER tDSSHADER tHSSHADER

// shader scalar types
%token tFLOAT_TYPE tINT_TYPE tBOOL_TYPE tUINT_TYPE

// shader vector types
%token tFVEC2_TYPE tFVEC3_TYPE tFVEC4_TYPE 
%token tIVEC2_TYPE tIVEC3_TYPE tIVEC4_TYPE
%token tBVEC2_TYPE tBVEC3_TYPE tBVEC4_TYPE

// shader matrix types
%token tMAT4_TYPE tMAT43_TYPE tMAT34_TYPE tMAT3_TYPE

%type <node> program program_globals var_decl
%type <intVal> var_type

%start program

%%

program 
  : program_globals 
    { $$ = nullptr; }
  ;

program_globals 
  : tSHADERDECLARE STR_VAL ';' 
    {$$ = nullptr; shadeAst->shaderName = $2;}
  | var_decl
    {$$ = nullptr; shadeAst->mGlobalVars.push_back($1); }
  ;

var_decl
  : var_type VAR_IDENT ';'
    {}
  | var_type VAR_IDENT ':' ';'
    {}
  ;

var_type
  : tMAT34_TYPE 
    {$$ = ShaderVarType::tTYPE_MAT34;}
  | tMAT43_TYPE
    {$$ = ShaderVarType::tTYPE_MAT43;}
  | tMAT3_TYPE
    {$$ = ShaderVarType::tMAT3_TYPE;}
  | tMAT4_TYPE
    {$$ = ShaderVarType::tMAT4_TYPE;}
  | tFVEC2_TYPE
    {$$ = ShaderVarType::tTYPE_FLOAT2;}
  | tFVEC3_TYPE
    {$$ = ShaderVarType::tTYPE_FLOAT3;}
  | tFVEC4_TYPE
    {$$ = ShaderVarType::tTYPE_FLOAT4;}
  | tIVEC2_TYPE
    {$$ = ShaderVarType::tTYPE_INT2;}
  | tIVEC3_TYPE
    {$$ = ShaderVarType::tTYPE_INT3;}
  | tIVEC4_TYPE
    {$$ = ShaderVarType::tTYPE_INT4;}
  | tBVEC2_TYPE
    {$$ = ShaderVarType::tTYPE_BOOL2;}
  | tBVEC3_TYPE
    {$$ = ShaderVarType::tTYPE_BOOL3;}
  | tBVEC4_TYPE
    {$$ = ShaderVarType::tTYPE_BOOL4;}
  | tFLOAT_TYPE
    {$$ = ShaderVarType::tTYPE_FLOAT;}
  | tINT_TYPE
    {$$ = ShaderVarType::tTYPE_INT;}
  | tUINT_TYPE
    {$$ = ShaderVarType::tTYPE_UINT;}
  | tBOOL_TYPE
    {$$ = ShaderVarType::tTYPE_BOOL;}
  ;

%%

void yyerror(yyscan_t yyscanner, const char* msg){
    Con::errorf("TorqueShader ERROR: %s Line: %d", msg, yylineno);
}

void yyerror(yyscan_t yyscanner, tShadeAst* shadeAst, char const *msg) {
	yyerror(yyscanner, msg);
}
