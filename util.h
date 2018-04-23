
#ifndef UTIL_H
#define UTIL_H

#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include "repre.h"
#include "struct.h"

using std::vector;
using std::string;
using std::pair;
using std::to_string;
using std::unordered_set;
using std::unordered_map;

/*  Opcode Map
    0 - add, sub, mult, div, lshift, rshift, and, or, cmp_LT, cmp_LE, cmp_GT, cmp_GE, cmp_EQ, cmp_NE
    1 - addI, subI, multI, divI, lshiftI, rshiftI, andI, orI
    2 - i2i, c2c, i2c, c2i
    3 - load, loadAI, loadAO, cload, cloadAI, cloadAO, read, cread
    4 - loadI
    5 - not
    9 - nop, halt, store, storeAI, storeAO, cstore, cstoreAI, cstoreAO, br, cbr, output, coutput, write, cwrite
*/
extern const vector <size_t> opcodeMap;

// dict maps opcode index to opcode name in ILOC code
extern const vector <string> dict;

// construct label map from parse result
void buildLabelMap (const vector <const Instruction*> &fromMe, unordered_map <string, size_t> &toMe);

// make hash tag of right hand side expression
inline string makeHashTag (OpCode code, size_t lhs, size_t rhs, size_t constant) {
    if (lhs > rhs) std::swap (lhs, rhs);
    return to_string (code - OpCode::nop_) + "$" + to_string (lhs) 
        + "$" + to_string (rhs) + "$" + to_string (constant);
}

struct Graph {
    // first is the head of block, second is the tail of block
    // use label name as vertex identifier
    // the first vertex is by default "START"
    vector <string> vertices;
    unordered_map <string, unordered_set <string>> edges;
};

struct Loop {
    string parent, head, tail;
    Loop (const string &p, const string &h, const string &t) :
        parent (p), head (h), tail (t) {}
};

// convert the result derived from buildCFG to Graph
void toGraph (const vector <const Instruction*> &insts, const vector <size_t> &lead, 
    const vector <size_t> &last, const vector <pair <size_t, size_t>> &edges, 
    Graph *graph);

// each entry in loops has the first element as the index of loop begin block
// and the second element as the index of loop end block in graph
void findLoop (const Graph &graph, vector <Loop> *loops);

// help to reverse edges in a directed graph
void reverseGraph (const Graph &fromMe, Graph *toMe);

// help to copy instructions from 'fromMe' to 'toMe'
void copyInstructions (const vector <const Instruction*> &fromMe, 
    vector <const Instruction*> *toMe, size_t fromLine, size_t numLines);

// help to modify branch when copy blocks in loop unrolling
void modifyBranch (const vector <const Instruction*> fromMe, 
    vector <const Instruction*> *toMe, Graph *graph, 
    const unordered_set <string> &involvedLabels, const string &oldLabel, 
    const string &newLabel, const string &suffix);

// help to finalize the branch-related instructions in the tail block
void finalizeTail (const vector <const Instruction*> &block, 
    vector <const Instruction*> *newBlock, size_t register, size_t step, 
    const string &label1, const string &label2);

// re-write the map of blocks to the destination vector of instructions
void writeInstsBack (const unordered_map <string, vector <const Instruction*>> &instMap, 
    vector <const Instruction*> *toMe, const vector <string> &order);

// get the # of next unused register
size_t nextUnusedReg (const vector <const Instruction*> &fromMe);

// translate encapsulated structure to ILOC code
string translate (const Instruction* me);

#endif
