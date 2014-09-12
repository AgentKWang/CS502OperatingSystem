/** This is the process management module of the OS


**/
#include "p_manage.h"



readyqueue_item * readyqueue;
INT32 current_pid;
INT32 process_counter=0; //count how many process have been created

PCB* create_process(void* code_to_run, BOOL mode) {
// initiate the pcb	
	PCB *pcb = (PCB*) malloc( sizeof(PCB));
	pcb->pid = process_counter;
	process_counter++;
	pcb->priority=0;
	void *context;
	Z502MakeContext(&context, code_to_run, mode);
	pcb->context=context;
//The PCB is created, now add it to the readyQueue
	add_ready_queue(pcb);
	return pcb;
}

void add_ready_queue(PCB *pcb){
	if(process_counter==1){
		//if the queue is empty, then initiate the queue
		readyqueue= (readyqueue_item*) malloc( sizeof( readyqueue_item));
		readyqueue->pcb=pcb;
	}
	else{
		//insert the pcb into the queue due to the priority
	}
}
void run_process(BOOL mode, PCB *pcb){
	current_pid=pcb->pid;
	Z502SwitchContext(mode, &pcb->context );
}
