%option reentrant bison-bridge
%option yylineno

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

  template< typename T >
  struct Token
  {
    T value;
    S32 lineNumber;
  };

  // Can't have ctors in structs used in unions, so we have this.
  template< typename T >
  inline Token< T > MakeToken( T value, U32 lineNumber )
  {
    Token< T > result;
    result.value = value;
    result.lineNumber = lineNumber;
    return result;
  }


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

%x INCLUDE
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
"="               { yylval->intVal = MakeToken< int >( yytext, yylineno ); return *yytext; }
0[xX]{HEXDIGIT}+  { S32 val = 0; dSscanf(yytext, "%x", &val); yylval->intVal = val; return INT_NUM; }
{INTEGER}         { yylval->intVal = dAtoi(yytext); return INT_NUM; }
{FLOAT}           { yylval->fVal = dAtof(yytext); return FLOAT_NUM; }

^"#"[\t]*include[ \t]*\[\"<] {BEGIN(INCLUDE); }
<INCLUDE>[^ \t\n">]+  { /* Handle include files.*/
  {
    int c;
    while((c= input()) && c != '\n');
  }
  yylineno++;
  if(!newfile(yytext))
    yyterminate();

  BEGIN(INITIAL);
}
<INCLUDE>(.|\n) {}

"/*"          {BEGIN(COMMENT);}
<COMMENT>(.|\n) {/* skip comments , after this point shader specific lexer rules*/ }
<COMMENT>"*/" {BEGIN(INITIAL);}

<<EOF>> {
  yypop_buffer_state(yyscanner);
  if(!YY_CURRENT_BUFFER)
    yyterminate();

    return END_INCLUDE;
}

"struct"    {return tSTRUCT;}
"uniform"   {return tUNIFORM;}
"cbuffer"   {return tCBUFFER;}

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

  bool ret = yyparse(yyscanner, shadeAst);
  yylex_destroy(yyscanner);
  return ret;
}