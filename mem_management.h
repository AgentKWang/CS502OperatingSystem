/*
 * mem_management.h
 *
 *  Created on: Nov 13, 2014
 *      Author: Kehan
 */

#ifndef MEM_MANAGEMENT_H_
#define MEM_MANAGEMENT_H_
#include "global.h"
#include "string.h"

extern UINT16        *Z502_PAGE_TBL_ADDR;
extern INT16         Z502_PAGE_TBL_LENGTH;


typedef struct{
	INT32 pid;
	BOOL used;
	BOOL shared;
	UINT16 *page_table_entry;
	INT16 disk_sector;
}phys_mem_entry;

typedef struct{
	INT32 pid;
	UINT16 *pg_tbl_entry;
	INT32 shared_id;
	struct shared_memory_entry *next;
}shared_memory_entry;

typedef struct{
	char tag[32];
	INT32 size;
	INT32* phy_adds;
	shared_memory_entry* info;
	struct sh_mem_tbl_entry* next;
}sh_mem_tbl_entry;

/**
 * Method List:
 *
 */
void init_page(INT32 vpn, INT32 pid);
sh_mem_tbl_entry* get_sh_mem(char* tag);
void create_sh_mem_entry(char* tag, INT32 pid, UINT16* pg_entry, INT32 size);
INT32 add_sharer_in_sh_mem(sh_mem_tbl_entry* shared_area_info, INT32 pid, UINT16* pg_entry);
void print_phys_mem();
#endif /* MEM_MANAGEMENT_H_ */
