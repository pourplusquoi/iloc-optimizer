
#ifndef OPTIM_H
#define OPTIM_H

#include <vector>
#include <string>
#include <unordered_map>
#include "repre.h"

using std::vector;
using std::string;
using std::pair;
using std::unordered_map;

void buildCFG (const vector <const Instruction*> &fromMe, vector <size_t> *lead, 
    vector <size_t> *last, vector <pair <size_t, size_t>> *edges=nullptr);

void valueNumbering (const vector <const Instruction*> &fromMe, 
    vector <const Instruction*> *toMe, 
    const vector <size_t> &lead, const vector <size_t> &last);

void loopUnrolling (const vector <const Instruction*> &fromMe, 
    vector <const Instruction*> *toMe, 
    const vector <size_t> &lead, const vector <size_t> &last, 
    const vector <pair <size_t, size_t>> edges, size_t unrollBy=4);

void generateCode (const vector <const Instruction*> fromMe, FILE *writeToMe);

void freeMemory (vector <const Instruction*> &fromMe);

#endif
