/* 046267 Computer Architecture - HW #1                                 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <math.h>
#include <stdlib.h>
#include <stddef.h>

typedef struct {
    uint32_t tag;
    uint32_t target;
    uint8_t history;
    uint8_t *fsm;
} BTB_entry;

typedef struct {
    BTB_entry *btb;
    unsigned btbSize;
    unsigned historySize;
    unsigned tagSize;
    unsigned fsmState;
    bool isGlobalHist;
    bool isGlobalTable;
    int Shared;
    unsigned globalHistory;
    uint8_t *globalFSM;
    SIM_stats stats;
} BP;

BP *bp = NULL;


int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
            bool isGlobalHist, bool isGlobalTable, int Shared) {
    if (!(btbSize == 1 || btbSize == 2 || btbSize == 4 || btbSize == 8 || btbSize == 16 || btbSize == 32)) return -1;
    if (historySize < 1 || historySize > 8) return -1;
    int log_btb_size = log2(btbSize);
    if (tagSize < 0 || tagSize > 30 - log_btb_size) return -1;
    if (fsmState < 0 || fsmState > 3) return -1;
    if (Shared != 0 && Shared != 1 && Shared != 2) return -1;

    bp = (BP *)malloc(sizeof(BP));
    if (!bp) return -1;
    bp->btb = (BTB_entry *)malloc(sizeof(BTB_entry) * btbSize);
    if (!bp->btb) {
        free(bp);
        return -1;
    }

    if (isGlobalTable) {
        bp->globalFSM = (uint8_t *)malloc(sizeof(uint8_t) * (1 << historySize));
        if (!bp->globalFSM) {
            free(bp->btb);
            free(bp);
            return -1;
        }
        for (unsigned i = 0; i < (1 << historySize); ++i) {
            bp->globalFSM[i] = fsmState;
        }
    } else { // local table and fsm
        for (unsigned i = 0; i < btbSize; ++i) {
            bp->btb[i].fsm = (uint8_t *)malloc(sizeof(uint8_t) * (1 << historySize));
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
    bp->stats.size = sizeof(BTB_entry) * btbSize + (isGlobalTable ? (sizeof(uint8_t) * (1 << historySize)) : 0);
    return 0;
}


bool BP_predict(uint32_t pc, uint32_t *dst){

	return false;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	return;
}

void BP_GetStats(SIM_stats *curStats){
	return;
}

