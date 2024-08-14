%option reentrant bison-bridge
%option never-interactive
%option yylineno
%option nounput
%option noinput
%option noyywrap
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
NEWLINE  (\n)|(\r\n)

%x COMMENT

%%

{SPACE}+ { }

"//"[^\n\r]* ;  

"if"        { return rwIF;        }
"else"      { return rwELSE;      }
"while"     { return rwWHILE;     }
"do"        { return rwDO;        }
"break"     { return rwBREAK;     }
"for"       { return rwFOR;       }
"switch"    { return rwSWITCH;    }
"case"      { return rwCASE;      }
"default"   { return rwDEFAULT;   }
"continue"  { return rwCONTINUE;  }
"discard"   { return rwDISCARD;   }
"void"      { return rwVOID;      }
"static"    { return rwSTATIC;    }
"in"        { return rwIN;        }
"out"       { return rwOUT;       }
"inout"     { return rwINOUT;     }
"typedef"   { return rwTYPEDEF;   }
"true"      { return rwTRUE;      }
"false"     { return rwFALSE;     }
"return"    { return rwRETURN;    }

"=="        { return OP_EQ;       }
"!="        { return OP_NEQ;      }
"&&"        { return OP_AND;      }
"||"        { return OP_OR;       }
"<="        { return OP_LE;       }
">="        { return OP_GE;       }

"++"        { return OP_PLUSPLUS; }
"--"        { return OP_MINUSMINUS; }

"+="        { return OP_PLUS_ASS; }
"-="        { return OP_MINUS_ASS; }
"*="        { return OP_MUL_ASS; }
"/="        { return OP_DIV_ASS; }
"%="        { return OP_MOD_ASS; }

"&="        { return OP_AND_ASS; }
"|="        { return OP_OR_ASS; }
"^="        { return OP_XOR_ASS; }
"<<="       { return OP_BIT_LEFT_ASS; }
">>="       { return OP_BIT_RIGHT_ASS; }
"<<"        { return OP_BIT_LEFT; }
">>"        { return OP_BIT_RIGHT; }
 
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
{FLOAT}           { yylval->fVal = dAtod(yytext); return FLOAT_NUM; }
\"(\\.|[^\\"\n\r])*\" { yylval->strVal = StringTable->insert(yytext); return STR_VAL; }

"/*"            { BEGIN(COMMENT); }
<COMMENT>(.|\n) {
/*  skip comments , after this point shader specific lexer rules.
    We need a case for each type both hlsl/glsl explicitly as we use these enums
    in the ast's to decide which type a var is. */ 
}

<COMMENT>"*/"   { BEGIN(INITIAL); }

"TorqueShader"  { return tSHADERDECLARE; }
"struct"        { return tSTRUCT; }
"uniform"       { return tUNIFORM; }
"cbuffer"       { return tCBUFFER; }
"VertexShader"   { return tVSSHADER; }
"PixelShader"    { return tPSSHADER; }
"GeometryShader" { return tGSSHADER; }
"ComputeShader"  { return tCSSHADER; }
"DomainShader"   { return tDSSHADER; }
"HullShader"     { return tHSSHADER; }

"mat3x4"  |
"float3x4"  { return tMAT34_TYPE; }

"mat4x3"  |
"float4x3"  { return tMAT43_TYPE; }

"mat3"    |
"mat3x3"  |
"float3x3"  { return tMAT3_TYPE; }

"mat4"    |
"mat4x4"  |
"float4x4"  { return tMAT4_TYPE; }

"float2" |
"vec2"      { return tFVEC2_TYPE; }
"float3" |
"vec3"      { return tFVEC3_TYPE; }
"float4" |
"vec4"      { return tFVEC4_TYPE; }

"int2" |
"ivec2"      { return tIVEC2_TYPE; }
"int3" |
"ivec3"      { return tIVEC3_TYPE; }
"int4" |
"ivec4"      { return tIVEC4_TYPE; }

"bool2" |
"bvec2"      { return tBVEC2_TYPE; }
"bool3" |
"bvec3"      { return tBVEC3_TYPE; }
"bool4" |
"bvec4"      { return tBVEC4_TYPE; }

"float"     { return tFLOAT_TYPE; }
"int"       { return tINT_TYPE; }
"uint"      { return tUINT_TYPE; }
"bool"      { return tBOOL_TYPE; }

\.[xyzw]{1,4} { yylval->strVal = strdup(yytext); return tSWIZZLE; }
\.[rgba]{1,4} { yylval->strVal = strdup(yytext); return tSWIZZLE; }
\.[stpq]{1,4} { yylval->strVal = strdup(yytext); return tSWIZZLE; }

{VAR}       { yylval->strVal = StringTable->insert(yytext); return VAR_IDENT; }
{NEWLINE}   {}

%%

bool TShadeParse(const char* code, tShadeAst* shadeAst)
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