
#ifndef STRUCT_H
#define STRUCT_H

#include <vector>
#include <string.h>
#include "repre.h"

using std::vector;

// description of a operation
struct Operation {
    OpCode code;
    
    size_t reg0;
    size_t reg1;
    size_t reg2;
    
    size_t constant;

    char* label1;
    char* label2;

    Operation (OpCode codeIn=OpCode::nop_, size_t reg0In=0, size_t reg1In=0, 
        size_t reg2In=0, size_t constantIn=0, 
        const char* label1In=nullptr, const char*label2In=nullptr) : 
        code (codeIn), reg0 (reg0In), reg1 (reg1In), 
        reg2 (reg2In), constant (constantIn) {
            
            if (label1In != nullptr)
                label1 = strdup (label1In);
            else label1 = nullptr;

            if (label2In != nullptr)
                label2 = strdup (label2In);
            else label2 = nullptr;
        }

    ~Operation () {
        if (label1 != nullptr) {
            delete [] label1;
            label1 = nullptr;
        }
        if (label2 != nullptr) {
            delete [] label2;
            label2 = nullptr;
        }
    }
};

// description of operation with label
struct Instruction {
    char* label;
    Operation* op;

    Instruction () {
        label = nullptr;
        op = nullptr;
    }

    // for nop in label case
    Instruction (const char* labelIn) {
        label = strdup (labelIn);
        op = new Operation ();
    }

    Instruction (char* labelIn, Operation* opIn) : 
        label (labelIn), op (opIn) {}

    Instruction (const Instruction* inst) {
        if (inst->label != nullptr)
            label = strdup (inst->label);
        else label = nullptr;

        Operation* tar = inst->op;
        op = new Operation (tar->code, tar->reg0, tar->reg1, tar->reg2, 
            tar->constant, tar->label1, tar->label2);
    }

    ~Instruction () {
        if (label != nullptr) {
            delete [] label;
            label = nullptr;
        }
        if (op != nullptr) {
            delete op;
            op = nullptr;
        }
    }
};

struct Instructions {
    vector <const Instruction*> insts;

    ~Instructions () {
        for (const Instruction* inst : insts) {
            delete inst;
            inst = nullptr;
        }
    }
};

#endif
