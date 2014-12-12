/*
 * disk_management.h
 *
 *  Created on: Nov 20, 2014
 *      Author: Kehan
 */

#ifndef DISK_MANAGEMENT_H_
#define DISK_MANAGEMENT_H_
#define DISK_QUEUE_LOCK MEMORY_INTERLOCK_BASE+20

#include "p_manage.h"
#include "syscalls.h"

typedef struct{
	INT32 disk_id;
	PCB* pcb;
	struct disk_queue_node* next;
}disk_queue_node;

PCB* get_next(INT32 disk_id);
void disk_read(INT32 disk_id, INT32 sector, char* buffer);
void disk_write(INT32 disk_id, INT32 sector, char* buffer);
INT32 get_process_id_in_disk_queue(char* process_name);
void print_disk_queue();

#endif /* DISK_MANAGEMENT_H_ */
