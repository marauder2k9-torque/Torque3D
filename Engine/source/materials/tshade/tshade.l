%option reentrant bison-bridge
%option never-interactive
%option yylineno
%option prefix="tshade_"

%{
  #define YYLMAX 4096
  #define YY_NO_UNISTD_H

  #include <stdio.h>
  #include "platform/platform.h"
  #include "core/stringTable.h"
  #include "console/console.h"                                                
  #include "console/dynamicTypes.h"
  #include "core/strings/stringFunctions.h"
  #include "tshadeAst.h"
  #include "tshade.h"

%}

DIGIT    [0-9]
INTEGER  {DIGIT}+
FLOAT    ({INTEGER}?\.{INTEGER})|({INTEGER}(\.{INTEGER})?[eE][+-]?{INTEGER})
LETTER   [A-Za-z_]
FILECHAR [A-Za-z_\.]
SPACE    [ \t\v\f]
HEXDIGIT [a-fA-F0-9]
IDTAIL   [A-Za-z0-9_]
VAR      {LETTER}{IDTAIL}*

%x COMMENT

%%

{SPACE}+ { }

"//"[^\n\r]* ;
[\r] ;
[\n] ;

"if"        {return rwIF;}
"else"      {return rwELSE;}
"while"     {return rwWHILE;}
"do"        {return rwDO;}
"break"     {return rwBREAK;}
"for"       {return rwFOR;}
"switch"    {return rwSWITCH;}
"case"      {return rwCASE;}
"default"   {return rwDEFAULT;}
"continue"  {return rwCONTINUE;}
"discard"   {return rwDISCARD;}

"=="        {return OP_EQ;}
"!="        {return OP_NEQ;}
"&&"        {return OP_AND;}
"||"        {return OP_OR;}
"<="        {return OP_LE;}
">="        {return OP_GE;}

"?" |
"[" |
"]" |
"(" |
")" |
"+" |
"-" |
"*" |
"/" |
"<" |
">" |
"|" |
"." |
"!" |
":" |
";" |
"{" |
"}" |
"," |
"&" |
"%" |
"^" |
"~" |
"="               { return *yytext; }
0[xX]{HEXDIGIT}+  { S32 val = 0; dSscanf(yytext, "%x", &val); yylval->intVal = val; return INT_NUM; }
{INTEGER}         { yylval->intVal = dAtoi(yytext); return INT_NUM; }
{FLOAT}           { yylval->fVal = dAtof(yytext); return FLOAT_NUM; }
\"[^\"\n]*\"      { yylval->strVal = StringTable->insert(yytext); return STR_VAL; }

"/*"          { BEGIN(COMMENT); }
<COMMENT>(.|\n) {
/*  skip comments , after this point shader specific lexer rules.
    We need a case for each type both hlsl/glsl explicitly as we use these enums
    in the ast's to decide which type a var is. */ 
}

<COMMENT>"*/" { BEGIN(INITIAL); }

"TorqueShader" { return tSHADERDECLARE; }
"struct"    { return tSTRUCT; }
"uniform"   { return tUNIFORM; }
"cbuffer"   { return tCBUFFER; }

"float2" |
"vec2"      {return tFVEC2_TYPE; }
"float3" |
"vec3"      {return tFVEC3_TYPE; }
"float4" |
"vec4"      {return tFVEC4_TYPE; }

"int2" |
"ivec2"      {return tIVEC2_TYPE; }
"int3" |
"ivec3"      {return tIVEC3_TYPE; }
"int4" |
"ivec4"      {return tIVEC4_TYPE; }

"float"     { return tFLOAT_TYPE; }
"int"       { return tINT_TYPE; }
"uint"      { return tUINT_TYPE; }
"bool"      { return tBOOL_TYPE; }

{VAR}       {yylval->strVal = StringTable->insert(yytext); return VAR_IDENT;}

%%

int yywrap(yyscan_t yyscanner)
{
	return 1;
}

bool TShadeParse(const char* code, tshadeAst* shadeAst)
{
  yyscan_t yyscanner;
  yylex_init(&yyscanner);
  struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
  yy_delete_buffer(YY_CURRENT_BUFFER, yyscanner);
  yy_scan_string(code, yyscanner);

  bool ret = tshade_parse(yyscanner, shadeAst);
  yylex_destroy(yyscanner);
  return ret;
}