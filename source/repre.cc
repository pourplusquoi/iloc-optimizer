#include <vector>

#include "../headers/repre.h"
#include "../headers/struct.h"

using namespace std;

extern "C" {

struct Operation* makeOperation (enum OpCode code, size_t reg0, size_t reg1, size_t reg2, size_t constant) {
    return new Operation (code, reg0, reg1, reg2, constant, nullptr, nullptr);
}

struct Operation* makeBranch (enum OpCode code, size_t reg0, char* label1, char* label2) {
    return new Operation (code, reg0, 0, 0, 0, label1, label2);
}

struct Instruction* makeInstruction (char* label, struct Operation* op) {
    return new Instruction (label, op);
}

struct Instructions* makeInstructions (struct Instruction* inst) {
    Instructions* res = new Instructions ();
    res->insts.push_back (inst);
    return res;
}

void appendInstruction (struct Instructions* toMe, struct Instruction* inst) {
    toMe->insts.push_back (inst);
}

} // extern
