/*
 * msg_manage.c
 *
 *  Created on: Oct 15, 2014
 *      Author: Kehan
 */
#include "msg_manage.h"
#define MSG_MAX_NUM 20

msg_list send_list_head={0,0,0,-1};
msg_list receive_list_head = {0,0,0,-1};
INT32 MSG_COUNTER=0;

INT32 send_msg(INT32 sender_pid, INT32 receiver_pid, char *msg, INT32 length, long* err_info){ //return 1 means the msg is sent immediately, 0 means the msg is put in Q
	msg_list *prev_pointer=&receive_list_head, *pointer = receive_list_head.next;
	INT32 send_flag = 0;
	while(pointer!=-1){
		if((pointer->sender_pid==sender_pid || pointer->sender_pid==-1) &&(pointer->receiver_pid==receiver_pid || receiver_pid==-1)){
			send_flag++;   //the msg is send immediately
			pointer->msg->actual_sender = sender_pid;
			pointer->msg->length = length;
			strcpy(pointer->msg->msgbody,msg);
			prev_pointer->next = pointer->next;
			MSG_COUNTER--;
		}
		prev_pointer = pointer;
		pointer = pointer->next;
	}
	if(send_flag==0){//msg not immediately send
		if(MSG_COUNTER >= MSG_MAX_NUM){  //exceed the maximum msg list
			*err_info = -3;
			return 0; //os will not suspend current process
		}
		msg_list *node = malloc(sizeof(msg_list));
		node->sender_pid = sender_pid;
		node->receiver_pid = receiver_pid;
		node->next = -1;
		node->msg = malloc(sizeof(message));
		node->msg->length = length;
		node->msg->msgbody = malloc(length);
		node->msg->actual_sender = sender_pid;
		strcpy(node->msg->msgbody,msg);
		pointer = &send_list_head;
		while(pointer->next!=-1) pointer=pointer->next;
		pointer->next = node;
		MSG_COUNTER ++;
	}
	*err_info = 0;
	return send_flag;
}

INT32 receive_msg(INT32 receiver_pid, INT32 sender_pid, char* buffer, INT32 length, INT32 *actual_length, INT32 *actual_sender, long* err_info){//return 1 means the msg is received,
	msg_list *prev_pointer=&send_list_head, *pointer=send_list_head.next;												//or we need to suspend
	while(pointer!=-1){
		if ((pointer->receiver_pid==receiver_pid || pointer->receiver_pid==-1) && (sender_pid==pointer->sender_pid || sender_pid==-1)){
			if(length < pointer->msg->length){
				*err_info = -3; //buffer not enough
				return 1; //os will not suspend current process
			}
			//feed the msg to receiver
			strcpy(buffer, pointer->msg);
			*actual_length = pointer->msg->length;
			*actual_sender = pointer->msg->actual_sender;
			//remove the node from send list
			prev_pointer->next = pointer->next;
			void *destroy = pointer;
			MSG_COUNTER--;
			free(destroy);
			//return to os
			*err_info = 0;
			return 1;
		}
		prev_pointer = pointer;
		pointer = pointer->next;
	}
	//if not returned before this line, then it indicates that the msg is not immediately received
	//make a node with memery space and attach it to the receive list.
	msg_list *node = malloc(sizeof(msg_list));
	node->next = -1;
	node->receiver_pid = receiver_pid;
	node->sender_pid = sender_pid;
	node->msg = malloc(sizeof(message));
	node->msg->actual_sender = actual_sender;
	node->msg->length = actual_length;
	node->msg->msgbody = buffer;
	pointer = &receive_list_head;
	while(pointer->next!=-1) pointer=pointer->next;
	pointer->next = node;
	*err_info = 0;
	return 0; //ask os the suspend current process
}
