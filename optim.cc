
#include <iostream>
#include <queue>
#include <cmath>
#include <algorithm>
#include <unordered_set>
#include <limits.h>
#include "optim.h"
#include "struct.h"
#include "util.h"

using namespace std;

void buildCFG (const vector <const Instruction*> &fromMe, vector <size_t> *lead, 
    vector <size_t> *last, vector <pair <size_t, size_t>> *edges) {

    unordered_map <string, size_t> labelMap;
    buildLabelMap (fromMe, labelMap);

    size_t next = 0;
    lead->push_back (next++);

    size_t num = fromMe.size ();
    for (size_t i = 0; i < num; i++) {
        const Operation* operation = fromMe[i]->op;
        
        if (operation->code == OpCode::br_) {
            size_t dstHead = labelMap[string (operation->label1)];
            
            lead->push_back (dstHead);

            if (edges != nullptr)
                edges->push_back (make_pair (i, dstHead));
        }

        else if (operation->code == OpCode::cbr_) {
            size_t dstHead1 = labelMap[string (operation->label1)];
            size_t dstHead2 = labelMap[string (operation->label2)];
            
            lead->push_back (dstHead1);
            lead->push_back (dstHead2);

            if (edges != nullptr) {
                edges->push_back (make_pair (i, dstHead1));
                edges->push_back (make_pair (i, dstHead2));
            }
        }
    }

    // eliminate duplicate leads
    std::sort (lead->begin (), lead->end ());
    lead->erase (std::unique (lead->begin (), lead->end ()), lead->end ());
    
    unordered_set <size_t> leadSet (lead->begin (), lead->end ());

    size_t size = lead->size();
    for (size_t i = 0; i < size; i++) {
        size_t j = (*lead)[i] + 1;
        
        while (j < num && leadSet.find (j) == leadSet.end ()) {
            j++;
        }

        last->push_back (j - 1);
    }

    // add natural edges, i.e. not br/cbr at the end of block
    for (size_t line : (*last)) {
        OpCode code = fromMe[line]->op->code;
        if (code != OpCode::br_ && code != OpCode::cbr_)
            edges->push_back (make_pair (line, line + 1));
    }
}

void valueNumbering (const vector <const Instruction*> &fromMe, size_t lead, size_t last, 
    vector <const Instruction*> *toMe, size_t nextReg) {

    // memorize the biggest register number
    size_t tempReg = nextReg;

    // maps variable or constant or expression to value number
    unordered_map <string, size_t> valDict;

    // maps value number to the #line where the variable is assigned
    // and the #line where the variable is re-written
    unordered_map <size_t, pair <size_t, size_t>> varDict;

    // maps old variable name to new variable name
    unordered_map <string, string> renameMap;

    // redundent instructions
    unordered_set <size_t> removal;

    // instructions needs to be re-written
    // maps the #line of variable need to be memorized
    // to #line after which memorization should be done
    // to #lines of variables that need this value
    unordered_map <size_t, pair <size_t, vector <size_t>>> rewrite;

    size_t nextVal = 0;
    for (size_t i = lead; i <= last; i++) {
        OpCode code = fromMe[i]->op->code;

        if (opcodeMap[code - OpCode::nop_] > 5)
            continue;

        string reg0 = "r" + to_string (fromMe[i]->op->reg0);
        string reg1 = "r" + to_string (fromMe[i]->op->reg1);
        string reg2 = "r" + to_string (fromMe[i]->op->reg2);
        string constant = to_string (fromMe[i]->op->constant);

        // remember the initial name of variable 'reg2'
        string reg2Init = reg2;

        // when the variable has been renamed, get its most recent name
        reg0 = (renameMap.find (reg0) == renameMap.end ()) ? reg0 : renameMap[reg0];
        reg1 = (renameMap.find (reg1) == renameMap.end ()) ? reg1 : renameMap[reg1];
        reg2 = (renameMap.find (reg2) == renameMap.end ()) ? reg2 : renameMap[reg2];

        // number the values
        size_t key = opcodeMap[code - OpCode::nop_];
        switch (key) {
            
            case 0: // opcode on 3 registers
            case 1: // opcode on 2 registers and 1 constant
            case 5: // opcode 'not'
            {    
                string tag;
                
                // when variable 'reg0' has not been used
                if (valDict.find (reg0) == valDict.end ())
                    valDict[reg0] = ++nextVal;

                if (key == 0) {
                    // when variable 'reg1' has not been used
                    if (valDict.find (reg1) == valDict.end ())
                        valDict[reg1] = ++nextVal;
                    tag = makeHashTag (code, valDict[reg0], valDict[reg1], 0);
                }

                else if (key == 1) {
                    // when constant number 'constant' has not been used
                    if (valDict.find (constant) == valDict.end ())
                        valDict[constant] = ++nextVal;
                    tag = makeHashTag (code, valDict[reg0], 0, valDict[constant]);
                }

                // the case where opcode is 'not'
                else tag = makeHashTag (code, valDict[reg0], 0, 0);

                // when the expression has been evaluated
                if (valDict.find (tag) != valDict.end ()) {
                    size_t rvalue = valDict[tag];

                    // when variable 'reg2' has not been assigned yet
                    if (valDict.find (reg2) == valDict.end ())
                        valDict[reg2] = rvalue;

                    // when variable 'reg2' changes value, rename it
                    else if (valDict[reg2] != rvalue) {
                        // memorize the #line where the variable is re-written
                        // note that only the smallest #line is kept
                        if (varDict[valDict[reg2]].second == INT_MAX)
                            varDict[valDict[reg2]].second = i - 1;

                        string newName = "r" + to_string (nextReg++);
                        renameMap[reg2Init] = newName;
                        valDict[newName] = rvalue;
                    }

                    // when the instruction has no effect
                    else removal.insert (i);

                    // when the opcode is 'mult' or 'div' or 'multI' or 'divI'
                    if (code == OpCode::mult_ || code == OpCode::div_ || 
                        code == OpCode::multI_ || code == OpCode::divI_) {
                        auto &lines = varDict[rvalue];
                        // lines.first is the entry, the #line where variable is originally assigned
                        // lines.second is the #line after which the variable should be memorized
                        // rewrite[lines.first].second stores #lines of variables that need the value
                        rewrite[lines.first].first = lines.second;
                        rewrite[lines.first].second.push_back (i);
                    }
                }

                // when the expression has not been evaluated
                else {
                    size_t lvalue = ++nextVal;
                    valDict[tag] = lvalue;

                    // when variable 'reg2' has not been assigned yet
                    if (valDict.find (reg2) == valDict.end ())
                        valDict[reg2] = lvalue;

                    // when variable 'reg2' changes value, rename it
                    else if (valDict[reg2] != lvalue) {
                        // memorize the #line where the variable is re-written
                        // note that only the smallest #line is kept
                        if (varDict[valDict[reg2]].second == INT_MAX)
                            varDict[valDict[reg2]].second = i - 1;

                        string newName = "r" + to_string (nextReg++);
                        renameMap[reg2Init] = newName;
                        valDict[newName] = lvalue;
                    }

                    varDict[lvalue] = make_pair (i, INT_MAX);
                }

                break;
            }

            case 2: // opcode 'i2i', 'c2c', 'i2c', 'c2i'
            case 4: // opcode 'loadI'
            {
                size_t rvalue;

                if (key == 2) {
                    // when variable 'reg0' has not been used
                    if (valDict.find (reg0) == valDict.end ())
                        valDict[reg0] = ++nextVal;
                    rvalue = valDict[reg0];
                }

                else { // the case where opcode is 'loadI'
                    // when constant number 'constant' has not been used
                    if (valDict.find (constant) == valDict.end ())
                        valDict[constant] = ++nextVal;
                    rvalue = valDict[constant];
                }

                // when variable 'reg2' has not been assigned yet
                if (valDict.find (reg2) == valDict.end ())
                    valDict[reg2] = rvalue;

                // when variable 'reg2' changes value, rename it
                else if (valDict[reg2] != rvalue) {
                    // memorize the #line where the variable is re-written
                    // note that only the smallest #line is kept
                    if (varDict[valDict[reg2]].second == INT_MAX)
                        varDict[valDict[reg2]].second = i - 1;

                    string newName = "r" + to_string (nextReg++);
                    renameMap[reg2Init] = newName;
                    valDict[newName] = rvalue;
                }

                // when the instruction has no effect
                else removal.insert (i);

                break;
            }

            case 3: // opcode 'load', 'loadAI', 'loadAO', 'cload', 
                    // 'cloadAI', 'cloadAO', 'read', 'cread'
            {
                size_t lvalue = ++nextVal;

                // when variable 'reg2' has not been assigned yet
                if (valDict.find (reg2) == valDict.end())
                    valDict[reg2] = lvalue;

                // when variable 'reg2' has been assigned before
                else {
                    // memorize the #line where the variable is re-written
                    // note that only the smallest #line is kept
                    if (varDict[valDict[reg2]].second == INT_MAX)
                        varDict[valDict[reg2]].second = i - 1;
                    
                    // since we don't know the loaded value, rename variable 'reg2' directly
                    string newName = "r" + to_string (nextReg++);
                    renameMap[reg2Init] = newName;
                    valDict[newName] = lvalue;
                }

                varDict[lvalue] = make_pair (i, INT_MAX);

                break;
            }

            default: break;
        } // end of switch
    } // end of for-loop

    // maps the #line that needed to be re-written to the copy target 
    // variable that holds the value of pre-computed variable
    unordered_map <size_t, size_t> copyMap;

    // maps the #line after which memorization should be done
    // to the memorizing instruction to be inserted there
    unordered_map <size_t, Instruction*> reminder;

    // finish probing, start re-write
    for (size_t i = lead; i <= last; i++) {

        // when the instruction is redundant, skip it
        if (removal.find (i) != removal.end ())
            continue;

        // when the result is needed in subsequent instructions
        if (rewrite.find (i) != rewrite.end ()) {

            // the source variable used to assign other variables
            size_t srcReg = fromMe[i]->op->reg2;

            // allocate new variable if reuse is after memorization
            for (const size_t line : rewrite[i].second)
                copyMap[line] = (rewrite[i].first < line) ? tempReg : srcReg;

            // when memorization is necessary, since variable has been re-written
            if (rewrite[i].first < rewrite[i].second.back ()) {

                // build up the assignment instruction (memorize) to be inserted
                Instruction* inst = new Instruction ();
                inst->op = new Operation (OpCode::i2i_, fromMe[i]->op->reg2, 0, tempReg++, 0);

                // add the future assignment instruction (memorize) to reminder
                reminder[rewrite[i].first] = inst;
            }

            toMe->push_back (new Instruction (fromMe[i]));
        }

        // when the line need to be re-written
        else if (copyMap.find (i) != copyMap.end ()) {
            Instruction* inst = new Instruction ();
            inst->op = new Operation (OpCode::i2i_, copyMap[i], 0, fromMe[i]->op->reg2, 0);

            toMe->push_back (inst);
        }

        else toMe->push_back (new Instruction (fromMe[i]));

        // finally, check the reminder if there is any pending instruction
        // if yes, insert that instruction to target 'toMe'
        if (reminder.find (i) != reminder.end ())
            toMe->push_back (reminder[i]);
    }
}

void loopUnrolling (unordered_map <string, vector <const Instruction*>> &instMap, 
    Graph &graph, Graph &revGraph, const Loop &loop, 
    size_t nextReg, size_t &nextLabel, size_t unrollBy) {

    // decide whether we unroll the loop
    vector <const Instruction*> &tailBlock = instMap[loop.tail];

    // there must be "[addI, subI, multI, divI, lshiftI, rshiftI] => 
    // [cmp_LT, cmp_LE, cmp_GT, cmp_GE] => cbr" sequence at the end
    size_t tsize = tailBlock.size ();
    if (tsize < 3 || tailBlock[tsize - 1]->op->code != OpCode::cbr_) return;
    
    OpCode cmp = tailBlock[tsize - 2]->op->code;
    OpCode loopType = tailBlock[tsize - 3]->op->code;
    if (cmp < OpCode::cmp_LT_ || cmp > OpCode::cmp_GE_ || (loopType != OpCode::addI_ && 
        loopType != OpCode::subI_ && loopType != OpCode::multI_ && loopType != OpCode::divI_ && 
        loopType != OpCode::lshiftI_ && loopType != OpCode::rshiftI_)) return;

    // get the looping step
    size_t loopStep = tailBlock[tsize - 3]->op->constant;

    // get the looping variable
    size_t loopVar = tailBlock[tsize - 3]->op->reg2;

    // find all the blocks that the loop affects using reverse graph
    unordered_set <string> involvedLabels;
    queue <string> q;
    
    q.push (loop.tail);
    involvedLabels.insert (loop.tail);
    
    while (q.size ()) {
        string label = q.front ();
        q.pop ();

        // when the loop head is visited, don't visit its parents
        if (label == loop.head)
            continue;

        for (const string &par : revGraph.edges[label]) {
            if (involvedLabels.find (par) != involvedLabels.end ())
                continue;
            involvedLabels.insert (par);
            q.push (par);
        }
    }

    // if too many blocks are involved in loop
    // we just give up unrolling it
    if (involvedLabels.size () > 20)
        return;

    // when the looping variable is assigned anywhere in loop, stop unrolling
    for (const string &label : involvedLabels) {
        size_t bsize = instMap[label].size ();
        for (size_t i = 0; i < bsize; i++) {

            // the looping variable should only be modifies here
            if (label == loop.tail && i == bsize - 3)
                continue;
            
            // when the operation is assignment and target is looping variable
            const Instruction *inst = instMap[label][i];
            if (opcodeMap[inst->op->code] != 9 && inst->op->reg2 == loopVar)
                return;
        }
    }

    // get the reference to the parent block and the head block
    // and get the size of each block
    vector <const Instruction*> &parBlock = instMap[loop.parent];
    vector <const Instruction*> &headBlock = instMap[loop.head];
    size_t psize = parBlock.size (), hsize = headBlock.size ();

    // the new parent for remaining case (< 'unrollBy')
    string extraParLabel = loop.parent + "X" + to_string (nextLabel);
    vector <const Instruction*> extraParBody;

    // add 'nop' after the label
    // and copy comparison and conditional branch instruction
    // finally add the extra parent to the instruction map
    extraParBody.push_back (new Instruction (extraParLabel.c_str ()));
    copyInstructions (parBlock, &extraParBody, psize - 2, 2);
    instMap[extraParLabel] = std::move (extraParBody);

    // maintain the graph
    string exitLabel = string (parBlock.back ()->op->label2);
    graph.vertices.push_back (extraParLabel);    
    graph.edges[extraParLabel].insert (loop.head);
    graph.edges[extraParLabel].insert (exitLabel);

    // calculate the new step value
    size_t newStep = unrollBy * loopStep;
    if (loopType == OpCode::multI_ || loopType == OpCode::divI_)
        newStep = pow (loopStep, unrollBy);

    // copy the loop body for 'unrollBy' times
    for (const string &label : involvedLabels) {
        if (label == loop.head || label == loop.tail)
            continue;

        // get the reference to the old body block
        vector <const Instruction*> &bodyBlock = instMap[label];
        size_t bsize = bodyBlock.size ();
        
        for (size_t i = 0; i < unrollBy; i++) {
            string suffix = "X" + to_string (nextLabel + i);
            string newLabel = label + suffix;

            // copy instructions from original block
            vector <const Instruction*> newBody;
            newBody.push_back (new Instruction (newLabel.c_str ()));
            copyInstructions (bodyBlock, &newBody, 1, bsize - 2);
            modifyBranch (bodyBlock, &newBody, &graph, 
                involvedLabels, label, newLabel, suffix);

            instMap[newLabel] = std::move (newBody);
        } // end of for-loop
    } // end of for-loop

    // this is the label of new head of unrolled loop
    string newHeadLabel = loop.head + "X" + to_string (nextLabel);
    
    if (loop.head != loop.tail) {
        // coalesce the head block and the tail block in the body of unrolled loop
        for (size_t i = 0; i < unrollBy - 1; i++) {
            string linkLabel = loop.tail + "X" + to_string (nextLabel + i);
            vector <const Instruction*> linkBody;
            
            linkBody.push_back (new Instruction (linkLabel.c_str ()));
            copyInstructions (tailBlock, &linkBody, 1, tsize - 3);
            copyInstructions (headBlock, &linkBody, 1, hsize - 2);
            modifyBranch (headBlock, &linkBody, &graph, involvedLabels, 
                loop.head, linkLabel, "X" + to_string (nextLabel + i + 1));

            instMap[linkLabel] = std::move (linkBody);
        } // end of for-loop

        // deal with the entry (new head) block
        vector <const Instruction*> newHeadBody;

        newHeadBody.push_back (new Instruction (newHeadLabel.c_str ()));
        copyInstructions (headBlock, &newHeadBody, 1, hsize - 2);
        modifyBranch (headBlock, &newHeadBody, &graph, involvedLabels, 
            loop.head, newHeadLabel, "X" + to_string (nextLabel));

        instMap[newHeadLabel] = std::move (newHeadBody);

        // deal with the exit (new tail) block
        string newTailLabel = loop.tail + "X" + to_string (nextLabel + unrollBy - 1);
        vector <const Instruction*> newTailBody;
        
        newTailBody.push_back (new Instruction (newTailLabel.c_str ()));
        copyInstructions (tailBlock, &newTailBody, 1, tsize - 3);
        finalizeTail (tailBlock, &newTailBody, nextReg, newStep, 
            newHeadLabel, extraParLabel);

        // maintail the graph
        graph.vertices.push_back (newTailLabel);
        graph.edges[newTailLabel].insert (newHeadLabel);
        graph.edges[newTailLabel].insert (extraParLabel);

        instMap[newTailLabel] = std::move (newTailBody);

        nextLabel += unrollBy;
    } // end of if (loop.head != loop.tail)

    else { // when loop.head == loop.tail
        string newHeadLabel = loop.head + "X" + to_string (nextLabel);
        vector <const Instruction*> newHeadBody;

        newHeadBody.push_back (new Instruction (newHeadLabel.c_str ()));
        for (size_t i = 0; i < unrollBy; i++)
            copyInstructions (headBlock, &newHeadBody, 1, hsize - 3);

        finalizeTail (headBlock, &newHeadBody, nextReg, newStep, 
            newHeadLabel, extraParLabel);

        // maintail the graph
        graph.vertices.push_back (newHeadLabel);
        graph.edges[newHeadLabel].insert (newHeadLabel);
        graph.edges[newHeadLabel].insert (extraParLabel);

        instMap[newHeadLabel] = std::move (newHeadBody);

        nextLabel++;
    }

    // rewrite the parent block to lead it to the new head
    const Instruction* cbrInst = parBlock[psize - 1];
    const Instruction* cmpInst = parBlock[psize - 2];

    graph.edges[loop.parent].erase (string (cbrInst->op->label1));
    graph.edges[loop.parent].erase (string (cbrInst->op->label2));
    
    delete cbrInst;

    // build the increment instruction
    parBlock[psize - 2] = new Instruction (nullptr, new Operation (
        loopType, loopVar, 0, nextReg, newStep));
    
    // build the comparison instruction
    parBlock[psize - 1] = new Instruction (nullptr, new Operation (
        cmpInst->op->code, nextReg, cmpInst->op->reg1, cmpInst->op->reg2));
    
    // build the conditional branch instruction
    parBlock.push_back (new Instruction (nullptr, new Operation (
        OpCode::cbr_, cmpInst->op->reg2, 0, 0, 0, 
        newHeadLabel.c_str (), extraParLabel.c_str ())));
    
    delete cmpInst;

    // maintain the graph
    graph.edges[loop.parent].insert (newHeadLabel);
    graph.edges[loop.parent].insert (extraParLabel);

    // also maintain the reverse graph
    reverseGraph (graph, &revGraph);
}

void valueNumbering (const vector <const Instruction*> &fromMe, 
    vector <const Instruction*> *toMe, 
    const vector <size_t> &lead, const vector <size_t> &last) {

    size_t numBlocks = lead.size ();
    vector <pair <size_t, size_t>> blocks;
    for (size_t i = 0; i < numBlocks; i++)
        blocks.push_back (make_pair (lead[i], last[i]));

    // get next unused register
    size_t nextReg = nextUnusedReg (fromMe);

    for (const auto &block : blocks)
        valueNumbering (fromMe, block.first, block.second, toMe, nextReg);
}

void loopUnrolling (const vector <const Instruction*> &fromMe, 
    vector <const Instruction*> *toMe, 
    const vector <size_t> &lead, const vector <size_t> &last, 
    const vector <pair <size_t, size_t>> edges, size_t unrollBy) {

    // maps label to instructions in block body
    unordered_map <string, vector <const Instruction*>> instMap;
    size_t numBlocks = lead.size ();

    // remember the block order
    vector <string> order;
    
    for (size_t i = 0; i < numBlocks; i++) {
        string label = i ? string (fromMe[lead[i]]->label) : "START";
        order.push_back (label);

        // locate the vector that stores instructions under label 'label'
        auto &insts = instMap[label];
        for (size_t j = lead[i]; j <= last[i]; j++)
            insts.push_back (new Instruction (fromMe[j]));
    }

    Graph graph;
    toGraph (fromMe, lead, last, edges, &graph);

    vector <Loop> loops;
    findLoop (graph, &loops);

    Graph revGraph;
    reverseGraph (graph, &revGraph);

    // get next unused register
    size_t nextReg = nextUnusedReg (fromMe);

    size_t nextLabel = 0;
    for (const auto &loop : loops)
        loopUnrolling (instMap, graph, revGraph, loop, nextReg, nextLabel, unrollBy);

    writeInstsBack (instMap, toMe, order);
}

void generateCode (const vector <const Instruction*> fromMe, FILE *writeToMe) {
    for (const Instruction* inst : fromMe) {
        string instruction = translate (inst);
        fprintf (writeToMe, "%s", instruction.c_str ());
    }
    fprintf (writeToMe, "\thalt\n");
}

void freeMemory (vector <const Instruction*> &fromMe) {
    for (const Instruction* &inst : fromMe) {
        delete inst;
        inst = nullptr;
    }
}
