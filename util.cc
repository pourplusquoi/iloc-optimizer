
#include <iostream>
#include <algorithm>
#include "util.h"

using namespace std;

const vector <size_t> opcodeMap {
    9, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 5, 4, 3, 
    3, 3, 3, 3, 3, 9, 9, 9, 9, 9, 9, 2, 2, 2, 2, 9, 9, 0, 0, 0, 
    0, 0, 0, 9, 3, 3, 9, 9, 9, 9
};

const vector <string> dict {
    "nop", "add", "addI", "sub", "subI", "mult", "multI", "div", "divI", 
    "lshift", "lshiftI", "rshift", "rshiftI", "and", "andI", "or", "orI", 
    "not", "loadI", "load", "loadAI", "loadAO", "cload", "cloadAI", "cloadAO", 
    "store", "storeAI", "storeAO", "cstore", "cstoreAI", "cstoreAO", 
    "i2i", "c2c", "i2c", "c2i", "br", "cbr", "cmp_LT", "cmp_LE", "cmp_GT", 
    "cmp_GE", "cmp_EQ", "cmp_NE", "halt", "read", "cread", "output", 
    "coutput", "write", "cwrite"
};

void buildLabelMap (const vector <const Instruction*> &fromMe, 
    unordered_map <string, size_t> &toMe) {
    
    size_t count = 0;
    for (const Instruction* inst: fromMe) {
        // the instruction has label
        if (inst->label != nullptr) {
            toMe[string (inst->label)] = count;
        }
        count++;
    }
}

void toGraph (const vector <const Instruction*> &insts, const vector <size_t> &lead, 
    const vector <size_t> &last, const vector <pair <size_t, size_t>> &edges, 
    Graph *graph) {

    // maps #line to label name
    // the first line has default label "START"
    unordered_map <size_t, string> headMap, tailMap;
    headMap[lead[0]] = "START";
    tailMap[last[0]] = "START";

    size_t numBlocks = lead.size ();
    for (size_t i = 1; i < numBlocks; i++) {
        string label (insts[lead[i]]->label);
        headMap[lead[i]] = label;
        tailMap[last[i]] = label;
    }

    // first build vertices of graph
    for (size_t i = 0; i < numBlocks; i++)
        (graph->vertices).push_back (headMap[lead[i]]);

    // now build edges of graph
    for (const auto &e : edges)
        (graph->edges)[tailMap[e.first]].insert (headMap[e.second]);
}

void dfs (const Graph &graph, const string &idx, unordered_map <string, int> &onStack, 
    unordered_map <string, unordered_set <string>> *body) {

    // when there is no edge coming out of 'idx'
    if (graph.edges.find (idx) == graph.edges.end ())
        return;
    
    // mark vertex idx as on stack
    onStack[idx] = 1;

    // exam each subsequent vertex
    for (const string &dstIdx : graph.edges.at(idx)) {
        // when vertex dstIdx is on stack, means there is a loop
        if (onStack[dstIdx])
            (*body)[dstIdx].insert (idx);
        else dfs (graph, dstIdx, onStack, body);
    }

    // recover stack in the end
    onStack[idx] = 0;
}

void findLoop (const Graph &graph, vector <Loop> *loops) {
    
    // initialize onStack such that no vertex is no stack
    unordered_map <string, int> onStack;
    for (const string &v : graph.vertices)
        onStack[v] = 0;

    // perform depth first search from beginning node (vertex 0)
    unordered_map <string, unordered_set <string>> body;
    dfs (graph, "START", onStack, &body);

    for (const auto &e : graph.edges) {
        for (const string &v : e.second) {
            // find the vertex, s.t. its child 'v' is the loop entry
            if (body.find (v) != body.end ()) {

                // but the parent of head can not be the tail of loop
                auto &tails = body[v];
                if (tails.find (e.first) != tails.end ())
                    continue;
                
                for (const string &tail : body[v])
                    loops->push_back (Loop (e.first, v, tail));
                // a block cannot be the parent of more than one loops
                break;
            }
        }
    }
}

void reverseGraph (const Graph &fromMe, Graph *toMe) {
    toMe->vertices = fromMe.vertices;
    for (const auto &e : fromMe.edges) {
        for (const string &v : e.second)
            (toMe->edges)[v].insert (e.first);
    }
}

void copyInstructions (const vector <const Instruction*> &fromMe, 
    vector <const Instruction*> *toMe, size_t fromLine, size_t numLines) {

    for (size_t i = 0; i < numLines; i++)
        toMe->push_back (new Instruction (fromMe[fromLine + i]));
}

void modifyBranch (const vector <const Instruction*> fromMe, 
    vector <const Instruction*> *toMe, Graph *graph, 
    const unordered_set <string> &involved, const string &oldLabel, 
    const string &newLabel, const string &suffix) {

    // maintain the graph
    graph->vertices.push_back (newLabel);

    // when the edge is a natural one, i.e. no cr/cbr in the end
    OpCode brType = fromMe.back ()->op->code;
    if (brType != OpCode::br_ && brType != OpCode::cbr_) {
        toMe->push_back (new Instruction (fromMe.back ()));

        // since it is a natural edge, target block must be involved
        string tarLabel = *(graph->edges)[oldLabel].begin () + suffix;

        // add the branch instruction to jump to target        
        toMe->push_back (new Instruction (nullptr, new Operation (
            OpCode::br_, 0, 0, 0, 0, tarLabel.c_str ())));

        // maintain the graph
        (graph->edges)[newLabel].insert (tarLabel);
        return;
    }

    // when the label is involved in loop, mangle them
    string label1 = string (fromMe.back ()->op->label1);
    if (involved.find (label1) != involved.end ())
        label1 += suffix;

    // maintain the graph
    (graph->edges)[newLabel].insert (label1);

    // this is a conditional branch in this case
    if (brType == OpCode::cbr_) {
        // we need another label in conditional branch
        string label2 = string (fromMe.back ()->op->label2);
        if (involved.find (label2) != involved.end ())
            label2 += suffix;

        // build the conditional branch instruction
        toMe->push_back (new Instruction (nullptr, new Operation (
            OpCode::cbr_, fromMe[fromMe.size () - 2]->op->reg2, 0, 0, 0, 
            label1.c_str (), label2.c_str ())));

        // add second edge into graph
        (graph->edges)[newLabel].insert (label2);
    }

    else { // this is a jump in this case
        toMe->push_back (new Instruction (nullptr, new Operation (
            OpCode::br_, 0, 0, 0, 0, label1.c_str ())));
    }
}

void finalizeTail (const vector <const Instruction*> &block, 
    vector <const Instruction*> *newBlock, size_t reg, size_t step, 
    const string &label1, const string &label2) {
    
    size_t size = block.size ();
    
    const Instruction* icrInst = block[size - 3];
    const Instruction* cmpInst = block[size - 2];

    // build the increment instruction
    newBlock->push_back (new Instruction (nullptr, new Operation (
        icrInst->op->code, icrInst->op->reg0, 0, reg, step)));

    // build the comparison instruction
    newBlock->push_back (new Instruction (nullptr, new Operation (
        cmpInst->op->code, reg, cmpInst->op->reg1, cmpInst->op->reg2)));

    // the last instruction of the tail block must be a conditional branch
    newBlock->push_back (new Instruction (nullptr, new Operation (
        OpCode::cbr_, cmpInst->op->reg2, 0, 0, 0, 
        label1.c_str (), label2.c_str ())));
}

void writeInstsBack (const unordered_map <string, vector <const Instruction*>> &instMap, 
    vector <const Instruction*> *toMe, const vector <string> &order) {

    unordered_set <string> beenWritten {order.back ()};

    // first append all the original blocks in order
    for (size_t i = 0; i < order.size () - 1; i++) {
        for (const Instruction *inst : instMap.at (order[i]))
            toMe->push_back (inst);
        beenWritten.insert (order[i]);
    }

    // append all the remaining instructions
    for (const auto &block : instMap) {
        // when the block has been written, then skip it
        if (beenWritten.find (block.first) != beenWritten.end ())
            continue;

        for (const Instruction *inst : block.second)
            toMe->push_back (inst);
    }

    // append the last block in the end
    for (const Instruction *inst : instMap.at (order.back ()))
        toMe->push_back (inst);
}

size_t nextUnusedReg (const vector <const Instruction*> &fromMe) {
    size_t nextReg = 0;
    for (const Instruction* inst : fromMe) {
        nextReg = max (nextReg, inst->op->reg0 + 1);
        nextReg = max (nextReg, inst->op->reg1 + 1);
        nextReg = max (nextReg, inst->op->reg2 + 1);
    }
    return nextReg;
}

string translate (const Instruction* me) {
    string ins;
    
    // append label if any
    if (me->label != nullptr)
        ins += string (me->label) + ":\t";
    else ins += "\t";

    Operation* op = me->op;

    // append opcode
    ins += dict[op->code - OpCode::nop_] + " ";

    size_t key = opcodeMap[op->code - OpCode::nop_];
    switch (key) {
        case 0:
            ins += "r" + to_string (op->reg0) + ", ";
            ins += "r" + to_string (op->reg1) + " => ";
            ins += "r" + to_string (op->reg2);
            break;
        
        case 1:
            ins += "r" + to_string (op->reg0) + ", ";
            ins += to_string (op->constant) + " => ";
            ins += "r" + to_string (op->reg2);
            break;
        
        case 2:
        case 5:
            ins += "r" + to_string (op->reg0) + " => ";
            ins += "r" + to_string (op->reg2);
            break;
        
        case 3:
            if (op->code == OpCode::read_ || op->code == OpCode::cread_)
                ins += "=> r" + to_string (op->reg2);

            else { // load, cload, loadAI, cloadAI, loadAO, cloadAO
                ins += "r" + to_string (op->reg0);
                
                if (op->code == OpCode::loadAI_ || op->code == OpCode::cloadAI_)
                    ins += ", " + to_string (op->constant);
                
                else if (op->code == OpCode::loadAO_ || op->code == OpCode::cloadAO_)
                    ins += ", r" + to_string (op->reg1);
                
                ins += " => r" + to_string (op->reg2);
            }
            break;
        
        case 4:
            ins += to_string (op->constant) + " => ";
            ins += "r" + to_string (op->reg2);
        
        case 9:
            if (op->code == OpCode::store_ || op->code == OpCode::cstore_) {
                ins += "r" + to_string (op->reg0) + " => ";
                ins += "r" + to_string (op->reg1);
            }

            else if (op->code == OpCode::storeAI_ || op->code == OpCode::cstoreAI_) {
                ins += "r" + to_string (op->reg0) + " => ";
                ins += "r" + to_string (op->reg1) + ", ";
                ins += to_string (op->constant);
            }

            else if (op->code == OpCode::storeAO_ || op->code == OpCode::cstoreAO_) {
                ins += "r" + to_string (op->reg0) + " => ";
                ins += "r" + to_string (op->reg1) + ", ";
                ins += "r" + to_string (op->reg2);
            }

            else if (op->code == OpCode::br_)
                ins += "-> " + string (op->label1);

            else if (op->code == OpCode::cbr_) {
                ins += "r" + to_string (op->reg0) + " -> ";
                ins += string (op->label1) + ", ";
                ins += string (op->label2);
            }

            else if (op->code == OpCode::output_ || op->code == OpCode::coutput_)
                ins += to_string (op->constant);

            else if (op->code == OpCode::write_ || op->code == OpCode::cwrite_)
                ins += "r" + to_string (op->reg0);
            
            break;
        
        default: break;
    }

    // end the instruction with '\n'
    ins += "\n";
    return ins;
}
