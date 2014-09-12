/*
 * p_manage.h
 *
 *  Created on: Sep 11, 2014
 *      Author: Kehan
 */

#ifndef P_MANAGE_H_
#define P_MANAGE_H_



#endif /* P_MANAGE_H_ */

#include "global.h"

typedef struct{
	INT32 pid;
	INT32 priority;
	void *context;
}PCB;

typedef struct{
	PCB* pcb;
	struct readyqueue_item* next;
}readyqueue_item;

PCB* create_process(void* code_to_run, BOOL mode);
void run_process(BOOL mode, PCB *pcb);

