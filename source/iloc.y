%{
    #include <stdio.h>
    #include "../headers/repre.h"
    int yylineno;
    int syntax_error = 0;
    char *yytext;
    FILE *yyin, *yyout;

    int yylex ();
    void yyerror (struct Instructions**, const char*);
%}

%union {
    int  myInt;
    char *myString;
    struct Operation  *myOp;
    struct Instruction  *myStmt;
    struct Instructions *myStmts;
}

%parse-param {struct Instructions **myStmts}

%token <myString> JUNK

%token <myString> LABEL
%token <myInt> REGISTER
%token <myInt> NUMBER

%token NOP

%token ADD
%token ADDI
%token SUB
%token SUBI
%token MULT
%token MULTI
%token DIV
%token DIVI
%token LSHIFT
%token LSHIFTI
%token RSHIFT
%token RSHIFTI
%token AND
%token ANDI
%token OR
%token ORI
%token NOT

%token LOADI
%token LOAD
%token LOADAI
%token LOADAO
%token CLOAD
%token CLOADAI
%token CLOADAO
%token STORE
%token STOREAI
%token STOREAO
%token CSTORE
%token CSTOREAI
%token CSTOREAO
%token ITOI
%token CTOC
%token ITOC
%token CTOI

%token BR
%token CBR
%token CMPLT
%token CMPLE
%token CMPGT
%token CMPGE
%token CMPEQ
%token CMPNE
%token HALT

%token READ
%token CREAD
%token OUTPUT
%token COUTPUT
%token WRITE
%token CWRITE

%start Procedure

%type <myStmts> Procedure
%type <myStmts> Instructions
%type <myStmt> Instruction
%type <myOp> Operation

%%
Procedure
    : Instructions HALT
        {
            $$ = $1;
            *myStmts = $$;
        }
    ;

Instructions
    : Instructions Instruction
        {
            $$ = $1;
            appendInstruction ($$, $2);
        }
    | Instruction
        {
            $$ = makeInstructions ($1);
        }
    ;

Instruction
    : LABEL ':' Operation
        {
            $$ = makeInstruction ($1, $3);
        }
    | Operation
        {
            $$ = makeInstruction (NULL, $1);
        }
    ;

Operation
    : NOP
        {
            $$ = makeOperation (nop_, 0, 0, 0, 0);
        }
    | ADD REGISTER ',' REGISTER '=' REGISTER
        {
            $$ = makeOperation (add_, $2, $4, $6, 0);
        }
    | ADDI REGISTER ',' NUMBER '=' REGISTER
        {
            $$ = makeOperation (addI_, $2, 0, $6, $4);
        }
    | SUB REGISTER ',' REGISTER '=' REGISTER
        {
            $$ = makeOperation (sub_, $2, $4, $6, 0);
        }
    | SUBI REGISTER ',' NUMBER '=' REGISTER
        {
            $$ = makeOperation (subI_, $2, 0, $6, $4);
        }
    | MULT REGISTER ',' REGISTER '=' REGISTER
        {
            $$ = makeOperation (mult_, $2, $4, $6, 0);
        }
    | MULTI REGISTER ',' NUMBER '=' REGISTER
        {
            $$ = makeOperation (multI_, $2, 0, $6, $4);
        }
    | DIV REGISTER ',' REGISTER '=' REGISTER
        {
            $$ = makeOperation (div_, $2, $4, $6, 0);
        }
    | DIVI REGISTER ',' NUMBER '=' REGISTER
        {
            $$ = makeOperation (divI_, $2, 0, $6, $4);
        }
    | LSHIFT REGISTER ',' REGISTER '=' REGISTER
        {
            $$ = makeOperation (lshift_, $2, $4, $6, 0);
        }
    | LSHIFTI REGISTER ',' NUMBER '=' REGISTER
        {
            $$ = makeOperation (lshiftI_, $2, 0, $6, $4);
        }
    | RSHIFT REGISTER ',' REGISTER '=' REGISTER
        {
            $$ = makeOperation (rshift_, $2, $4, $6, 0);
        }
    | RSHIFTI REGISTER ',' NUMBER '=' REGISTER
        {
            $$ = makeOperation (rshiftI_, $2, 0, $6, $4);
        }
    | AND REGISTER ',' REGISTER '=' REGISTER
        {
            $$ = makeOperation (and_, $2, $4, $6, 0);
        }
    | ANDI REGISTER ',' NUMBER '=' REGISTER
        {
            $$ = makeOperation (andI_, $2, 0, $6, $4);
        }
    | OR REGISTER ',' REGISTER '=' REGISTER
        {
            $$ = makeOperation (or_, $2, $4, $6, 0);
        }
    | ORI REGISTER ',' NUMBER '=' REGISTER
        {
            $$ = makeOperation (orI_, $2, 0, $6, $4);
        }
    | NOT REGISTER '=' REGISTER
        {
            $$ = makeOperation (not_, $2, 0, $4, 0);
        }

    | LOADI NUMBER '=' REGISTER
        {
            $$ = makeOperation (loadI_, 0, 0, $4, $2);
        }
    | LOAD REGISTER '=' REGISTER
        {
            $$ = makeOperation (load_, $2, 0, $4, 0);
        }
    | LOADAI REGISTER ',' NUMBER '=' REGISTER
        {
            $$ = makeOperation (loadAI_, $2, 0, $6, $4);
        }
    | LOADAO REGISTER ',' REGISTER '=' REGISTER
        {
            $$ = makeOperation (loadAO_, $2, $4, $6, 0);
        }
    | CLOAD REGISTER '=' REGISTER
        {
            $$ = makeOperation (cload_, $2, 0, $4, 0);
        }
    | CLOADAI REGISTER ',' NUMBER '=' REGISTER
        {
            $$ = makeOperation (cloadAI_, $2, 0, $6, $4);
        }
    | CLOADAO REGISTER ',' REGISTER '=' REGISTER
        {
            $$ = makeOperation (cloadAO_, $2, $4, $6, 0);
        }
    | STORE REGISTER '=' REGISTER
        {
            $$ = makeOperation (store_, $2, $4, 0, 0);
        }
    | STOREAI REGISTER '=' REGISTER ',' NUMBER
        {
            $$ = makeOperation (storeAI_, $2, $4, 0, $6);
        }
    | STOREAO REGISTER '=' REGISTER ',' REGISTER
        {
            $$ = makeOperation (storeAO_, $2, $4, $6, 0);
        }
    | CSTORE REGISTER '=' REGISTER
        {
            $$ = makeOperation (cstore_, $2, $4, 0, 0);
        }
    | CSTOREAI REGISTER '=' REGISTER ',' NUMBER
        {
            $$ = makeOperation (cstoreAI_, $2, $4, 0, $6);
        }
    | CSTOREAO REGISTER '=' REGISTER ',' REGISTER
        {
            $$ = makeOperation (cstoreAO_, $2, $4, $6, 0);
        }
    | ITOI REGISTER '=' REGISTER
        {
            $$ = makeOperation (i2i_, $2, 0, $4, 0);
        }
    | CTOC REGISTER '=' REGISTER
        {
            $$ = makeOperation (c2c_, $2, 0, $4, 0);
        }
    | ITOC REGISTER '=' REGISTER
        {
            $$ = makeOperation (i2c_, $2, 0, $4, 0);
        }
    | CTOI REGISTER '=' REGISTER
        {
            $$ = makeOperation (c2i_, $2, 0, $4, 0);
        }

    | BR '-' LABEL
        {
            $$ = makeBranch (br_, 0, $3, NULL);
        }
    | CBR REGISTER '-' LABEL ',' LABEL
        {
            $$ = makeBranch (cbr_, $2, $4, $6);
        }
    | CMPLT REGISTER ',' REGISTER '=' REGISTER
        {
            $$ = makeOperation (cmp_LT_, $2, $4, $6, 0);
        }
    | CMPLE REGISTER ',' REGISTER '=' REGISTER
        {
            $$ = makeOperation (cmp_LE_, $2, $4, $6, 0);
        }
    | CMPGT REGISTER ',' REGISTER '=' REGISTER
        {
            $$ = makeOperation (cmp_GT_, $2, $4, $6, 0);
        }
    | CMPGE REGISTER ',' REGISTER '=' REGISTER
        {
            $$ = makeOperation (cmp_GE_, $2, $4, $6, 0);
        }
    | CMPEQ REGISTER ',' REGISTER '=' REGISTER
        {
            $$ = makeOperation (cmp_EQ_, $2, $4, $6, 0);
        }
    | CMPNE REGISTER ',' REGISTER '=' REGISTER
        {
            $$ = makeOperation (cmp_NE_, $2, $4, $6, 0);
        }

    | READ '=' REGISTER
        {
            $$ = makeOperation (read_, 0, 0, $3, 0);
        }
    | CREAD '=' REGISTER
        {
            $$ = makeOperation (cread_, 0, 0, $3, 0);
        }
    | OUTPUT NUMBER
        {
            $$ = makeOperation (output_, 0, 0, 0, $2);
        }
    | COUTPUT NUMBER
        {
            $$ = makeOperation (coutput_, 0, 0, 0, $2);
        }
    | WRITE REGISTER
        {
            $$ = makeOperation (write_, $2, 0, 0, 0);
        }
    | CWRITE REGISTER
        {
            $$ = makeOperation (cwrite_, $2, 0, 0, 0);
        }
    ;

%%
int yywrap () { return 1; } /* for flex: only one input file */

void yyerror (struct Instructions **myStmts, const char *s) {
    syntax_error++;
    fprintf(stderr, "Parser: '%s' around line %d.\n", s, yylineno);
    s = NULL;
}
