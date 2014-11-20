/*
 * mem_management.h
 *
 *  Created on: Nov 13, 2014
 *      Author: Kehan
 */

#ifndef MEM_MANAGEMENT_H_
#define MEM_MANAGEMENT_H_
#include "global.h"
extern UINT16        *Z502_PAGE_TBL_ADDR;
extern INT16         Z502_PAGE_TBL_LENGTH;


typedef struct{
	INT32 pid;
	BOOL used;
}phys_mem_entry;


/**
 * Method List:
 *
 */
void init_page(INT32 vpn, INT32 pid);

#endif /* MEM_MANAGEMENT_H_ */
