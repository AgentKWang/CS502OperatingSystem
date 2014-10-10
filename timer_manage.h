/*
 * timer_manage.h
 *
 *  Created on: Sep 21, 2014
 *      Author: Kehan
 */

#ifndef TIMER_MANAGE_H_
#define TIMER_MANAGE_H_

#include "p_manage.h"
#include "syscalls.h"

typedef struct{
	INT32 wakeuptime;
	PCB* pcb;
	struct timequeue_node* next;
}timequeue_node;

INT32 add_time_queue(PCB* pcb, INT32 wake_up_time);
PCB* get_wake_up_pcb();
INT32 get_next_alarm();


#endif /* TIMER_MANAGE_H_ */
