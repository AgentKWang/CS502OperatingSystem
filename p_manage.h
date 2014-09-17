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
	INT32 wakeuptime;
	PCB* pcb;
	struct timequeue_node* next;
}timequeue_node;

typedef struct{
	PCB* pcb;
	struct readyqueue_item* next;
}readyqueue_item;

PCB* create_process(void* code_to_run, BOOL mode);
void run_process(BOOL mode, PCB *pcb);
void add_ready_queue(PCB *pcb);
PCB* get_current_pcb();
void add_time_queue(PCB* pcb, INT32 wake_up_time);
PCB* get_wake_up_pcb();

