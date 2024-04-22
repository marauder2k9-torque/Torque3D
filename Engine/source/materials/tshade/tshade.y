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
  void yyerror(yyscan_t yyscanner, tshadeAst* shadeAst, char const *msg);
  #define YY_DECL int yylex(union YYSTYPE *, yyscan_t)
  YY_DECL;

  extern int yylineno;
%} 

%union{
  // source side ASTnode.
  struct tshadeNode* node;
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

// Conditional OPS.
%token OP_EQ OP_NEQ OP_AND OP_OR OP_LE OP_GE

%token <intVal> INT_NUM
%token <fVal> FLOAT_NUM
%token <strVal> VAR_IDENT STR_VAL TYPE_IDENT

// Everything after this point is specific to shader language. 
// shader specific keywords
%token tSTRUCT tUNIFORM tCBUFFER tSHADERDECLARE

// shader stages
%token tVSSHADER

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
  ;

var_type
  : tMAT34_TYPE 
    {$$ = tshade_tokentype::tMAT34_TYPE;}
  | tMAT43_TYPE
    {$$ = tshade_tokentype::tMAT43_TYPE;}
  | tMAT3_TYPE
    {$$ = tshade_tokentype::tMAT3_TYPE;}
  | tMAT4_TYPE
    {$$ = tshade_tokentype::tMAT4_TYPE;}
  | tFVEC2_TYPE
    {$$ = tshade_tokentype::tFVEC2_TYPE;}
  | tFVEC3_TYPE
    {$$ = tshade_tokentype::tFVEC3_TYPE;}
  | tFVEC4_TYPE
    {$$ = tshade_tokentype::tFVEC4_TYPE;}
  | tIVEC2_TYPE
    {$$ = tshade_tokentype::tIVEC2_TYPE;}
  | tIVEC3_TYPE
    {$$ = tshade_tokentype::tIVEC3_TYPE;}
  | tIVEC4_TYPE
    {$$ = tshade_tokentype::tIVEC4_TYPE;}
  | tBVEC2_TYPE
    {$$ = tshade_tokentype::tBVEC2_TYPE;}
  | tBVEC3_TYPE
    {$$ = tshade_tokentype::tBVEC3_TYPE;}
  | tBVEC4_TYPE
    {$$ = tshade_tokentype::tBVEC4_TYPE;}
  | tFLOAT_TYPE
    {$$ = tshade_tokentype::tFLOAT_TYPE;}
  | tINT_TYPE
    {$$ = tshade_tokentype::tINT_TYPE;}
  | tUINT_TYPE
    {$$ = tshade_tokentype::tUINT_TYPE;}
  | tBOOL_TYPE
    {$$ = tshade_tokentype::tBOOL_TYPE;}
  ;

%%

void yyerror(yyscan_t yyscanner, const char* msg){
    Con::errorf("TorqueShader ERROR: %s Line: %d", msg, yylineno);
}

void yyerror(yyscan_t yyscanner, tshadeAst* shadeAst, char const *msg) {
	yyerror(yyscanner, msg);
}
