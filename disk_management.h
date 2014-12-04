/*
 * disk_management.h
 *
 *  Created on: Nov 20, 2014
 *      Author: Kehan
 */

#ifndef DISK_MANAGEMENT_H_
#define DISK_MANAGEMENT_H_

#include "p_manage.h"
#include "syscalls.h"

typedef struct{
	INT32 disk_id;
	PCB* pcb;
	struct disk_queue_node* next;
}disk_queue_node;

PCB* get_next(INT32 disk_id);
void add_disk_queue(PCB* pcb, INT32 disk_id);

#endif /* DISK_MANAGEMENT_H_ */