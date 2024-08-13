%define api.header.include {"tshade.h"}
%define api.pure full
%define api.prefix {tshade_}

%lex-param {yyscan_t scanner}
%parse-param {yyscan_t scanner}
%parse-param {tShadeAst* shadeAst}

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
  double fVal;
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
%token tSWIZZLE

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

%left OP_OR
%left OP_AND
%left '|' '^' '&'
%left OP_EQ OP_NEQ
%left '<' '>' OP_LE OP_GE
%left '+' '-'
%left '*' '/' '%'
%right '!' '~' // Prefix operators
%nonassoc '(' ')' // Highest precedence for grouping

%type <node> program program_globals var_decl expression shader_stage shader_body
%type <intVal> var_type


%start program

%%

program 
  : tSHADERDECLARE STR_VAL '{' program_globals '}' 
    { $$ = nullptr; shadeAst->shaderName = $2; }
  ;

program_globals 
  : program_globals var_decl
    {$$ = nullptr; shadeAst->mGlobalVars.push_back($1); }
  ;

shader_stage
  : tVSSHADER '{' shader_body '}'
    { shadeAst->mVertStage = new tStageNode(ShaderStageType::tSTAGE_VERTEX, $3); }
  | tPSSHADER '{' shader_body '}'
    {shadeAst->mPixStage = new tStageNode(ShaderStageType::tSTAGE_PIXEL, $3);}
  | tGSSHADER '{' shader_body '}'
    {shadeAst->mPixStage = new tStageNode(ShaderStageType::tSTAGE_GEOMETRY, $3);}
  | tCSSHADER '{' shader_body '}'
    {shadeAst->mPixStage = new tStageNode(ShaderStageType::tSTAGE_COMPUTE, $3);}
  ;

shader_body
  :
  ;

var_decl
  : var_type VAR_IDENT ';'
    {$$ = new tVarDeclNode($2, $1); }
  | var_type VAR_IDENT '=' expression ';'
    {$$ = new tVarDeclNode($2, $1, $4);}
  ;

expression
  : expression '+' expression 
    { $$ = new tBinaryOpNode('+', $1, $3); }
  | expression '-' expression 
    { $$ = new tBinaryOpNode('-', $1, $3); }
  | expression '*' expression 
    { $$ = new tBinaryOpNode('*', $1, $3); }
  | expression '/' expression 
    { $$ = new tBinaryOpNode('/', $1, $3); }
  | expression '%' expression 
    { $$ = new tBinaryOpNode('%', $1, $3); }
  | expression OP_EQ expression 
    { $$ = new tBinaryOpNode('==', $1, $3); }
  | expression OP_NEQ expression 
    { $$ = new tBinaryOpNode('!=', $1, $3); }
  | expression '<' expression 
    { $$ = new tBinaryOpNode('<', $1, $3); }
  | expression '>' expression 
    { $$ = new tBinaryOpNode('>', $1, $3); }
  | expression OP_GE expression 
    { $$ = new tBinaryOpNode('>=', $1, $3); }
  | expression OP_AND expression 
    { $$ = new tBinaryOpNode('&&', $1, $3); }
  | expression OP_OR expression 
    { $$ = new tBinaryOpNode('||', $1, $3); }
  | '!' expression 
    { $$ = new tUnaryOpNode('!', $2); }
  | '-' expression %prec '*' 
    { $$ = new tUnaryOpNode('-', $2); } // Unary negation
  | '(' expression ')' 
    { $$ = $2; } // Grouping
  | expression '.' VAR_IDENT
    {}
  | VAR_IDENT tSWIZZLE
    {}
  | VAR_IDENT 
    { $$ = new tVarRefNode($1); }
  | INT_NUM 
    { $$ = new tIntLiteralNode($1); }
  | FLOAT_NUM 
    { $$ = new tFloatLiteralNode($1); }
  ;

var_type
  : tMAT34_TYPE 
    {$$ = ShaderVarType::tTYPE_MAT34;}
  | tMAT43_TYPE
    {$$ = ShaderVarType::tTYPE_MAT43;}
  | tMAT3_TYPE
    {$$ = ShaderVarType::tTYPE_MAT33;}
  | tMAT4_TYPE
    {$$ = ShaderVarType::tTYPE_MAT44;}
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
