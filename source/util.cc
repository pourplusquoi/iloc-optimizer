#include <algorithm>
#include <iostream>
#include <queue>

#include "../headers/util.h"

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

Graph :: Graph (const vector <const Instruction*> &insts, const vector <size_t> &lead, 
    const vector <size_t> &last, const vector <pair <size_t, size_t>> &edgesIn) {

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
        vertices.push_back (headMap[lead[i]]);

    // now build edges of graph
    for (const auto &e : edgesIn)
        edges[tailMap[e.first]].insert (headMap[e.second]);
}

void Graph :: dfs (const string &idx, unordered_map <string, int> &onStack, 
    unordered_map <string, unordered_set <string>> *body) const {

    // when there is no edge coming out of 'idx'
    if (edges.find (idx) == edges.end ())
        return;
    
    // mark vertex idx as on stack
    onStack[idx] = 1;

    // exam each subsequent vertex
    for (const string &dstIdx : edges.at(idx)) {
        // when vertex dstIdx is on stack, means there is a loop
        if (onStack[dstIdx])
            (*body)[dstIdx].insert (idx);
        else dfs (dstIdx, onStack, body);
    }

    // recover stack in the end
    onStack[idx] = 0;
}

void Graph :: findLoop (vector <Loop> *loops) const {
    // initialize onStack such that no vertex is no stack
    unordered_map <string, int> onStack;
    for (const string &v : vertices)
        onStack[v] = 0;

    // perform depth first search from beginning node (vertex 0)
    unordered_map <string, unordered_set <string>> body;
    dfs ("START", onStack, &body);

    for (const auto &e : edges) {
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

void Graph :: reverseGraph (Graph *toMe) const {
    toMe->vertices = vertices;
    for (const auto &e : edges) {
        for (const string &v : e.second)
            (toMe->edges)[v].insert (e.first);
    }
}

void sortVertexEBB (const Graph &graph, const Graph &revGraph, 
    vector <string> *toMe) {

    unordered_set <string> exist;
    queue <string> ready;
    
    // first all EBB heads
    for (const string &v : revGraph.vertices) {
        if (revGraph.edges.find (v) == revGraph.edges.end () || 
            revGraph.edges.at (v).size () > 1) {
            exist.insert (v);
            ready.push (v);
        }
    }

    while (ready.size ()) {
        string vertex = ready.front ();
        ready.pop ();

        // apend that ready label to result
        toMe->push_back (vertex);

        // when the vertex do not have any child
        if (graph.edges.find (vertex) == graph.edges.end ())
            continue;

        for (const string &v : graph.edges.at (vertex)) {
            // when the child is not the head of a EBB
            if (exist.find (v) == exist.end ()) {
                exist.insert (v);
                ready.push (v);
            }
        }
    }
}

void copyInstructions (const vector <const Instruction*> &fromMe, 
    vector <const Instruction*> *toMe, size_t fromLine, size_t numLines) {

    for (size_t i = 0; i < numLines; i++)
        toMe->push_back (new Instruction (fromMe[fromLine + i]));
}

void modifyBranch (const vector <const Instruction*> fromMe, 
    vector <const Instruction*> *toMe, Graph *graph, 
    unordered_map <string, string> *dependency, 
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
        (*dependency)[oldLabel + suffix] = tarLabel;
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

bool isPowerOfTwo (size_t num) {
    if (num == 0)
        return false;
    while ((num & 1) == 0)
        num >>= 1;
    return num == 1;
}

size_t getPower (size_t num) {
    size_t res = 0;
    while ((num & 1) == 0) {
        res++;
        num >>= 1;
    }
    return res;
}

bool shiftOptimizable (const Instruction *inst) {
    if (inst->op->code == OpCode::multI_) {
        // when multiply zero or power of two
        if (inst->op->constant == 0 || isPowerOfTwo (inst->op->constant))
            return true;
        return false;
    }
    // divide power of two
    if (inst->op->code == OpCode::divI_ && isPowerOfTwo (inst->op->constant))
        return true;
    return false;
}

void writeInstsBack (const vector <const Instruction*> &fromMe, 
    vector <const Instruction*> *toMe, const unordered_set <size_t> &removal, 
    const unordered_map <size_t, RewriteInfo> &rewrite, size_t tempReg) {

    // maps the #line that needed to be re-written to the copy target 
    // variable that holds the value of pre-computed variable
    unordered_map <size_t, size_t> copyMap;

    // maps the #line after which memorization should be done
    // to the memorizing instruction to be inserted there
    unordered_map <size_t, Instruction*> reminder;

    // finish probing, start re-write
    for (size_t i = 0; i < fromMe.size (); i++) {

        // when the instruction is redundant, skip it
        if (removal.find (i) != removal.end ())
            continue;

        // when the result is needed in subsequent instructions
        // and cannot be optimized by shift
        if (rewrite.find (i) != rewrite.end () && !shiftOptimizable (fromMe[i])) {

            // the source variable used to assign other variables
            size_t srcReg = fromMe[i]->op->reg2;

            // allocate new variable if reuse is after memorization
            for (const size_t line : rewrite.at (i).second)
                copyMap[line] = (rewrite.at (i).first < line) ? tempReg : srcReg;

            // when memorization is necessary, since variable has been re-written
            if (rewrite.at (i).first < rewrite.at (i).second.back ()) {

                // build up the assignment instruction (memorize) to be inserted
                Instruction* inst = new Instruction ();
                inst->op = new Operation (OpCode::i2i_, fromMe[i]->op->reg2, 0, tempReg++, 0);

                // add the future assignment instruction (memorize) to reminder
                reminder[rewrite.at (i).first] = inst;
            }

            toMe->push_back (new Instruction (fromMe[i]));
        }

        // when the line need to be re-written
        else if (copyMap.find (i) != copyMap.end ()) {
            Instruction* inst = new Instruction ();
            inst->op = new Operation (OpCode::i2i_, copyMap[i], 0, fromMe[i]->op->reg2, 0);

            toMe->push_back (inst);
        }

        // when shift optimization is possible
        else if (shiftOptimizable (fromMe[i])) {
            if (fromMe[i]->op->code == OpCode::multI_) {

                // optimize multiplication with zero
                if (fromMe[i]->op->constant == 0)
                    toMe->push_back (new Instruction (nullptr, new Operation (
                        OpCode::loadI_, 0, 0, 0, 0)));
                
                // optimize multiplication with power of two
                else {
                    toMe->push_back (new Instruction (nullptr, new Operation (
                        OpCode::lshiftI_, fromMe[i]->op->reg0, 0, 
                        fromMe[i]->op->reg2, getPower (fromMe[i]->op->constant))));
                }
            }

            // optimize divide power of two
            else {
                toMe->push_back (new Instruction (nullptr, new Operation (
                    OpCode::rshiftI_, fromMe[i]->op->reg0, 0, 
                    fromMe[i]->op->reg2, getPower (fromMe[i]->op->constant))));
            }
        }

        else toMe->push_back (new Instruction (fromMe[i]));

        // finally, check the reminder if there is any pending instruction
        // if yes, insert that instruction to target 'toMe'
        if (reminder.find (i) != reminder.end ())
            toMe->push_back (reminder[i]);
    }
}

void writeInstsBack (const unordered_map <string, vector <const Instruction*>> &instMap, 
    vector <const Instruction*> *toMe, const vector <string> &order, 
    const unordered_map <string, string> &dependency) {

    unordered_set <string> beenWritten {order.back ()};

    // first append all the original blocks in order
    for (size_t i = 0; i < order.size () - 1; i++) {
        for (const Instruction *inst : instMap.at (order[i]))
            toMe->push_back (inst);
        beenWritten.insert (order[i]);
    }

    // regard all the latter label in dependency as been written
    for (const auto &fstSnd : dependency)
        beenWritten.insert (fstSnd.second);

    // append all the remaining instructions
    for (const auto &block : instMap) {
        // when the block has been written, then skip it
        if (beenWritten.find (block.first) != beenWritten.end ())
            continue;

        for (const Instruction *inst : block.second)
            toMe->push_back (inst);

        // deal with dependency, when there is a dependency
        if (dependency.find (block.first) != dependency.end ()) {
            const string &nextLabel = dependency.at (block.first);
            for (const Instruction *inst : instMap.at (nextLabel))
                toMe->push_back (inst);
            beenWritten.insert (nextLabel);
        }
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
    ins += dict[op->code - OpCode::nop_];
    if (op->code != OpCode::nop_)
        ins += " ";

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
