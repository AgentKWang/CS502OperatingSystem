/*
 * timer_manage.c
 *
 *  Created on: Sep 21, 2014
 *      Author: Kehan
 */
#include "timer_manage.h"

timequeue_node timequeue_header = {0,0,-1};

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


