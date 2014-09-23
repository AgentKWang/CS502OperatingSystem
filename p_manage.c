/** This is the process management module of the OS


**/
#include "p_manage.h"
#include "stdlib.h"



readyqueue_item readyqueue_header = {0,-1};
PCB *current_pcb;
INT32 process_counter=0; //count how many process have been created

PCB* create_process(void* code_to_run, BOOL mode, INT32 priority, char* name) {
// initiate the pcb	
	PCB *pcb = (PCB*) malloc( sizeof(PCB));
	pcb->pid = process_counter;
	process_counter++;
	pcb->priority=priority;
	pcb->name = name;
	void *context;
	Z502MakeContext(&context, code_to_run, mode);
	pcb->context=context;
	return pcb;
}

void add_ready_queue(PCB *pcb){
	readyqueue_item * item;
	item= (readyqueue_item*) malloc( sizeof( readyqueue_item));
	item->pcb=pcb;
	if(readyqueue_header.next == -1){
		//if the queue is empty, then initiate the queue
		item->next = -1;
		readyqueue_header.next = item; //load the first pcb into queue
	} // end if situation empty queue
	else{
		//insert the pcb into the queue in the order of the priority
		readyqueue_item *pointer, *prev_pointer;
		prev_pointer = &readyqueue_header;
		pointer = readyqueue_header.next;
		while (pointer->next != -1 && pointer->pcb->priority < pcb->priority) {
			prev_pointer = pointer;
			pointer = pointer->next;
		}
		if(pointer->next==-1) {
			item->next = (pointer->pcb->priority < pcb->priority) ? -1 : pointer ;
			pointer->next = (pointer->pcb->priority >= pcb->priority) ?  : item ;
			prev_pointer->next = (pointer->pcb->priority < pcb->priority) ? :item;
		}
		else{
			item->next = pointer;
			prev_pointer->next = item;
		}
	} //end of situation more-than-two items queue
}

void run_process(BOOL mode, PCB *pcb){
	current_pcb=pcb; //Always remember the process that is now running
	/*some code here to remove the pcb from ready_queue
	 * As far as I'm concerned now, a process entered in may be
	 * from a readyqueue, in this case you need to move it out from the
	 * readyqueue, or it may be called by the routine in svc where the
	 * pcb is from a timequeque, and in this case, since the pcb is moved
	 * out from timequeue already and is going to run right away,
	 * we do not need to move it out from any queue.
	 */
	Z502SwitchContext(mode, &pcb->context );
}

PCB* get_current_pcb(){
	return current_pcb;
}

void dispatcher(BOOL mode){
	PCB *pcb;
	readyqueue_item *q_pointer = readyqueue_header.next;
	if(q_pointer == -1){
		printf("*****************\nThere's no process to run!\n*****************\n");
		return;
	}
	else{
		pcb=q_pointer->pcb;
		printf("find process %s to run \n",pcb->name);
		readyqueue_header.next = q_pointer->next;
		free(q_pointer);
		run_process(mode, pcb);
	}
}
