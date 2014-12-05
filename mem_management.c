#include "mem_management.h"
#include "disk_management.h"
#include "p_manage.h"
#include "protos.h"
#include "stdlib.h"
#define PTBL_ON_DISK 0x1000

void clear_touched();
INT32 find_free_phys_page();
phys_mem_entry* phys_mem = 0;

void init_page(INT32 vpn, INT32 pid){
	if(phys_mem==0)  phys_mem = calloc(PHYS_MEM_PGS, sizeof(phys_mem_entry)); //initial the phys_mem allocate table at the first time
	INT32 blank_page = find_free_phys_page(vpn);
	if(blank_page==-1){
		printf("An error has happened in locate physical memory: find_free_phys_page returns -1 \n");
		return;
	}
	if(Z502_PAGE_TBL_ADDR[vpn] & PTBL_ON_DISK){ //The page is on the disk
		INT32 disk_id = get_current_pcb()->pid;
		INT32 sector = vpn; //use the vpn as sector no
		if(disk_id==0) disk_id=1;
		char buffer[PGSIZE];
		disk_read(disk_id, sector, buffer); //get the page from disk
		Z502WritePhysicalMemory(blank_page, buffer); //then write it into memory
		//now the page is ready
	}
	Z502_PAGE_TBL_ADDR[vpn] = Z502_PAGE_TBL_ADDR[vpn] | (blank_page & PTBL_PHYS_PG_NO); //set the phys page number
	Z502_PAGE_TBL_ADDR[vpn] = Z502_PAGE_TBL_ADDR[vpn] | PTBL_VALID_BIT; //set valid pid
	phys_mem[blank_page].pid = pid;
	phys_mem[blank_page].used = TRUE;
	phys_mem[blank_page].page_table_entry = &Z502_PAGE_TBL_ADDR[vpn]; //set the page table entry which is using this memory page
	phys_mem[blank_page].disk_sector = vpn;
	clear_touched(); //clear reference bit
}

void clear_touched(){
	int i;
	UINT16* pg_tbl_entry;
	for(i=0; i<PHYS_MEM_PGS; i++) {
		if(phys_mem[i].used==TRUE){
			pg_tbl_entry = phys_mem[i].page_table_entry;
			*pg_tbl_entry = *pg_tbl_entry & ~PTBL_REFERENCED_BIT;
		}
	}
}

INT32 find_free_phys_page(){
	int i; //Initialization in for loop is only allowed in C99
	INT32 phy_page_number = -1;
	UINT16* pg_tbl_entry;
	for(i=0; i<PHYS_MEM_PGS; i++){
		if(phys_mem[i].used==FALSE){
			return i;
		}
		else{
			pg_tbl_entry = phys_mem[i].page_table_entry;
			if(!((*pg_tbl_entry)|PTBL_REFERENCED_BIT)){ //The page is not touched recently
				phy_page_number=i;
			}
		}
	}
	//if the next code is executed, then the memory is full and we need to do a page replacement
	if(phy_page_number==-1){
		//printf("\nPANIC!!:ERROR NO SPACE IN MEMORY!\n");
		phy_page_number = 0;
	}
	pg_tbl_entry = phys_mem[phy_page_number].page_table_entry;//get the entry which is currently using the phys memory
	if(((*pg_tbl_entry) | PTBL_MODIFIED_BIT) || !((*pg_tbl_entry) | PTBL_ON_DISK)){ //if the memory is dirty, we need to write it to disk
		INT32 disk_id = get_current_pcb()->pid;
		if(disk_id==0) disk_id=1;
		char buffer[PGSIZE];
		Z502ReadPhysicalMemory(phy_page_number, buffer);
		*pg_tbl_entry = *pg_tbl_entry & (~PTBL_VALID_BIT);//clear the replaced page's valid bit
		disk_write(disk_id, phys_mem[phy_page_number].disk_sector,buffer); //write the data to the memory
		*pg_tbl_entry = *pg_tbl_entry | PTBL_ON_DISK;//set on disk bit
		*pg_tbl_entry = *pg_tbl_entry & (~PTBL_MODIFIED_BIT); //clear modified bit
	}
	return phy_page_number;
}
