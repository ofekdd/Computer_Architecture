#include "bp_api.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

// Structure representing a BTB (Branch Target Buffer) entry
typedef struct {
    uint32_t tag;      // Tag to identify the instruction
    uint32_t target;   // Target address for the branch
    uint32_t history;  // Branch history
    uint32_t *fsm;     // Finite State Machine (FSM) states for prediction
} BTB_entry;

// Structure representing the Branch Predictor
typedef struct {
    BTB_entry *btb;           // Pointer to the BTB entries
    unsigned btbSize;         // Size of the BTB
    unsigned historySize;     // Size of the history
    unsigned tagSize;         // Size of the tag
    unsigned fsmState;        // Initial FSM state
    bool isGlobalHist;        // Flag for global history
    bool isGlobalTable;       // Flag for global FSM table
    int Shared;               // Sharing mode
    unsigned globalHistory;   // Global history register
    uint32_t *globalFSM;      // Global FSM table
    SIM_stats stats;          // Statistics for the simulation
} BP;

static BP *bp = NULL; // Global pointer to the Branch Predictor

// Function to initialize the Branch Predictor
int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
            bool isGlobalHist, bool isGlobalTable, int Shared) {
    // Check for valid input parameters
    if (!(btbSize == 1 || btbSize == 2 || btbSize == 4 || btbSize == 8 || btbSize == 16 || btbSize == 32)) return -1;
    if (historySize < 1 || historySize > 8) return -1;
    int log_btb_size = log2(btbSize);
    if (tagSize < 0 || tagSize > 30 - log_btb_size) return -1;
    if (fsmState < 0 || fsmState > 3) return -1;
    if (Shared != 0 && Shared != 1 && Shared != 2) return -1;

    // Allocate memory for the Branch Predictor
    bp = (BP *)malloc(sizeof(BP));
    if (!bp) return -1;
    bp->btb = (BTB_entry *)malloc(sizeof(BTB_entry) * btbSize);
    if (!bp->btb) {
        free(bp);
        return -1;
    }

    // Initialize FSM table
    if (isGlobalTable) {
        bp->globalFSM = (uint32_t *)malloc(sizeof(uint32_t) * (1 << historySize));
        if (!bp->globalFSM) {
            free(bp->btb);
            free(bp);
            return -1;
        }
        for (unsigned i = 0; i < (1 << historySize); ++i) {
            bp->globalFSM[i] = fsmState;
        }
    } else {
        for (unsigned i = 0; i < btbSize; ++i) {
            bp->btb[i].fsm = (uint32_t *)malloc(sizeof(uint32_t) * (1 << historySize));
            if (!bp->btb[i].fsm) {
                for (unsigned j = 0; j < i; ++j) {
                    free(bp->btb[j].fsm);
                }
                free(bp->btb);
                free(bp);
                return -1;
            }
            for (unsigned j = 0; j < (1 << historySize); ++j) {
                bp->btb[i].fsm[j] = fsmState;
            }
        }
    }

    // Initialize local histories if not using global history
    if (!isGlobalHist) {
        for (unsigned i = 0; i < btbSize; ++i) {
            bp->btb[i].history = 0;
            bp->btb[i].target = 0;
        }
    }

    // Set Branch Predictor parameters
    bp->btbSize = btbSize;
    bp->historySize = historySize;
    bp->tagSize = tagSize;
    bp->fsmState = fsmState;
    bp->isGlobalHist = isGlobalHist;
    bp->isGlobalTable = isGlobalTable;
    bp->Shared = Shared;
    bp->globalHistory = 0;
    bp->stats.flush_num = 0;
    bp->stats.br_num = 0;

    // Calculate the size of the predictor
    unsigned target_size = 30; // Assume the size of target is 30 bits
    unsigned num_of_fsms = bp->isGlobalTable ? 1 : bp->btbSize; // Number of FSMs
    unsigned num_of_histories = bp->isGlobalHist ? 1 : bp->btbSize; // Number of history registers

    bp->stats.size = 2 * pow(2, bp->historySize) * num_of_fsms; // FSM size
    bp->stats.size += bp->historySize * num_of_histories; // History size
    bp->stats.size += (bp->tagSize + target_size + 1) * bp->btbSize; // Entry size

    return 0;
}

// Function to predict the branch target
bool BP_predict(uint32_t pc, uint32_t *dst) {
    // Calculate the BTB index
    uint32_t btbIndex = (pc >> 2) & (bp->btbSize - 1);
    BTB_entry *entry = &bp->btb[btbIndex];

    // Calculate the tag
    uint32_t tag = (pc >> (2 + (int)log2(bp->btbSize))) & ((1 << bp->tagSize) - 1);

    // Check if the tag matches
    if (entry->tag != tag) {
        *dst = pc + 4; // Default to the next sequential instruction
        return false;
    }

    // Determine history
    unsigned history = bp->isGlobalHist ? bp->globalHistory : entry->history;

    // Calculate the FSM index with optional sharing
    unsigned fsmIndex = history;
    if (bp->Shared == 1) { // Using lower bits of PC
        fsmIndex ^= (pc >> 2) & ((1 << bp->historySize) - 1);
    } else if (bp->Shared == 2) { // Using mid bits of PC
        fsmIndex ^= (pc >> 16) & ((1 << bp->historySize) - 1);
    }

    // Get the current FSM state
    int fsm_current_state;
    if (bp->isGlobalTable) {
        fsm_current_state = bp->globalFSM[fsmIndex];
    } else {
        fsm_current_state = entry->fsm[fsmIndex];
    }

    // Determine if the branch is taken based on the FSM state
    bool taken = fsm_current_state >= 2;
    *dst = taken ? entry->target : pc + 4;
    return taken;
}

// Function to update the Branch Predictor after the branch outcome is known
void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst) {
    // Calculate the BTB index
    uint32_t btbIndex = (pc >> 2) & (bp->btbSize - 1);
    BTB_entry *entry = &bp->btb[btbIndex];

    // Calculate the tag
    uint32_t tag = (pc >> (2 + (int)log2(bp->btbSize))) & ((1 << bp->tagSize) - 1);

    // Determine history
    unsigned history = bp->isGlobalHist ? bp->globalHistory : entry->history;

    // Calculate the FSM index with optional sharing
    unsigned fsmIndex = history;
    if (bp->Shared == 1) { // Using lower bits of PC
        fsmIndex ^= (pc >> 2) & ((1 << bp->historySize) - 1);
    } else if (bp->Shared == 2) { // Using mid bits of PC
        fsmIndex ^= (pc >> 16) & ((1 << bp->historySize) - 1);
    }

    bool tagMismatch = (entry->tag != tag);
    bool misprediction = (taken && pred_dst != targetPc) || (!taken && pred_dst != pc + 4);

    // Check if the tag matches or if there was a misprediction
    if (tagMismatch || misprediction) {
        // Insert new branch instruction into the BTB entry if the tag doesn't match
        if (tagMismatch) {
            entry->tag = tag;
            entry->target = targetPc;

            if (!bp->isGlobalHist) {
                // Reset local history
                entry->history = 0;
            }

            if (!bp->isGlobalTable) {
                // Initialize local FSM to the default state
                for (unsigned i = 0; i < (1 << bp->historySize); ++i) {
                    entry->fsm[i] = bp->fsmState;
                }
            }
        }

        // Update statistics for misprediction
        if (misprediction) {
            bp->stats.flush_num++;
            // Correct the target address in the BTB entry
            entry->target = targetPc;
        }
    }

    // Update the FSM state based on the actual outcome
    uint32_t *fsm = bp->isGlobalTable ? bp->globalFSM : entry->fsm;
    if (taken) {
        if (fsm[fsmIndex] < 3) {
            fsm[fsmIndex]++;
        }
    } else {
        if (fsm[fsmIndex] > 0) {
            fsm[fsmIndex]--;
        }
    }

    // Update the history
    history = ((history << 1) | taken) & ((1 << bp->historySize) - 1);
    if (bp->isGlobalHist) {
        bp->globalHistory = history;
    } else {
        entry->history = history;
    }

    bp->stats.br_num++;
}

// Function to retrieve statistics and clean up the Branch Predictor
void BP_GetStats(SIM_stats *curStats) {
    if (!bp || !curStats) return;

    // Retrieve the statistics
    curStats->flush_num = bp->stats.flush_num;
    curStats->br_num = bp->stats.br_num;
    curStats->size = bp->stats.size;

    // Deallocate all allocated memory
    if (!bp->isGlobalTable) {
        for (unsigned i = 0; i < bp->btbSize; ++i) {
            free(bp->btb[i].fsm);
        }
    }
    free(bp->btb);
    if (bp->isGlobalTable) {
        free(bp->globalFSM);
    }
    free(bp);
    bp = NULL;
}
