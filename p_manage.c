/** This is the process management module of the OS


**/
#include "p_manage.h"



readyqueue_item * readyqueue_header;
PCB *current_pcb;
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
	readyqueue_item * item;
	item= (readyqueue_item*) malloc( sizeof( readyqueue_item));
	item->pcb=pcb;
	if(process_counter==1){
		//if the queue is empty, then initiate the queue
		item->next = -1;
		readyqueue_header = item; //load the first pcb into queue
	} // end if situation empty queue
	else{
		//insert the pcb into the queue due to the priority
		if(readyqueue_header->next==-1){ // if the queue has only one item
			item->next = (readyqueue_header->pcb->priority < pcb->priority) ? -1 : readyqueue_header ;
			readyqueue_header->next = (readyqueue_header->pcb->priority < pcb->priority) ? :item;
			//This is a little tricky, but it works!
			//If there's only one item in queue, you need only to compare the priority of
			//header and item to be inserted.
		} //end of situation one-item queue
		else{ //at least 2 elements in queue
			readyqueue_item *pointer, *prev_pointer;
			prev_pointer = readyqueue_header;
			pointer = prev_pointer->next;
			while (pointer->next != -1 && pointer->pcb->priority < pcb->priority) {
				prev_pointer = pointer;
				pointer = pointer->next;
			}
			if(pointer->next==-1) {
				item->next = (pointer->pcb->priority < pcb->priority) ? -1 : pointer ;
				pointer->next = (pointer->pcb->priority < pcb->priority) ? :item;
			}
			else{
				item->next = pointer;
				prev_pointer->next = item;
			}
		} //end of situation more-than-two items queue
	} // end of situation none-empty queue
}
void run_process(BOOL mode, PCB *pcb){
	current_pcb=pcb; //Always remember the process that is now running
	Z502SwitchContext(mode, &pcb->context );
}

PCB* get_current_pcb(){
	return current_pcb;
}
