/*
 * timer_manage.c
 *
 *  Created on: Sep 21, 2014
 *      Author: Kehan
 */
#include "timer_manage.h"

timequeue_node timequeue_header = {0,0,-1};

void print_time_queue(){
	INT32 lock_result;
	READ_MODIFY(MEMORY_INTERLOCK_BASE+1, 1, TRUE, &lock_result);
	timequeue_node *pointer = timequeue_header.next;
	PCB* current_pcb = get_current_pcb();
	SP_setup(SP_RUNNING_MODE, current_pcb->pid);
	while(pointer!=-1){
		SP_setup(SP_TIMER_SUSPENDED_MODE,pointer->pcb->pid);
		pointer=pointer->next;
	}
	READ_MODIFY(MEMORY_INTERLOCK_BASE+1, 0, TRUE, &lock_result);
}

INT32 get_next_alarm(){
	timequeue_node * pointer = timequeue_header.next;
	if(pointer == -1 ) return -1;
	else return pointer->wakeuptime;
}

INT32 add_time_queue(PCB* pcb, INT32 wake_up_time){
	timequeue_node* new_node = malloc(sizeof(timequeue_node));
	new_node->pcb = pcb;
	new_node->wakeuptime = wake_up_time;
	if(timequeue_header.next == -1){
		timequeue_header.next = new_node;
		new_node->next = -1;
	}
	else{
		INT32 lock_result;
		READ_MODIFY(MEMORY_INTERLOCK_BASE+1, 1, TRUE, &lock_result);
		timequeue_node* prev_pointer, *pointer;
		prev_pointer = &timequeue_header;
		pointer = timequeue_header.next;
		while (pointer->next != -1 && pointer->wakeuptime <= wake_up_time){
			prev_pointer = pointer;
			pointer = pointer->next;
		}
		if(pointer->next == -1){
			new_node->next = (pointer->wakeuptime < new_node->wakeuptime)? pointer->next : pointer;
			pointer->next = (pointer->wakeuptime >= new_node->wakeuptime)? -1  : new_node;
			if (pointer->wakeuptime >= new_node->wakeuptime) prev_pointer->next= new_node;
		}
		else{
			prev_pointer->next = new_node;
			new_node->next = pointer;
		}
		READ_MODIFY(MEMORY_INTERLOCK_BASE+1, 0, TRUE, &lock_result);
	}
	timequeue_node *next_alarm = timequeue_header.next;
	//print_time_queue();
	return next_alarm->wakeuptime;
}

timequeue_node* get_timer_queue_head(){
	return &timequeue_header;
}
PCB* get_wake_up_pcb(){
	PCB *wake_up_pcb = -1;
	if(timequeue_header.next != -1){
		INT32 lock_result;
		READ_MODIFY(MEMORY_INTERLOCK_BASE+1, 1, TRUE, &lock_result);
		timequeue_node* temp;
		temp = timequeue_header.next;
		wake_up_pcb = temp->pcb;
		timequeue_header.next = temp->next;
		READ_MODIFY(MEMORY_INTERLOCK_BASE+1, 0, TRUE, &lock_result);
		free(temp);
		//print_time_queue();
	}
	return wake_up_pcb;
}

INT32 get_process_id_in_timer_queue(char *process_name){
	INT32 lock_result;
	READ_MODIFY(MEMORY_INTERLOCK_BASE+1, 1, TRUE, &lock_result); //lock on time_queue
	timequeue_node * queue_pointer = timequeue_header.next;
		while(queue_pointer != -1) {
			if(strcmp(process_name, queue_pointer->pcb->name)==0){
				READ_MODIFY(MEMORY_INTERLOCK_BASE+1, 0, TRUE, &lock_result); //unlock the time_queue
				return queue_pointer->pcb->pid;
			}
			else
				queue_pointer= queue_pointer->next;
		}
		READ_MODIFY(MEMORY_INTERLOCK_BASE+1, 0, TRUE, &lock_result); //unlock the time_queue
		return -1;
}

PCB* get_pcb_from_timer_queue(INT32 pid){
	timequeue_node * pointer = timequeue_header.next;
	INT32 lock_result;
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 1, 1, TRUE, &lock_result);
	while(pointer!=-1){
		if(pointer->pcb->pid==pid) break;
		pointer = pointer->next;
	}
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 1, 0, TRUE, &lock_result);
	if(pointer!=-1) return pointer->pcb;
	else return -1;
}

INT32 check_process_in_timerQ(INT32 pid){
	timequeue_node *pointer = timequeue_header.next;
	INT32 lock_result;
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 1, 1, TRUE, &lock_result);
	while(pointer!=-1){
		if(pointer->pcb->pid==pid){
			READ_MODIFY(MEMORY_INTERLOCK_BASE + 1, 0, TRUE, &lock_result);
			return 0;
		}
		pointer = pointer->next;
	}
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 1, 0, TRUE, &lock_result);
	return -1;
}

INT32 change_priority_in_timer_queue(INT32 pid, INT32 priority){
	INT32 lock_result;
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 1, 1, TRUE, &lock_result);
	timequeue_node *pointer = timequeue_header.next;
	while(pointer!=-1){
		if(pointer->pcb->pid==pid) break;
		pointer = pointer->next;
	}
	if(pointer==-1){
		READ_MODIFY(MEMORY_INTERLOCK_BASE + 1, 0, TRUE, &lock_result);
		return -1;
	}
	else{
		READ_MODIFY(MEMORY_INTERLOCK_BASE + 1, 0, TRUE, &lock_result);
		pointer->pcb->priority = priority;
		return ERR_SUCCESS;
	}
}
