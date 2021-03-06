/************************************************************************

        This code forms the base of the operating system you will
        build.  It has only the barest rudiments of what you will
        eventually construct; yet it contains the interfaces that
        allow test.c and z502.c to be successfully built together.

        Revision History:
        1.0 August 1990
        1.1 December 1990: Portability attempted.
        1.3 July     1992: More Portability enhancements.
                           Add call to sample_code.
        1.4 December 1992: Limit (temporarily) printout in
                           interrupt handler.  More portability.
        2.0 January  2000: A number of small changes.
        2.1 May      2001: Bug fixes and clear STAT_VECTOR
        2.2 July     2002: Make code appropriate for undergrads.
                           Default program start is in test0.
        3.0 August   2004: Modified to support memory mapped IO
        3.1 August   2004: hardware interrupt runs on separate thread
        3.11 August  2004: Support for OS level locking
	4.0  July    2013: Major portions rewritten to support multiple threads
************************************************************************/

#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"
#include 			 "p_manage.h"
#include			 "timer_manage.h"
#include			 "msg_manage.h"
#include			 "mem_management.h"

// These loacations are global and define information about the page table
extern UINT16        *Z502_PAGE_TBL_ADDR;
extern INT16         Z502_PAGE_TBL_LENGTH;

extern void          *TO_VECTOR [];

char                 *call_names[] = { "mem_read ", "mem_write",
                            "read_mod ", "get_time ", "sleep    ",
                            "get_pid  ", "create   ", "term_proc",
                            "suspend  ", "resume   ", "ch_prior ",
                            "send     ", "receive  ", "disk_read",
                            "disk_wrt ", "def_sh_ar" };
INT32 MEM_PRINTER_CTRL = 0;
INT32 STATE_PRINTER_CTRL = 0;
#define MEM_PRINTER_NO 0
#define MEM_PRINTER_LIMITED 1
#define MEM_PRINTER_FULL 2
#define MEM_PRINTER_FREQUENCY 100
#define STATE_PRINTER_NO 0
#define STATE_PRINTER_LIMITED 1
#define STATE_PRINTER_FULL 2
#define STATE_PRINTER_FREQUENCY 10
/************************************************************************
   Internal routine for system calls.
************************************************************************/
void svc_sleep(SYSTEM_CALL_DATA *SystemCallData);
void svc_create_process(SYSTEM_CALL_DATA *SystemCallData);
void svc_terminate_process(SYSTEM_CALL_DATA *SystemCallData);
void svc_get_process_id(char* process_name,long* process_id,long* err_info);
void svc_suspend_process(INT32 pid,long* err_info);
void svc_resume_process(INT32 pid, long* err_info);
void svc_change_priority(INT32 pid, INT32 priority, long*err_info);
void svc_send_message(INT32 target_pid, char* msg, INT32 msg_length, long *err_info);
void svc_receive_message(INT32 source_pid, char* buffer, INT32 receive_length, INT32 *actual_length, INT32 *actual_source, long* err_info);
void state_print(char* action, INT32 target_pid);
void page_fault(int vpn);
void svc_share_memory(INT32 vpn, INT32 size, char* tag, INT32* id, INT32* err_info );
/************************************************************************
Internal routine for interrupts.
************************************************************************/
void clock_interrupt_handler();
void disk_interrupt_handler(INT32 disk_id);
void mem_printer();



/************************************************************************
    INTERRUPT_HANDLER
        When the Z502 gets a hardware interrupt, it transfers control to
        this routine in the OS.
************************************************************************/

void    interrupt_handler( void ) {
    INT32              device_id;
    INT32              status;
    INT32              Index = 0;

    // Get cause of interrupt
    MEM_READ(Z502InterruptDevice, &device_id );
    // Set this device as target of our query
    MEM_WRITE(Z502InterruptDevice, &device_id );
    // Now read the status of this device
    MEM_READ(Z502InterruptStatus, &status );
    if(device_id==TIMER_INTERRUPT)  clock_interrupt_handler(); //need add switch here
    else if(device_id>=DISK_INTERRUPT && device_id<=LARGEST_STAT_VECTOR_INDEX) disk_interrupt_handler(device_id-DISK_INTERRUPT+1);
    else{
		printf( "ERROR!  Interrupt device not recognized!\n" );
		printf( "Call_type is - %i\n", device_id);
    }
    MEM_WRITE(Z502InterruptClear, &Index ); // Clear out this device - we're done with it
}                                       /* End of interrupt_handler */
/************************************************************************
    FAULT_HANDLER
        The beginning of the OS502.  Used to receive hardware faults.
************************************************************************/

void    fault_handler( void )
    {
    INT32       device_id;
    INT32       status;
    INT32       Index = 0;

    // Get cause of interrupt
    MEM_READ(Z502InterruptDevice, &device_id );
    // Set this device as target of our query
    MEM_WRITE(Z502InterruptDevice, &device_id );
    // Now read the status of this device
    MEM_READ(Z502InterruptStatus, &status );

    //printf( "Fault_handler: Found vector type %d with value %d\n",
                        //device_id, status );
    switch(device_id){
    	case PRIVILEGED_INSTRUCTION:
    		printf("Permission Denied!");
    		CALL(Z502Halt());
    		break;
    	case INVALID_MEMORY:
    		page_fault(status);
    		break;
        default:
            printf( "ERROR!  fault type not recognized!\n" );
            printf( "Call_type is - %i\n", device_id);
    }
    // Clear out this device - we're done with it
    MEM_WRITE(Z502InterruptClear, &Index );
}                                       /* End of fault_handler */

/************************************************************************
    SVC
        The beginning of the OS502.  Used to receive software interrupts.
        All system calls come to this point in the code and are to be
        handled by the student written code here.
        The variable do_print is designed to print out the data for the
        incoming calls, but does so only for the first ten calls.  This
        allows the user to see what's happening, but doesn't overwhelm
        with the amount of data.
************************************************************************/

void    svc( SYSTEM_CALL_DATA *SystemCallData ) {
    short               call_type;
    static short        do_print = 10;
    short               i;
	INT32				Time;

    call_type = (short)SystemCallData->SystemCallNumber;
    if ( do_print > 0 ) {
        printf( "SVC handler: %s\n", call_names[call_type]);
        for (i = 0; i < SystemCallData->NumberOfArguments - 1; i++ ){
        	 //Value = (long)*SystemCallData->Argument[i];
             printf( "Arg %d: Contents = (Decimal) %8ld,  (Hex) %8lX\n", i,
             (unsigned long )SystemCallData->Argument[i],
             (unsigned long )SystemCallData->Argument[i]);
        }
    do_print--;
    }
	switch (call_type) {
        // Get time service call
        case SYSNUM_GET_TIME_OF_DAY:   // This value is found in syscalls.h
            CALL( MEM_READ( Z502ClockStatus, &Time ) );
            *(INT32 *)SystemCallData->Argument[0] = Time;
            break;
        // terminate system call
        case SYSNUM_TERMINATE_PROCESS:
        	svc_terminate_process(SystemCallData);
        	//Z502Halt();
            break;
        case SYSNUM_SLEEP: //If SLEEP is called
        	svc_sleep(SystemCallData);
        	break;
        case SYSNUM_CREATE_PROCESS:
        	svc_create_process(SystemCallData);
        	break;
        case SYSNUM_GET_PROCESS_ID:
        {
        	char* process_name= (char*) SystemCallData->Argument[0];
        	long* process_id = SystemCallData->Argument[1];
        	long* err_info = SystemCallData->Argument[2];
        	svc_get_process_id(process_name, process_id, err_info);
        	break;
        }
        case SYSNUM_SUSPEND_PROCESS:{
        	INT32 pid = (INT32)SystemCallData->Argument[0];
        	long *err_info = SystemCallData->Argument[1];
        	svc_suspend_process(pid, err_info);
        	break;
        }
        case SYSNUM_RESUME_PROCESS:{
        	INT32 pid = (INT32)SystemCallData->Argument[0];
        	long *err_info = SystemCallData->Argument[1];
        	svc_resume_process(pid, err_info);
        	break;
        }
        case SYSNUM_CHANGE_PRIORITY:{
        	INT32 pid = (INT32)SystemCallData->Argument[0];
        	INT32 priority = (INT32)SystemCallData->Argument[1];
        	long *err_info = SystemCallData->Argument[2];
        	svc_change_priority(pid, priority, err_info);
        	break;
        }
        case SYSNUM_SEND_MESSAGE:{
        	INT32 target_pid = (INT32)SystemCallData->Argument[0];
        	char *msg = (char *) SystemCallData->Argument[1];
        	INT32 msg_length = (INT32)SystemCallData->Argument[2];
        	long *err_info = SystemCallData->Argument[3];
        	svc_send_message(target_pid, msg, msg_length, err_info);
        	break;
        }
        case SYSNUM_RECEIVE_MESSAGE:{
        	INT32 source_pid = (INT32)SystemCallData->Argument[0];
        	char *buffer = (char *) SystemCallData->Argument[1];
        	INT32 receive_length = (INT32)SystemCallData->Argument[2];
        	INT32 *actual_length = (INT32 *) SystemCallData->Argument[3];
        	INT32 *actual_pid = (INT32 *)SystemCallData->Argument[4];
        	long *err_info = SystemCallData->Argument[5];
        	svc_receive_message(source_pid, buffer, receive_length, actual_length, actual_pid, err_info);
        	break;
        }
        case SYSNUM_DISK_WRITE:{
        	INT32 disk_id = (INT32)SystemCallData->Argument[0];
        	INT32 sector = (INT32)SystemCallData->Argument[1];
        	char *buffer = (char*)SystemCallData->Argument[2];
        	disk_write(disk_id, sector, buffer);
        	break;
        }
        case SYSNUM_DISK_READ:{
            INT32 disk_id = (INT32)SystemCallData->Argument[0];
            INT32 sector = (INT32)SystemCallData->Argument[1];
            char *buffer = (char*)SystemCallData->Argument[2];
            disk_read(disk_id, sector, buffer);
            break;
        }
        case SYSNUM_DEFINE_SHARED_AREA:{
        	INT32 virtual_page_number = (INT32)SystemCallData->Argument[0]/PGSIZE;
        	INT32 size = (INT32)SystemCallData->Argument[1];
        	char *tag = (char*)SystemCallData->Argument[2];
        	INT32* share_id = (INT32*)SystemCallData->Argument[3];
        	INT32* err_info = (INT32*)SystemCallData->Argument[4];
        	svc_share_memory(virtual_page_number, size, tag, share_id, err_info);
        	break;
        }
        default:  
            printf( "ERROR!  call_type not recognized!\n" ); 
            printf( "Call_type is - %i\n", call_type);
    }
}                                               // End of svc


/************************************************************************
    osInit
        This is the first routine called after the simulation begins.  This
        is equivalent to boot code.  All the initial OS components can be
        defined and initialized here.
************************************************************************/

void    osInit( int argc, char *argv[]  ) {
    PCB                *pcb;
    INT32               i;

    /* Demonstrates how calling arguments are passed thru to here       */

    printf( "Program called with %d arguments:", argc );
    for ( i = 0; i < argc; i++ )
        printf( " %s", argv[i] );
    printf( "\n" );
    printf( "Calling with argument 'sample' executes the sample program.\n" );

    /*          Setup so handlers will come to code in base.c           */

    TO_VECTOR[TO_VECTOR_INT_HANDLER_ADDR]   = (void *)interrupt_handler;
    TO_VECTOR[TO_VECTOR_FAULT_HANDLER_ADDR] = (void *)fault_handler;
    TO_VECTOR[TO_VECTOR_TRAP_HANDLER_ADDR]  = (void *)svc;

    /*  Determine if the switch was set, and if so go to demo routine.  */
    if ( argc > 1) {
    	if(strcmp(argv[1],"sample") == 0){
    		printf("sample code is chosen, now run sample \n");
    		pcb = create_process((void *)sample_code, KERNEL_MODE, 0, "sample");
    	    run_process (pcb);
    	}
    	else if(strcmp(argv[1],"test0") == 0 || strcmp(argv[1],"0") == 0){
    		printf("test0 is chosen, now run test0 \n");
    		pcb = create_process((void *)test0, USER_MODE ,0, "test0");
    		run_process (pcb);
    	}
    	else if(strcmp(argv[1],"test1a") == 0 || strcmp(argv[1],"1a") == 0){
    		printf("test1a is chosen, now run test1a \n");
    		pcb = create_process((void *)test1a, USER_MODE ,0, "test1a");
    		run_process (pcb);
    	}
    	else if(strcmp(argv[1],"test1b") == 0 || strcmp(argv[1],"1b") == 0){
    		printf("test1b is chosen, now run test1b \n");
    		pcb = create_process((void *)test1b, USER_MODE ,0, "test1b");
    		run_process (pcb);
    	}
    	else if(strcmp(argv[1],"test1c") == 0 || strcmp(argv[1],"1c") == 0){
			printf("test1c is chosen, now run test1c \n");
			pcb = create_process((void *)test1c, USER_MODE ,0, "test1c");
			run_process (pcb);
    	}
    	else if(strcmp(argv[1],"test1d") == 0 || strcmp(argv[1],"1d") == 0){
			printf("test1d is chosen, now run test1d \n");
			pcb = create_process((void *)test1d, USER_MODE ,0, "test1d");
			run_process (pcb);
    	}
    	else if(strcmp(argv[1],"test1e") == 0 || strcmp(argv[1],"1e") == 0){
			printf("test1e is chosen, now run test1e \n");
			pcb = create_process((void *)test1e, USER_MODE ,0, "test1e");
			run_process (pcb);
    	}
    	else if(strcmp(argv[1],"test1f") == 0 || strcmp(argv[1],"1f") == 0){
			printf("test1f is chosen, now run test1f \n");
			pcb = create_process((void *)test1f, USER_MODE ,0, "test1f");
			run_process (pcb);
    	}
    	else if(strcmp(argv[1],"test1g") == 0 || strcmp(argv[1],"1g") == 0){
			printf("test1g is chosen, now run test1g \n");
			pcb = create_process((void *)test1g, USER_MODE ,0, "test1g");
			run_process (pcb);
    	}
    	else if(strcmp(argv[1],"test1h") == 0 || strcmp(argv[1],"1h") == 0){
			printf("test1h is chosen, now run test1h \n");
			pcb = create_process((void *)test1h, USER_MODE ,0, "test1h");
			run_process (pcb);
    	}
    	else if(strcmp(argv[1],"test1i") == 0 || strcmp(argv[1],"1i") == 0){
			printf("test1i is chosen, now run test1i \n");
			pcb = create_process((void *)test1i, USER_MODE ,0, "test1i");
			run_process (pcb);
    	}
    	else if(strcmp(argv[1],"test1j") == 0 || strcmp(argv[1],"1j") == 0){
			printf("test1j is chosen, now run test1j \n");
			pcb = create_process((void *)test1j, USER_MODE ,0, "test1j");
			run_process (pcb);
    	}
    	else if(strcmp(argv[1],"test1k") == 0 || strcmp(argv[1],"1k") == 0){
			printf("test1k is chosen, now run test1k \n");
			pcb = create_process((void *)test1k, USER_MODE ,0, "test1k");
			run_process (pcb);
    	}
    	else if(strcmp(argv[1],"test2a") == 0 || strcmp(argv[1],"2a") == 0){
    		MEM_PRINTER_CTRL = MEM_PRINTER_FULL; //set the memory printer to be full
			printf("test2a is chosen, now run test2a \n");
			pcb = create_process((void *)test2a, USER_MODE ,0, "test2a");
			run_process (pcb);
    	}
    	else if(strcmp(argv[1],"test2b") == 0 || strcmp(argv[1],"2b") == 0){
    		MEM_PRINTER_CTRL = MEM_PRINTER_FULL;
			printf("test2b is chosen, now run test2b \n");
			pcb = create_process((void *)test2b, USER_MODE ,0, "test2b");
			run_process (pcb);
    	}
    	else if(strcmp(argv[1],"test2c") == 0 || strcmp(argv[1],"2c") == 0){
    		STATE_PRINTER_CTRL = STATE_PRINTER_NO;
			printf("test2c is chosen, now run test2c \n");
			pcb = create_process((void *)test2c, USER_MODE ,0, "test2c");
			run_process (pcb);
    	}
    	else if(strcmp(argv[1],"test2d") == 0 || strcmp(argv[1],"2d") == 0){
    		STATE_PRINTER_CTRL=STATE_PRINTER_LIMITED;
			printf("test2d is chosen, now run test2d \n");
			pcb = create_process((void *)test2d, USER_MODE ,0, "test2d");
			run_process (pcb);
    	}
    	else if(strcmp(argv[1],"test2e") == 0 || strcmp(argv[1],"2e") == 0){
    		MEM_PRINTER_CTRL = MEM_PRINTER_LIMITED;
    		STATE_PRINTER_CTRL=STATE_PRINTER_LIMITED;
			printf("test2e is chosen, now run test2e \n");
			pcb = create_process((void *)test2e, USER_MODE ,0, "test2e");
			run_process (pcb);
    	}
    	else if(strcmp(argv[1],"test2f") == 0 || strcmp(argv[1],"2f") == 0){
    		MEM_PRINTER_CTRL = MEM_PRINTER_LIMITED;
			printf("test2f is chosen, now run test2f \n");
			pcb = create_process((void *)test2f, USER_MODE ,0, "test2f");
			run_process (pcb);
		}
    	else if(strcmp(argv[1],"test2g") == 0 || strcmp(argv[1],"2g") == 0){
			printf("test2g is chosen, now run test2g \n");
			pcb = create_process((void *)test2g, USER_MODE ,0, "test2g");
			run_process (pcb);
		}
    	else if(strcmp(argv[1],"test2h") == 0 || strcmp(argv[1],"2h") == 0){
			printf("test2h is chosen, now run test2h \n");
			pcb = create_process((void *)test2h, USER_MODE ,0, "test2h");
			run_process (pcb);
		}
    }
    else{
    	STATE_PRINTER_CTRL = STATE_PRINTER_FULL;
    	MEM_PRINTER_CTRL = MEM_PRINTER_FULL;
    	printf("No switch set, run test2a now \n");
    	pcb = create_process( (void *)test2a, USER_MODE ,0, "test2a");
    	run_process(pcb);
    }
}                                               // End of osInit

/***********************************************************************
 * System Call Service Routine
 **********************************************************************/

void svc_sleep(SYSTEM_CALL_DATA *SystemCallData){
	INT32 sleeptime, time, waketime;
	PCB *current;
	sleeptime = (INT32)SystemCallData->Argument[0];
	MEM_READ( Z502ClockStatus, &time);
	waketime = time + sleeptime;
	current = get_current_pcb();
	INT32 next_alarm = add_time_queue(current, waketime);
	sleeptime = next_alarm - time; //get the next alarm time
	if(sleeptime <= 0) sleeptime = 1; //if alarm time is negative
	MEM_WRITE(Z502TimerStart, &sleeptime);
	//LOCK when do the state print
	CALL(state_print("sleep", current->pid));
	current = dispatcher(); //check if any process wait in ready queue
	while((INT32)current==-1) {
		Z502Idle();
		current = dispatcher();
	}
	state_print("dispatch", current->pid);
	run_process(current);
}  // End of svc_sleep

void svc_create_process(SYSTEM_CALL_DATA *SystemCallData){
	char *name = (char*)SystemCallData->Argument[0];
	void *code_address = (void*)SystemCallData->Argument[1];
	INT32 priority = (INT32)SystemCallData->Argument[2];
	PCB *pcb = create_process(code_address, USER_MODE, priority, name);
	if((long)pcb < 0){
		*(SystemCallData->Argument[4])=(long)pcb;
	}
	else{
		*(SystemCallData->Argument[4])=ERR_SUCCESS;
		*(SystemCallData->Argument[3])=pcb->pid;
		add_ready_queue(pcb);
	}
	//print_ready_queue();
} //End of svc_create_process

void svc_terminate_process(SYSTEM_CALL_DATA *SystemCallData){
	INT32 pid = (INT32)SystemCallData->Argument[0];
	if(pid==-2)
		Z502Halt();
	if(pid==-1)
		pid = get_current_pcb()->pid;
	INT32 result=terminate_process(pid);
	*(SystemCallData->Argument[1]) = result;
	PCB *pcb_to_run = dispatcher();
	if((INT32)pcb_to_run!=-1) run_process(pcb_to_run);
	else {
		//check if there's anything in timerQ
		timequeue_node* timeq_head = get_timer_queue_head();
		if ((INT32)timeq_head->next == -1) Z502Halt();
		else {
			while((INT32)pcb_to_run==-1){
				Z502Idle();
				pcb_to_run = dispatcher();
			}
			run_process(pcb_to_run);
		}
	}
	//print_ready_queue();
} //End of svc terminate_process

void svc_get_process_id(char* process_name,long* process_id,long* err_info){
	if (strcmp(process_name,"")==0) {
		PCB *current_pcb = get_current_pcb();
		*process_id = current_pcb->pid;
		*err_info = ERR_SUCCESS;
		return;
	}
	INT32 pid = get_process_id(process_name);
	if(pid < 0) pid = get_process_id_in_timer_queue(process_name); //check if the process is in time queue
	if(pid < 0) pid = get_process_id_in_disk_queue(process_name);
	if(pid < 0){
		*process_id = -1;
		*err_info = pid;
		return;
	}
	*process_id = pid;
	*err_info = ERR_SUCCESS;
}

void svc_resume_process(INT32 pid, long* err_info){
	INT32 result = resume_process(pid);
	if(result==-1){
		PCB* pcb=get_pcb_from_timer_queue(pid);
		if((INT32)pcb!=-1){
			pcb->suspend_flag = 0;
		}
		else *err_info=-1;
	}
	else *err_info=ERR_SUCCESS;
	state_print("resume", pid);
}

void svc_suspend_process(INT32 pid,long* err_info){
	PCB *current_run = get_current_pcb();
	if(current_run->pid!=pid && pid!=-1){
		PCB* pcb = get_pcb_from_ready_queue(pid);
		if((INT32)pcb==-1){
			pcb = get_pcb_from_timer_queue(pid);
			if((INT32)pcb==-1){
				*err_info = -1;
			}
			else{
				pcb->suspend_flag =1;
			}
		}
		else *err_info = suspend_process(pid);
	}
	else{ //if the process ask to suspend itself
		if(is_ready_queue_empty()){ //if there's nothing in ready q
			*err_info = suspend_process(pid);
			state_print("never", 0);
			PCB* next_run = dispatcher();
			while(next_run==-1){
				Z502Idle();
				next_run = dispatcher();
			}
			run_process(next_run);
			return;
		}
		else{ // if there's something in readyq, run it.
			*err_info = suspend_process(pid);
			state_print("suspend",pid); //check the state first
			PCB* next_run = dispatcher();
			run_process(next_run);
			return; //do not print the state when resume
		}
	}
	state_print("suspend",pid);
}

void svc_change_priority(INT32 pid, INT32 priority, long *err_info){
	if(pid==-1){
		PCB *pcb = get_current_pcb();
		pcb->priority = priority;
		*err_info = ERR_SUCCESS;
		return;
	}
	if(priority<0 || priority > 100){
		*err_info = -2;
		return;
	}
	INT32 result = change_priority_in_ready_queue(pid, priority);
	if(result==ERR_SUCCESS){
		*err_info = ERR_SUCCESS;
		state_print("ChPrior", pid);
		return;
	}
	result = change_priority_in_timer_queue(pid, priority);
	if(result == ERR_SUCCESS){
		*err_info = ERR_SUCCESS;
		state_print("ChPrior",pid);
		return;
	}
	*err_info = -1;
}

void svc_send_message(INT32 target_pid, char* msg, INT32 msg_length, long *err_info){
	if(target_pid != -1 && target_pid != (get_current_pcb())->pid){
		if(check_process_in_readyQ(target_pid)<0){
			if(check_process_in_suspendQ(target_pid)<0){
				if(check_process_in_timerQ(target_pid<0)){
					*err_info = -1;
					return;
				}
			}
		}
	}
	if(msg_length>100){
		*err_info = -2;
		return;
	}
	INT32 sender_pid = (get_current_pcb())->pid;
	INT32 need_kick = send_msg(sender_pid, target_pid, msg, msg_length, err_info);
	if(need_kick) svc_resume_process(target_pid, err_info);
}

void svc_receive_message(INT32 source_pid, char* buffer, INT32 receive_length, INT32 *actual_length, INT32 *actual_source, long* err_info){
	if(source_pid != -1 && source_pid != (get_current_pcb())->pid){
		if(check_process_in_readyQ(source_pid)<0){
			if(check_process_in_suspendQ(source_pid)<0){
				if(check_process_in_timerQ(source_pid<0)){
					*err_info = -1;
					return;
				}
			}
		}
	}
	if(receive_length>100){
		*err_info = -2;
		return;
	}
	INT32 receiver_pid = (get_current_pcb())->pid;
	INT32 receive_immediately = receive_msg(receiver_pid, source_pid, buffer, receive_length, actual_length, actual_source, err_info);
	if(receive_immediately==0) {
		svc_suspend_process(receiver_pid, err_info);
		get_msg();
	}
}

void clock_interrupt_handler(){
	PCB* pcb;
	INT32 next_alarm, sleeptime, current_time;
	MEM_READ( Z502ClockStatus, &current_time);
	pcb = get_wake_up_pcb();
	if(pcb->suspend_flag){
		pcb->suspend_flag = 0; //clear the flag
		add_suspend_queue(pcb);
	}
	else add_ready_queue(pcb);
	//if (strcmp(pcb->name, "test1c")==0) printf("1c is going back! \n");
	state_print("wakeup",pcb->pid);
	next_alarm = get_next_alarm();
	if( next_alarm < 0 ) return; //there's no item in time queue, do nothing.
	else {
		sleeptime = next_alarm - current_time;
		if(sleeptime<=0) sleeptime = 1;
		MEM_WRITE(Z502TimerStart, &sleeptime);
		//pcb = dispatcher();
		//run_process(pcb);
	}
}

void disk_interrupt_handler(INT32 disk_id){
	PCB* next_to_run = get_next(disk_id);
	add_ready_queue(next_to_run);
	state_print("d_int", next_to_run->pid);
}

void state_print(char* action, INT32 target_pid){
	static int scheduler_counter = 0;
	scheduler_counter += 1;
	switch(STATE_PRINTER_CTRL){//now we want to limit the use of state printer in project phase 2
		case STATE_PRINTER_NO:
			return;
			break;
		case STATE_PRINTER_LIMITED:
			if(scheduler_counter % STATE_PRINTER_FREQUENCY != 0) return;
			break;
	}
	INT32 current_time;
	MEM_READ( Z502ClockStatus, &current_time); //read current time
	INT32 lock_result;
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 2, 1, TRUE, &lock_result); //Lock the printer
	SP_setup(SP_TIME_MODE, current_time);
	SP_setup_action(SP_ACTION_MODE, action);
	SP_setup(SP_NEW_MODE, 0);
	//SP_setup(SP_TERMINATED_MODE, 0 );
	SP_setup(SP_TARGET_MODE, target_pid);
	print_ready_queue();
	print_time_queue();
	print_suspend_queue();
	print_disk_queue(get_current_pcb()->pid);
	SP_print_line();
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 2 , 0, TRUE, &lock_result); //unlock the printer
}

/***
 * Page Fault Routines--------------------------------------------------------------------
 */
void page_fault(INT32 vpn){
	if(Z502_PAGE_TBL_ADDR==0) 	Z502_PAGE_TBL_ADDR = calloc(VIRTUAL_MEM_PAGES,sizeof(short));
	if(Z502_PAGE_TBL_LENGTH==0) 	Z502_PAGE_TBL_LENGTH = VIRTUAL_MEM_PAGES;
	if(vpn >= VIRTUAL_MEM_PAGES || vpn < 0 ){ //check if the vpn is legal
		printf("\n\x1b[34m"
			   "************************************************************************\n"
			   "*                                                                      *\n"
			   "*                    Blue Screen      CS502soft                        *\n"
			   "*                                                                      *\n"
			   "*                     Fatal: Illegal Memory Address                    *\n"
			   "*                        0x%x0f1adc0f1cdba                            *\n"
			   "*                     Cannot be written or read                        *\n"
			   "*                                                                      *\n"
			   "*     If this is the first time you've see this stop error screen,     *\n"
			   "*     restart your computer. If this screen appears again, follow      *\n"
			   "*     these steps:                                                     *\n"
			   "*                                                                      *\n"
			   "*             Beg professor do not touch illegal page                  *\n"
			   "*                                                                      *\n"
			   "************************************************************************\n"
			   "\x1b[0m",vpn);
		Z502Halt();
	}
	init_page(vpn, get_current_pcb()->pid); //in the mem_management block, os will find a proper phys_page
	mem_printer();
}

void svc_share_memory(INT32 vpn, INT32 size, char* tag, INT32* id, INT32* err_info ){
	if(Z502_PAGE_TBL_ADDR==0) 	Z502_PAGE_TBL_ADDR = calloc(VIRTUAL_MEM_PAGES,sizeof(short)); //first initial the page table if it has not been initialized
	if(Z502_PAGE_TBL_LENGTH==0) 	Z502_PAGE_TBL_LENGTH = VIRTUAL_MEM_PAGES;
	sh_mem_tbl_entry* sh_mem = get_sh_mem(tag);
	if(sh_mem == (sh_mem_tbl_entry*) -1){
		create_sh_mem_entry(tag, get_current_pcb()->pid, &Z502_PAGE_TBL_ADDR[vpn], size);
		*id = 0;
	}
	else{
		*id = add_sharer_in_sh_mem(sh_mem, get_current_pcb()->pid, &Z502_PAGE_TBL_ADDR[vpn]);
	}
	/**this is for debug
	int i;
	for(i=0; i<size; i++){
		printf("Page:%d   Content:%d \n", vpn+i, Z502_PAGE_TBL_ADDR[vpn+i]);
	}
	//debug end **/
	*err_info = ERR_SUCCESS;
}

void mem_printer(){
	static mem_printer_counter = 0;
	mem_printer_counter += 1;
	switch(MEM_PRINTER_CTRL){
		case MEM_PRINTER_NO:
			break;
		case MEM_PRINTER_LIMITED:
			if(mem_printer_counter % MEM_PRINTER_FREQUENCY == 1) print_phys_mem();
			break;
		case MEM_PRINTER_FULL:
			print_phys_mem();
			break;
	}
}
