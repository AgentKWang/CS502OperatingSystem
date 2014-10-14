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
timequeue_node* get_timer_queue_head();
INT32 get_process_id_in_timer_queue(char *process_name);
PCB* get_pcb_from_timer_queue(INT32 pid);

#endif /* TIMER_MANAGE_H_ */
