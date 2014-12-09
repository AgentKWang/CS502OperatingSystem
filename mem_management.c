#include "mem_management.h"
#include "disk_management.h"
#include "p_manage.h"
#include "protos.h"
#include "stdlib.h"
#define PTBL_ON_DISK 0x1000

void clear_touched();
INT32 find_free_phys_page();
phys_mem_entry* phys_mem = 0;
sh_mem_tbl_entry SH_MEM_TBL = {"header", -1, -1, -1, -1};
void set_area_valid(UINT16* pg_entry, INT32 size, INT32* phys_mem_adds);

void init_page(INT32 vpn, INT32 pid){
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
	Z502_PAGE_TBL_ADDR[vpn] = Z502_PAGE_TBL_ADDR[vpn] & (~PTBL_MODIFIED_BIT); //clear the modified bit
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
	if(phys_mem==0)  phys_mem = calloc(PHYS_MEM_PGS, sizeof(phys_mem_entry)); //initial the phys_mem allocate table at the first time
	int i; //Initialization in for loop is only allowed in C99
	INT32 phy_page_number = -1;
	INT32 second_best_page = -1;
	INT32 third_best_page = -1;
	UINT16* pg_tbl_entry;
	for(i=0; i<PHYS_MEM_PGS; i++){
		if(phys_mem[i].used==FALSE){
			phys_mem[i].used=TRUE;
			return i; //The best page found, return immediately
		}
		else{
			pg_tbl_entry = phys_mem[i].page_table_entry;
			if(!((*pg_tbl_entry)|PTBL_REFERENCED_BIT) && !((*pg_tbl_entry)|PTBL_MODIFIED_BIT)){ //The page is not touched recently nor modified
				second_best_page=i; //the 2nd best page, which is not touched nor modified
			}
			else if(!((*pg_tbl_entry)|PTBL_MODIFIED_BIT)){
				third_best_page = i; //the page is referenced, but not modified
			}
		}
	}
	//if the next code is executed, then the memory is full and we need to do a page replacement
	if(phy_page_number==-1){
		if(second_best_page != -1) phy_page_number = second_best_page;
		else if(third_best_page != -1) phy_page_number = third_best_page;
		else phy_page_number = 0;
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

sh_mem_tbl_entry* get_sh_mem(char* tag){
	sh_mem_tbl_entry* pointer = SH_MEM_TBL.next;
	while(pointer!=(sh_mem_tbl_entry*)-1){
		if(strcmp(pointer->tag,tag)==0) return pointer;
		pointer = pointer->next;
	}
	return (sh_mem_tbl_entry*)-1;
}

void create_sh_mem_entry(char* tag, INT32 pid, UINT16* pg_entry, INT32 size){ //create a node in the list of shared memory
	sh_mem_tbl_entry* new_node = malloc(sizeof(shared_memory_entry));
	sh_mem_tbl_entry* pointer = &SH_MEM_TBL;
	shared_memory_entry* sh_mem_info = malloc(sizeof(shared_memory_entry));
	sh_mem_info->pid = pid;
	sh_mem_info->pg_tbl_entry = pg_entry;
	sh_mem_info->shared_id = 0; //the shared id is always starts with 0
	sh_mem_info->next = -1;
	//the first node of the entry is created
	//then create the entry
	new_node->info = sh_mem_info;
	new_node->next =-1;
	strcpy(new_node->tag, tag);
	new_node->size = size;
	new_node->phy_adds = calloc(size, sizeof(INT32));
	int i;
	for(i=0; i<size; i++) new_node->phy_adds[i] = find_free_phys_page(); //allocate the physical memory page for the share area
	set_area_valid(pg_entry, size, new_node->phy_adds);//set up the local page table entries
	while(pointer->next != (sh_mem_tbl_entry*)-1) pointer=pointer->next; //get to the end of the list
	pointer->next = new_node;
}

INT32 add_sharer_in_sh_mem(sh_mem_tbl_entry* shared_area_info, INT32 pid, UINT16* pg_entry){
	//first we prepare the node to be added
	shared_memory_entry* new_node = malloc(sizeof(shared_memory_entry));
	shared_memory_entry* master_node = shared_area_info->info;
	new_node->next= (shared_memory_entry*)-1; //it's the end of list
	new_node->pg_tbl_entry = pg_entry;
	new_node->pid = pid;
	//we don't know the shared id yet, let's count it
	INT32 shared_id = 1;
	while(master_node->next != (shared_memory_entry*)-1){
		shared_id++;
		master_node = master_node->next;
	}
	new_node->shared_id = shared_id;
	master_node->next = new_node;
	set_area_valid(pg_entry, shared_area_info->size, shared_area_info->phy_adds); //set the local page table entry to be valid
	return shared_id;
}

void set_area_valid(UINT16* pg_entry, INT32 size, INT32* phys_mem_adds){
	int i;
	for(i=0; i<size; i++) {
		pg_entry[i] = pg_entry[i] | PTBL_VALID_BIT; //set valid bit
		pg_entry[i] = pg_entry[i] | (phys_mem_adds[i] & PTBL_PHYS_PG_NO); //set the phys page number
	}
}
