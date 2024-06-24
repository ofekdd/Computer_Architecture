/* 046267 Computer Architecture - HW #1                                 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"

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
			bool isGlobalHist, bool isGlobalTable, int Shared){
	if(btbSize == 1 || btbSize == 2 || btbSize == 4 || btbSize == 8 || btbSize == 16 || btbSize == 32){
		if(historySize <= 8 && historySize >= 1){
			int log_btb_size = log2(btbSize);
			if(tagSize >= 0 && tagSize <= 30-log_btb_size){
				if(fsmState == 1 || fsmState == 2 || fsmState == 3 || fsmState == 4){
					if(Shared == 0 || Shared == 1 || Shared == 2){
						bp = (BP*)malloc(sizeof(BP));
						if(bp == NULL) return -1;
						bp->btb = (BTB_entry*)malloc(sizeof(BTB_entry)*btbSize);
						if(bp->btb == NULL) return -1;

						bp->btbSize = btbSize;
						bp->historySize = historySize;
						bp->tagSize = tagSize;
						bp->fsmState = fsmState;
						bp->isGlobalHist = isGlobalHist;
						bp->isGlobalTable = isGlobalTable;
						bp->Shared = Shared;
						bp->stats.flush_num = 0;
						bp->stats.br_num = 0;
						return 0;
					}
				}
			}
		}
	}
	return -1;
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

