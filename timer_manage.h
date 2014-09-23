/*
 * timer_manage.h
 *
 *  Created on: Sep 21, 2014
 *      Author: Kehan
 */

#ifndef TIMER_MANAGE_H_
#define TIMER_MANAGE_H_

#include "p_manage.h"

typedef struct{
	INT32 wakeuptime;
	PCB* pcb;
	struct timequeue_node* next;
}timequeue_node;

void add_time_queue(PCB* pcb, INT32 wake_up_time);
PCB* get_wake_up_pcb();


#endif /* TIMER_MANAGE_H_ */
