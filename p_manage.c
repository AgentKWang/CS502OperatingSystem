/** This is the process management module of the OS


**/
#include "p_manage.h"
//#include "stdlib.h"



readyqueue_item readyqueue_header = {0,-1};
PCB *current_pcb;
INT32 process_counter=0; //count how many process have been created
INT32 pid_counter=0;

void print_ready_queue(){
	INT32 lock_result;
	READ_MODIFY(MEMORY_INTERLOCK_BASE, 1, TRUE, &lock_result);
	readyqueue_item *pointer = readyqueue_header.next;
	SP_setup(SP_RUNNING_MODE, current_pcb->pid);
	while(pointer!=-1){
		SP_setup(SP_READY_MODE,pointer->pcb->pid);
		pointer=pointer->next;
	}
	READ_MODIFY(MEMORY_INTERLOCK_BASE, 0, TRUE, &lock_result);
}

INT32 get_process_id(char* process_name){
	if (strcmp(current_pcb->name, process_name)==0) return current_pcb->pid;
	if(strcmp(process_name,"")==0)
		return current_pcb->pid;
	INT32 lock_result;
	READ_MODIFY(MEMORY_INTERLOCK_BASE, 1, TRUE, &lock_result); //lock on ready_queue
	readyqueue_item * queue_pointer = readyqueue_header.next;
	while(queue_pointer != -1) {
		if(strcmp(process_name, queue_pointer->pcb->name)==0){
			READ_MODIFY(MEMORY_INTERLOCK_BASE, 0, TRUE, &lock_result); //unlock the ready_queue
			return queue_pointer->pcb->pid;
		}
		else
			queue_pointer= queue_pointer->next;
	}
	READ_MODIFY(MEMORY_INTERLOCK_BASE, 0, TRUE, &lock_result); //unlock the ready_queue
	return -1;
}

INT32 terminate_process(INT32 pid){
	PCB* pcb_to_destroy;
	if(current_pcb->pid==pid){
		pcb_to_destroy = current_pcb;
		process_counter--;
		free(pcb_to_destroy);
		return ERR_SUCCESS;
	}
	else{
		INT32 lock_result;
		READ_MODIFY(MEMORY_INTERLOCK_BASE, 1, TRUE, &lock_result);
		readyqueue_item *pointer = readyqueue_header.next;
		readyqueue_item *prev_pointer = &readyqueue_header;
		while(pointer!=-1){
			if(pointer->pcb->pid==pid){
				prev_pointer->next=pointer->next;
				process_counter--;
				free(pointer->pcb);
				free(pointer);
				return ERR_SUCCESS;
			}
			prev_pointer = pointer;
			pointer = pointer->next;
		}
		READ_MODIFY(MEMORY_INTERLOCK_BASE, 0, TRUE, &lock_result);
		if(pointer==-1){
			//find in timer queue
			return -1;
		}
	}
}

PCB* create_process(void* code_to_run, BOOL mode, INT32 priority, char* name) {
	/*check if everything is right
	 * return -1 for illegal priority
	 * return -2 for same process name error
	 * return pcb if success
	 */
	if(process_counter>=20){
		return (PCB*)-3;
	}
	if(priority<0){
		PCB* pcb;
		pcb=-1;
		return pcb;
	}
	readyqueue_item *q_pointer = &readyqueue_header;
	while(q_pointer->next!=-1){
		q_pointer = q_pointer->next;
		if(strcmp(q_pointer->pcb->name,name)==0){
			return (PCB*)-2;
		}
	}
	// initiate the pcb
	PCB *pcb = (PCB*) malloc( sizeof(PCB));
	pcb->pid = pid_counter;
	process_counter++;
	pid_counter++;
	pcb->priority=priority;
	pcb->name = (char*)malloc(sizeof(char[16]));
	strcpy(pcb->name,name);
	pcb->mode=mode;
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
		INT32 lock_result;
		READ_MODIFY(MEMORY_INTERLOCK_BASE, 1, TRUE, &lock_result); //lock ready_queue
		readyqueue_item *pointer, *prev_pointer;
		prev_pointer = &readyqueue_header;
		pointer = readyqueue_header.next;
		while (pointer!= -1 && pointer->pcb->priority <= pcb->priority) {
			prev_pointer = pointer;
			pointer = pointer->next;
		}
		item->next = pointer;
		prev_pointer->next = item;
		READ_MODIFY(MEMORY_INTERLOCK_BASE, 0, TRUE, &lock_result);
	} //end of situation more-than-two items queue
}

void run_process(PCB *pcb){
	current_pcb=pcb; //Always remember the process that is now running
	/*some code here to remove the pcb from ready_queue
	 * As far as I'm concerned, a process entered in may be
	 * from a readyqueue, in this case you need to move it out from the
	 * readyqueue, or it may be called by the routine in svc where the
	 * pcb is from a timequeque, and in this case, since the pcb is moved
	 * out from timequeue already and is going to run right away,
	 * we do not need to move it out from any queue.
	 */
	Z502SwitchContext(SWITCH_CONTEXT_SAVE_MODE, &pcb->context );
}

PCB* get_current_pcb(){
	return current_pcb;
}

PCB* dispatcher(){
	PCB *pcb;
	readyqueue_item *q_pointer = readyqueue_header.next;
	if(q_pointer == -1){
		//printf("*****************\nThere's no process to run!\n*****************\n");
		//print_ready_queue();
		return -1;
	}
	else{
		INT32 lock_result;
		READ_MODIFY(MEMORY_INTERLOCK_BASE, 1, TRUE, &lock_result);
		pcb=q_pointer->pcb;
		//printf("find process %s to run \n",pcb->name);
		readyqueue_header.next = q_pointer->next;
		free(q_pointer);
		READ_MODIFY(MEMORY_INTERLOCK_BASE, 0, TRUE, &lock_result);
		//print_ready_queue();
		return pcb;
	}
}
