/*
 * p_manage.h
 *
 *  Created on: Sep 11, 2014
 *      Author: Kehan
 */

#ifndef P_MANAGE_H_
#define P_MANAGE_H_

#include "global.h"
#include "syscalls.h"

typedef struct{
	INT32 pid;
	INT32 priority;
	void *context;
	char *name;
	BOOL mode;
}PCB;


typedef struct{
	PCB* pcb;
	struct readyqueue_item* next;
}readyqueue_item;

PCB* create_process(void* code_to_run, BOOL mode, INT32 priority, char* name);
void run_process(PCB *pcb);
void add_ready_queue(PCB *pcb);
PCB* get_current_pcb();
PCB* dispatcher();
INT32 get_process_id(char* process_name);


#endif /* P_MANAGE_H_ */
