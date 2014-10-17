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
	BOOL suspend_flag; //If Interrupt Handler see this flag,
						//it will put the pcb in suspendQ rather than ReadyQ.
}PCB;

typedef struct{
	PCB* pcb;
	struct suspendqueue_node *next;
}suspendqueue_node;

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
INT32 suspend_process(INT32 pid);
INT32 resume_process(INT32 pid);
void add_suspend_queue(PCB *pcb);
INT32 change_priority_in_ready_queue(INT32 pid, INT32 priority);
INT32 check_process_in_readyQ(INT32 pid);
INT32 check_process_in_suspendQ(INT32 pid);
INT32 is_ready_queue_empty();

#endif /* P_MANAGE_H_ */
