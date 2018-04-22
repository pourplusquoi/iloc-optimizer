%{
    #ifdef FLEX_SCANNER
        extern int syntax_error;
        extern int yylineno;
        #define YYINCR(n) yylineno += n
    #else
        #define YYINCR(n)
    #endif
    #include "parser.h"
    #include "strings.h"
    int CTRL_M = 0; /* count number of ^M’s seen */
%}

%%
"//"[^\n]*    ; /* skip comment part */

"nop"         return NOP;

"add"         return ADD;
"addI"        return ADDI;
"sub"         return SUB;
"subI"        return SUBI;
"mult"        return MULT;
"multI"       return MULTI;
"div"         return DIV;
"divI"        return DIVI;
"lshift"      return LSHIFT;
"lshiftI"     return LSHIFTI;
"rshift"      return RSHIFT;
"rshiftI"     return RSHIFTI;
"and"         return AND;
"andI"        return ANDI;
"or"          return OR;
"orI"         return ORI;
"not"         return NOT;

"loadI"       return LOADI;
"load"        return LOAD;
"loadAI"      return LOADAI;
"loadAO"      return LOADAO;
"cload"       return CLOAD;
"cloadAI"     return CLOADAI;
"cloadAO"     return CLOADAO;
"store"       return STORE;
"storeAI"     return STOREAI;
"storeAO"     return STOREAO;
"cstore"      return CSTORE;
"cstoreAI"    return CSTOREAI;
"cstoreAO"    return CSTOREAO;
"i2i"         return ITOI;
"c2c"         return CTOC;
"i2c"         return ITOC;
"c2i"         return CTOI;

"br"          return BR;
"cbr"         return CBR;
"cmp_LT"      return CMPLT;
"cmp_LE"      return CMPLE;
"cmp_GT"      return CMPGT;
"cmp_GE"      return CMPGE;
"cmp_EQ"      return CMPEQ;
"cmp_NE"      return CMPNE;
"halt"        return HALT;

"read"        return READ;
"cread"       return CREAD;
"output"      return OUTPUT;
"coutput"     return COUTPUT;
"write"       return WRITE;
"cwrite"      return CWRITE;

":"           return ':';
","           return ',';
"->"          return '-';
"=>"          return '=';

[\n]          YYINCR(1);
[ \t]+        ; /* skip all blanks */

[r][0-9]+ {
    yylval.myInt = atoi (&yytext[1]);
    return REGISTER;
}

[A-Za-z][A-Za-z0-9]* {
    yylval.myString = strndup (yytext, yyleng);
    return LABEL;
}

[0-9]+ {
    yylval.myInt = atoi (yytext);
    return NUMBER;
}

. { /* error token */

    char this = *yytext;
    fprintf (stderr, "Scanner: unknown symbol '%s' around line %d.\n", yytext, yylineno);

    if (this == 0xd && 0 == CTRL_M++)
        fprintf (stderr, "CTRL/LF problem\n");
    else
        yylval.myString = strndup (yytext, yyleng);
    return JUNK;
}

%%
