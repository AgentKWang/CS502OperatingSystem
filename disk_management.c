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

void disk_write(INT32 disk_id, INT32 sector, char* buffer){
	INT32 status;
	PCB* current = get_current_pcb();
	MEM_WRITE(Z502DiskSetID, &disk_id);
	MEM_WRITE(Z502DiskSetSector, &sector);
	MEM_WRITE(Z502DiskSetBuffer, buffer);
	INT32 temp = 1; //1 means write in the disk action
	MEM_WRITE(Z502DiskSetAction, &temp);
	temp = 0; //0 means start the disk action
	MEM_READ(Z502DiskStatus, &status);
	if(status == DEVICE_FREE){
		add_disk_queue(current, disk_id);
		MEM_WRITE(Z502DiskStart, &temp); //the disk is start
	}
	else if(status==DEVICE_IN_USE){
		while(status==DEVICE_IN_USE){
			add_disk_queue(current, disk_id);
			transfer_ctrl();
			MEM_WRITE(Z502DiskSetID, &disk_id);
			MEM_READ(Z502DiskStatus, &status);
		}
		MEM_WRITE(Z502DiskSetID, &disk_id);
		MEM_WRITE(Z502DiskSetSector, &sector);
		MEM_WRITE(Z502DiskSetBuffer, buffer);
		temp = 1; //write operation
		MEM_WRITE(Z502DiskSetAction, &temp);
		temp = 0; //start the disk
		MEM_WRITE(Z502DiskStart, &temp);
	}
	transfer_ctrl();
}

void disk_read(INT32 disk_id, INT32 sector, char* buffer){
	INT32 status;
	PCB *current = get_current_pcb();
	MEM_WRITE(Z502DiskSetID, &disk_id);
	MEM_WRITE(Z502DiskSetSector, &sector);
	MEM_WRITE(Z502DiskSetBuffer, buffer);
	INT32 temp = 0; //0 means read in the disk action
	MEM_WRITE(Z502DiskSetAction, &temp);
	MEM_READ(Z502DiskStatus, &status);
	if(status == DEVICE_FREE){//start the disk if disk if free
		add_disk_queue(current, disk_id);
		MEM_WRITE(Z502DiskStart, &temp);
	}
	else if(status==DEVICE_IN_USE){
		while(status==DEVICE_IN_USE){
			add_disk_queue(current, disk_id);
			transfer_ctrl();
			MEM_WRITE(Z502DiskSetID, &disk_id);
			MEM_READ(Z502DiskStatus, &status);
		}
		MEM_WRITE(Z502DiskSetID, &disk_id);
		MEM_WRITE(Z502DiskSetSector, &sector);
		MEM_WRITE(Z502DiskSetBuffer, buffer);
		MEM_WRITE(Z502DiskSetAction, &temp);
		MEM_WRITE(Z502DiskStart, &temp);
	}
	transfer_ctrl();
}

void transfer_ctrl(){//check if any process wait in ready queue,find another process to run
	PCB *target = dispatcher();
	while((INT32)target==-1) {
		Z502Idle();
		target = dispatcher();
	}
	run_process(target);
}

INT32 get_process_id_in_disk_queue(char* process_name){
	INT32 lock_result;
	READ_MODIFY(DISK_QUEUE_LOCK, 1, TRUE, &lock_result); //lock on diskQ
	disk_queue_node * queue_pointer = disk_queue_header.next;
	while(queue_pointer != -1) {
		if(strcmp(process_name, queue_pointer->pcb->name)==0){
			READ_MODIFY(DISK_QUEUE_LOCK, 0, TRUE, &lock_result); //unlock the diskq
			return queue_pointer->pcb->pid;
		}
		else
			queue_pointer= queue_pointer->next;
	}
	READ_MODIFY(DISK_QUEUE_LOCK, 0, TRUE, &lock_result); //unlock the disk_queue
	return -1;
}
