/* 046267 Computer Architecture - HW #3 */
/* Implementation (skeleton) for the dataflow statistics calculator */

#include "dflow_calc.h"
#include <vector>
#include <algorithm>

struct Node {
    unsigned int opcode;
    int dstIdx;
    unsigned int src1Idx;
    unsigned int src2Idx;
    int src1Dep;
    int src2Dep;
    unsigned int waitingTime;
};

class ProgCtxImpl {
public:
    // Constructor
    ProgCtxImpl(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts);
    
    // Get the dataflow dependency depth in clock cycles
    int getInstDepth(unsigned int theInst) const;
    
    // Get the instructions that a given instruction depends upon
    int getInstDeps(unsigned int theInst, int &src1DepInst, int &src2DepInst) const;
    
    // Get the longest execution path of this program (from Entry to Exit)
    int getProgDepth() const;

private:
    std::vector<Node> instructions;
    std::vector<int> opsLatency;
    unsigned int progDepth;

    void analyzeDependencies();
    void calculateDepths();
};

// Constructor
ProgCtxImpl::ProgCtxImpl(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts) 
    : opsLatency(opsLatency, opsLatency + MAX_OPS), progDepth(0) {
    
    // Initialize instructions
    instructions.resize(numOfInsts);
    for (unsigned int i = 0; i < numOfInsts; ++i) {
        instructions[i].opcode = progTrace[i].opcode;
        instructions[i].dstIdx = progTrace[i].dstIdx;
        instructions[i].src1Idx = progTrace[i].src1Idx;
        instructions[i].src2Idx = progTrace[i].src2Idx;
        instructions[i].src1Dep = -1;
        instructions[i].src2Dep = -1;
        instructions[i].waitingTime = 0;
    }

    // Analyze dependencies and calculate depths
    analyzeDependencies();
    calculateDepths();
}

// Analyze dependencies
void ProgCtxImpl::analyzeDependencies() {
    for (unsigned int i = 0; i < instructions.size(); ++i) {
        for (unsigned int j = 0; j < i; ++j) {
            if (instructions[i].src1Idx == static_cast<unsigned int>(instructions[j].dstIdx)) {
                instructions[i].src1Dep = j;
            }
            if (instructions[i].src2Idx == static_cast<unsigned int>(instructions[j].dstIdx)) {
                instructions[i].src2Dep = j;
            }
        }
    }
}

// Calculate depths
void ProgCtxImpl::calculateDepths() {
    for (unsigned int i = 0; i < instructions.size(); ++i) {
        unsigned int src1Time = 0, src2Time = 0;
        if (instructions[i].src1Dep != -1) {
            src1Time = instructions[instructions[i].src1Dep].waitingTime;
        }
        if (instructions[i].src2Dep != -1) {
            src2Time = instructions[instructions[i].src2Dep].waitingTime;
        }
        instructions[i].waitingTime = opsLatency[instructions[i].opcode] + std::max(src1Time, src2Time);
        progDepth = std::max(progDepth, instructions[i].waitingTime);
    }
}

// Get instruction depth
int ProgCtxImpl::getInstDepth(unsigned int theInst) const {
    if (theInst >= instructions.size()) {
        return -1;
    }
    return instructions[theInst].waitingTime - opsLatency[instructions[theInst].opcode];
}

// Get instruction dependencies
int ProgCtxImpl::getInstDeps(unsigned int theInst, int &src1DepInst, int &src2DepInst) const {
    if (theInst >= instructions.size()) {
        return -1;
    }
    src1DepInst = instructions[theInst].src1Dep;
    src2DepInst = instructions[theInst].src2Dep;
    return 0;
}

// Get program depth
int ProgCtxImpl::getProgDepth() const {
    return progDepth;
}

extern "C" {

ProgCtx analyzeProg(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts) {
    return new ProgCtxImpl(opsLatency, progTrace, numOfInsts);
}

void freeProgCtx(ProgCtx ctx) {
    delete static_cast<ProgCtxImpl*>(ctx);
}

int getInstDepth(ProgCtx ctx, unsigned int theInst) {
    return static_cast<ProgCtxImpl*>(ctx)->getInstDepth(theInst);
}

int getInstDeps(ProgCtx ctx, unsigned int theInst, int *src1DepInst, int *src2DepInst) {
    return static_cast<ProgCtxImpl*>(ctx)->getInstDeps(theInst, *src1DepInst, *src2DepInst);
}

int getProgDepth(ProgCtx ctx) {
    return static_cast<ProgCtxImpl*>(ctx)->getProgDepth();
}

} // extern "C"
