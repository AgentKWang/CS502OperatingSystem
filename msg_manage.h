/*
 * msg_manage.h
 *
 *  Created on: Oct 15, 2014
 *      Author: Kehan
 */

#ifndef MSG_MANAGE_H_
#define MSG_MANAGE_H_

#include "global.h"

typedef struct{
	char *msgbody;
	INT32 length;
	INT32 actual_sender;
}message;

typedef struct{
	INT32 receiver_pid;
	INT32 sender_pid;
	message* msg;
	struct msg_list* next;
}msg_list;

INT32 send_msg(INT32 sender_pid, INT32 receiver_pid, char *msg, INT32 length, long* err_info);
INT32 receive_msg(INT32 receiver_pid, INT32 sender_pid, char* buffer, INT32 length, INT32 *actual_length, INT32 actual_sender, long* err_info);
#endif /* MSG_MANAGE_H_ */
