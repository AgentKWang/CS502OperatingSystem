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

/************************************************************************
   Internal routine for system calls.
************************************************************************/
void svc_sleep(SYSTEM_CALL_DATA *SystemCallData);
void svc_create_process(SYSTEM_CALL_DATA *SystemCallData);
void svc_terminate_process(SYSTEM_CALL_DATA *SystemCallData);
void svc_get_process_id(char* process_name,long* process_id,long* err_info);


/************************************************************************
Internal routine for interrupts.
************************************************************************/
void clock_interrupt_handler();






/************************************************************************
    INTERRUPT_HANDLER
        When the Z502 gets a hardware interrupt, it transfers control to
        this routine in the OS.
************************************************************************/

void    interrupt_handler( void ) {
    INT32              device_id;
    INT32              status;
    INT32              Index = 0;
    static BOOL        remove_this_in_your_code = TRUE;   /** TEMP **/
    static INT32       how_many_interrupt_entries = 0;    /** TEMP **/

    // Get cause of interrupt
    MEM_READ(Z502InterruptDevice, &device_id );
    // Set this device as target of our query
    MEM_WRITE(Z502InterruptDevice, &device_id );
    // Now read the status of this device
    MEM_READ(Z502InterruptStatus, &status );
    //printf("Aha! Here is the Interrupt_handler!!");
    /** REMOVE THE NEXT SIX LINES **/
    how_many_interrupt_entries++;                         /** TEMP **/
    if ( remove_this_in_your_code && ( how_many_interrupt_entries < 20 ) )
        {
        printf( "Interrupt_handler: Found device ID %d with status %d\n",
                        device_id, status );
    }
    clock_interrupt_handler(); //need add switch here
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

    printf( "Fault_handler: Found vector type %d with value %d\n",
                        device_id, status );
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
    }
    else{
    	printf("No switch set, run test1c now \n");
    	pcb = create_process( (void *)test1c, USER_MODE ,0, "test1c");
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
	//LOCK when do the state print
	INT32 lock_result;
	READ_MODIFY(MEMORY_INTERLOCK_BASE+2, 1, TRUE, &lock_result); //unlock the ready_queue
	SP_setup_action(SP_ACTION_MODE, "SLEEP");
	SP_setup(SP_NEW_MODE, 0);
	//SP_setup(SP_TERMINATED_MODE, 0 );
	SP_setup(SP_TIME_MODE, time);
	SP_setup(SP_TARGET_MODE, current->pid);
	print_time_queue();
	print_ready_queue();
	SP_print_line();
	READ_MODIFY(MEMORY_INTERLOCK_BASE+2, 0, TRUE, &lock_result); //unlock the ready_queue
	sleeptime = next_alarm - time; //get the next alarm time
	if(sleeptime <= 0) sleeptime = 1; //if alarm time is negative
	MEM_WRITE(Z502TimerStart, &sleeptime);
	current = dispatcher(); //check if any process wait in ready queue
	while(current==-1) {
		Z502Idle();
		current = dispatcher();
	}
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 2, 1, TRUE, &lock_result); //Lock the printer
	MEM_READ( Z502ClockStatus, &time); //read current time
	SP_setup(SP_TIME_MODE, time);
	SP_setup_action(SP_ACTION_MODE, "Dispatch");
	SP_setup(SP_NEW_MODE, 0);
	//SP_setup(SP_TERMINATED_MODE, 0 );
	SP_setup(SP_TARGET_MODE, current->pid);
	print_ready_queue();
	print_time_queue();
	SP_print_line();
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 2 , 0, TRUE, &lock_result); //unlock the printer
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
	if(pcb_to_run!=-1) run_process(pcb_to_run);
	else {
		//check if there's anything in timerQ
		timequeue_node* timeq_head = get_timer_queue_head();
		if (timeq_head->next == -1) Z502Halt();
		else {
			while(pcb_to_run==-1){
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
	if(pid < 0) {
		pid = get_process_id_in_timer_queue(process_name); //check if the process is in time queue
		if(pid < 0){
			*process_id = -1;
			*err_info = pid;
		}
	}
	else{
		*process_id = pid;
		*err_info = ERR_SUCCESS;
	}
}


void clock_interrupt_handler(){
	PCB* pcb;
	INT32 next_alarm, sleeptime, current_time;
	MEM_READ( Z502ClockStatus, &current_time);
	pcb = get_wake_up_pcb();
	add_ready_queue(pcb);
	//if (strcmp(pcb->name, "test1c")==0) printf("1c is going back! \n");
	INT32 lock_result;
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 2, 1, TRUE, &lock_result); //Lock the printer
	SP_setup(SP_TIME_MODE, current_time);
	SP_setup_action(SP_ACTION_MODE, "Wakeup");
	SP_setup(SP_NEW_MODE, 0);
	//SP_setup(SP_TERMINATED_MODE, 0 );
	SP_setup(SP_TARGET_MODE, pcb->pid);
	print_ready_queue();
	print_time_queue();
	SP_print_line();
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 2 , 0, TRUE, &lock_result); //unlock the printer
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


