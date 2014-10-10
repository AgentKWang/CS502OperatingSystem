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
		READ_MODIFY(MEMORY_INTERLOCK_BASE+1, 0, TRUE, &lock_result);
	}
	timequeue_node *next_alarm = timequeue_header.next;
	//print_time_queue();
	return next_alarm->wakeuptime;
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


