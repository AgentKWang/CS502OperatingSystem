#include "mem_management.h"

INT32 find_free_phys_page();

phys_mem_entry* phys_mem = 0;

void init_page(INT32 vpn, INT32 pid){
	if(phys_mem==0)  phys_mem = calloc(PHYS_MEM_PGS, sizeof(phys_mem)); //initial the phys_mem allocate table at the first time
	INT32 blank_page = find_free_phys_page();
	if(blank_page!=-1){
		Z502_PAGE_TBL_ADDR[vpn] = Z502_PAGE_TBL_ADDR[vpn] | (blank_page & PTBL_PHYS_PG_NO);
		phys_mem[blank_page].pid = pid;
		phys_mem[blank_page].used = TRUE;
	}
	else{
		printf("errrrrrrrrrrrr\nerrrrrrrrrrrr\n");
	}
}

void clear_touched(){

}

INT32 find_free_phys_page(){
	int i; //Initialization in for loop is only allowed in C99
	for(i=0; i<PHYS_MEM_PGS; i++){
		if(phys_mem[i].used==FALSE){
			return i;
		}
	}
	return -1;
}
