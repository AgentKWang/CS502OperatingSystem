/** This is the process management module of the OS


**/
#include "p_manage.h"
#include "stdlib.h"



readyqueue_item readyqueue_header = {0,-1};
PCB *current_pcb;
INT32 process_counter=0; //count how many process have been created
timequeue_node timequeue_header = {0,0,-1};


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

void add_time_queue(PCB* pcb, INT32 wake_up_time){
	timequeue_node* new_node = malloc(sizeof(timequeue_node));
	new_node->pcb = pcb;
	new_node->wakeuptime = wake_up_time;
	if(timequeue_header.next == -1){
		timequeue_header.next = new_node;
		new_node->next = -1;
	}
	else{
		timequeue_node* prev_pointer, *pointer;
		prev_pointer = &timequeue_header;
		pointer = timequeue_header.next;
		while (pointer->next != -1 && pointer->wakeuptime < wake_up_time){
			prev_pointer = pointer;
			pointer = pointer->next;
		}
		if(pointer == -1){
			new_node->next = (pointer->wakeuptime < new_node->wakeuptime)? pointer->next : pointer;
			pointer->next = (pointer->wakeuptime >= new_node->wakeuptime)?  : new_node;
			prev_pointer->next = (pointer->wakeuptime < new_node->wakeuptime)? : new_node;
		}
		else{
			prev_pointer->next = new_node;
			new_node->next = pointer;
		}
	}
}

PCB* get_wake_up_pcb(){
	PCB *wake_up_pcb = -1;
	if(timequeue_header.next != -1){
		timequeue_node* temp;
		temp = timequeue_header.next;
		wake_up_pcb = temp->pcb;
		timequeue_header.next = temp->next;
		free(temp);
	}
	return wake_up_pcb;
}
