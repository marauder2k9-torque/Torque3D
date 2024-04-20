%define api.pure full
%lex-param {yyscan_t scanner}
%parse-param {yyscan_t scanner}
%parse-param {tshadeAst* shadeAst}

%{
  #include <stdlib.h>
  #include <stdio.h>
  #include "console/console.h"
  #include "console/consoleInternal.h"
  #include "core/strings/stringFunctions.h"
  #include "tshadeAst.h"

  typedef void* yyscan_t;
  void yyerror (yyscan_t yyscanner, const char* msg);
  int yylex(YYSTYPE *yylval_param, yyscan_t yyscanner);
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
%token rwSWITCH rwCASE rwDEFAULT rwWHILE rwDO 
%token rwFOR rwBREAK rwCONTINUE rwIF rwELSE rwDISCARD

%token END_INCLUDE

// Conditional OPS.
%token OP_EQ OP_NEQ OP_AND OP_OR OP_LE OP_GE

// Operators and misc
%token <intVal> '+' '-' '*' '/' '<' '>' '=' '.' '|' '&' '%'
%token <intVal> '(' ')' ',' ':' ';' '{' '}' '^' '~' '!' '@'

%token <intVal> INT_NUM
%token <fVal> FLOAT_NUM
%token <strVal> VAR_IDENT STR_VAL TYPE_IDENT

// Everything after this point is specific to shader language. 
// shader specific keywords
%token tSTRUCT tUNIFORM tCBUFFER

%start program

%%



%%

void yyerror (yyscan_t yyscanner, const char* msg){
    fprintf(stderr, "%s\n", msg);
}