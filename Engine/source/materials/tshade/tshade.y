%define api.pure full
%define api.prefix {tshade_}

%lex-param {yyscan_t scanner}
%parse-param {yyscan_t scanner}
%parse-param {tshadeAst* shadeAst}

%{
  #include <stdlib.h>
  #include <stdio.h>
  #include "platform/platform.h"
  #include "core/strings/stringFunctions.h"
  #include "tshadeAst.h"

  #define nil 0

  typedef void* yyscan_t;
  void yyerror (yyscan_t yyscanner, const char* msg);
  void yyerror (yyscan_t yyscanner, tshadeAst* shadeAst, char const *msg);
  extern int yylex(YYSTYPE *yylval_param, yyscan_t yyscanner);

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
// flow control keyword tokens
%token '+' '-' '*' '/' '<' '>' '=' '.' '|' '&' '%'
%token '(' ')' ',' ':' ';' '{' '}' '^' '~' '!' '@'

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

// shader scalar types
%token tFLOAT_TYPE tINT_TYPE tBOOL_TYPE tUINT_TYPE

// shader vector types
%token tFVEC2_TYPE tFVEC3_TYPE tFVEC4_TYPE 
%token tIVEC2_TYPE tIVEC3_TYPE tIVEC4_TYPE
%token tBVEC2_TYPE tBVEC3_TYPE tBVEC4_TYPE

%type <node> program program_globals

%start program

%%

program 
  : program_globals 
    { $$ = nullptr; }

program_globals 
  : tSHADERDECLARE STR_VAL ';' 
    {$$ = nullptr; shadeAst->shaderName = $2;}

%%

void yyerror (yyscan_t yyscanner, const char* msg){
    fprintf(stderr, "%s\n", msg);
}

void yyerror (yyscan_t yyscanner, tshadeAst* shadeAst, char const *msg) {
	yyerror(yyscanner, msg);
}
