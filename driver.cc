
#include <iostream>
#include "struct.h"
#include "optim.h"

using namespace std;

extern int syntax_error;
extern FILE *yyin, *yyout;

extern "C" int yyparse (Instructions **);

int main (int argc, char** argv) {

    string error = "Incorrect input format.\n./opt [-v][-u][-i] file.i\n";
    string number = "-v: value numbering\n";
    string unroll = "-u: loop unrolling\n";
    string motion = "-i: loop-invariant code motion\n";

    if (argc < 3 || argc > 5) {
        cout << (error + number + unroll + motion);
        exit (0);
    }

    vector <string> options;
    for (size_t i = 1; i < argc - 1; i++) {
        string option = string (argv[i]);
        
        if (option != "-v" && option != "-u" && option != "-i") {
            cout << (error + number + unroll + motion);
            exit (0);
        }

        if (option == "-i") {
            cout << "-i: code motion not implemented\n";
            exit (0);
        }
        
        else options.push_back (option);
    }

    char* filename = argv[argc - 1];

    if (strlen (filename) == 2 && filename[0] == '-') {
        cout << (error + number + unroll + motion);
        exit (0);
    }

    yyout = stdout;

    yyin = fopen ((const char *) filename, "r");
    
    if (yyin == nullptr) {
        cout << "Cannot open file '" << string (filename) << "'.\n";
        exit (0);
    }

    Instructions *final = nullptr;
    if (yyparse (&final)) {
        cout << "Parse stopped with " << syntax_error << " error(s).\n";
        if (final != nullptr)
            delete final;
        exit (0);
    }

    vector <const Instruction*> insts = final->insts;

    vector <const Instruction*> src, dst;
    for (const Instruction* inst : insts)
        src.push_back (new Instruction (inst));

    for (const auto &option : options) {
        
        if (option == "-v") {
            vector <size_t> lead, last;
            vector <pair <size_t, size_t>> edges;
            buildCFG (src, &lead, &last, &edges);
            valueNumbering (src, &dst, lead, last, edges);

            freeMemory (src);
            src = std::move (dst);
        }

        else if (option == "-u") {
            vector <size_t> lead, last;
            vector <pair <size_t, size_t>> edges;
            buildCFG (src, &lead, &last, &edges);
            loopUnrolling (src, &dst, lead, last, edges);

            freeMemory (src);
            src = std::move (dst);
        }

        else if (option == "-i") {}
    }

    generateCode (src, yyout);
    freeMemory (src);

    if (final != nullptr)
        delete final;

    return 0;
}
