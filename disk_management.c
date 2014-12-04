/*
 * disk_management.c
 *
 *  Created on: Nov 20, 2014
 *      Author: Kehan
 */
#include "disk_management.h"
#define DISK_QUEUE_LOCK MEMORY_INTERLOCK_BASE+20

disk_queue_node disk_queue_header = {0,0,-1};


PCB* get_next(INT32 disk_id){
	PCB *wake_up_pcb = -1;
	disk_queue_node *prev_pointer=&disk_queue_header;
	disk_queue_node *pointer = disk_queue_header.next;
	INT32 lock_result;
	READ_MODIFY(DISK_QUEUE_LOCK, 1, TRUE, &lock_result);
	while(pointer!= -1){
		if(pointer->disk_id==disk_id){
			prev_pointer->next = pointer->next;
			wake_up_pcb = pointer->pcb;
			free(pointer);
			break;
		}
		prev_pointer = pointer;
		pointer = pointer->next;
		//print_time_queue();
	}
	READ_MODIFY(DISK_QUEUE_LOCK, 0, TRUE, &lock_result);
	return wake_up_pcb;
}

void add_disk_queue(PCB* pcb, INT32 disk_id){
	disk_queue_node* new_node = malloc(sizeof(disk_queue_node));
	disk_queue_node* pointer = &disk_queue_header;
	INT32 lock_result;
	new_node->pcb = pcb;
	new_node->disk_id = disk_id;
	new_node->next = -1;
	READ_MODIFY(DISK_QUEUE_LOCK, 1, TRUE, &lock_result);
	while(pointer->next!=-1) pointer = pointer->next;
	pointer->next = new_node;
	READ_MODIFY(DISK_QUEUE_LOCK, 0, TRUE, &lock_result);
}
