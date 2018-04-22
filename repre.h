
#ifndef REPRE_H
#define REPRE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum OpCode {
    nop_, add_, addI_, sub_, subI_, mult_, multI_, div_, divI_, 
    lshift_, lshiftI_, rshift_, rshiftI_, and_, andI_, or_, orI_, not_, 

    loadI_, load_, loadAI_, loadAO_, cload_, cloadAI_, cloadAO_, 
    store_, storeAI_, storeAO_, cstore_, cstoreAI_, cstoreAO_, 
    i2i_, c2c_, i2c_, c2i_, 

    br_, cbr_, cmp_LT_, cmp_LE_, cmp_GT_, cmp_GE_, cmp_EQ_, cmp_NE_, halt_, 

    read_, cread_, output_, coutput_, write_, cwrite_
};

// description of a operation
struct Operation;

// description of operation with label
struct Instruction;

// a number of statements
struct Instructions;

struct Operation*  makeOperation (enum OpCode code, size_t reg0, size_t reg1, size_t reg2, size_t constant);
struct Operation*  makeBranch (enum OpCode code, size_t reg0, char* label1, char* label2);

struct Instruction*  makeInstruction (char* label, struct Operation* op);
struct Instructions* makeInstructions (struct Instruction* inst);

void appendInstruction (struct Instructions* toMe, struct Instruction* inst);

#ifdef __cplusplus
}
#endif

#endif
