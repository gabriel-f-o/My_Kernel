/*
 * cli_task.c
 *
 *  Created on: Jul 16, 2022
 *      Author: Gabriel
 */

#include "common.h"
#include "main.h"
#include "OS/OS_Core/OS_Internal.h"

#if ( defined(CLI_EN) && (CLI_EN == 1) )

/**********************************************************
 * PUBLIC VARIABLES
 **********************************************************/

extern os_list_head_t os_head;
extern os_list_head_t os_process_list;

/**********************************************************
 * PRIVATE FUNCTIONS
 **********************************************************/

static char* task_states[] = {
		[OS_TASK_NOT_EXIST] 		= "INEXISTENT",
		[OS_TASK_ENDED]				= "ENDED",
		[OS_TASK_READY]				= "READY",
		[OS_TASK_BLOCKED]			= "BLOCKED",
		[OS_TASK_DELETING]			= "DELETING",
};

static void top(){

	/* For each task
	 ------------------------------------------------------*/
	os_heap_mon_t mon = os_heap_monitor();
	os_list_cell_t* it = os_process_list.head.next;
	PRINTLN("");
	PRINTLN("Memory usage, Used = %lu, Free = %lu, Total = %lu, Used Perc = %lu.%lu %%", mon.used_size, mon.total_size - mon.used_size, mon.total_size, mon.used_size * 100 / mon.total_size, mon.used_size * 10000 / mon.total_size % 100);
	PRINTLN("Curent Tasks : ");
	PRINTLN("PID       name");
	while(it != NULL){
		PRINTLN("%05d     %-10s", (int)((os_process_t*)it->element)->PID, ((os_process_t*)it->element)->p_name == NULL ? "No name" : ((os_process_t*)it->element)->p_name);
		it = it->next;
	}
}

static void task_top(){

	/* For each task
	 ------------------------------------------------------*/
	os_heap_mon_t mon = os_heap_monitor();
	os_list_cell_t* it = os_head.head.next;
	PRINTLN("");
	PRINTLN("Memory usage, Used = %lu, Free = %lu, Total = %lu, Used Perc = %lu.%lu %%", mon.used_size, mon.total_size - mon.used_size, mon.total_size, mon.used_size * 100 / mon.total_size, mon.used_size * 10000 / mon.total_size % 100);
	PRINTLN("Curent Tasks : ");
	PRINTLN("PID       state           prio    name");
	while(it != NULL){
		char fullname[32] = "No name";
		if(((os_task_t*)it->element)->process != NULL){
			snprintf(fullname, sizeof(fullname), "[%s#%d]", ((os_task_t*)it->element)->process->p_name, os_list_searchIndex(((os_task_t*)it->element)->process->thread_list, it->element));
		}

		char* name = ((os_task_t*)it->element)->obj.name == NULL ? fullname : ((os_task_t*)it->element)->obj.name;

		PRINTLN("%05d     %-11s     %03d     %s", ((os_task_t*)it->element)->process == NULL ? 0 : ((os_task_t*)it->element)->process->PID, task_states[os_task_getState(((os_handle_t)it->element))],
												((os_task_t*)it->element)->priority, name);
		it = it->next;
	}
}

static void kill(){

	/* Get argument
	 ------------------------------------------------------*/
	uint16_t pid = cli_get_uint16_argument(0, NULL);

	/* Get task by PID
	 ------------------------------------------------------*/
	os_process_t* h = os_process_getByPID(pid);

	/* Delete task
	 ------------------------------------------------------*/
	os_process_kill(h);

	/* Feedback
	 ------------------------------------------------------*/
	if(h == NULL)
		PRINTLN("Process PID %d not found", pid);
	else{
		PRINTLN("Process PID %d killed", pid);
	}
}

static void exec(){

	/* Count arguments
	 ------------------------------------------------------*/
	int argc = 0;
	bool resStr = true;
	bool resInt= true;
	char buffer[50];

	/* For each argument
	 ------------------------------------------------------*/
	while(resInt == true || resStr == true){

		/* Get as string or int
		 ------------------------------------------------------*/
		cli_get_string_argument((size_t)argc, (uint8_t*)buffer, sizeof(buffer), &resStr);
		cli_get_int_argument((size_t)argc, &resInt);

		/* if none, stop
		 ------------------------------------------------------*/
		if(!(resInt == true || resStr == true)) break;

		/* Otherwise count and continue
		 ------------------------------------------------------*/
		argc++;
	}


	/* Allocate argv vector
	 ------------------------------------------------------*/
	char** argv = (char**)os_heap_alloc( (uint32_t)(argc * (int)sizeof(char*)) );

	/* Reinit variables
	 ------------------------------------------------------*/
	argc = 0;
	resStr = true;
	resInt= true;

	/* For each argument
	 ------------------------------------------------------*/
	while(resInt == true || resStr == true){

		/* Get as string and as integer
		 ------------------------------------------------------*/
		size_t len = cli_get_string_argument((size_t)argc, (uint8_t*)buffer, sizeof(buffer), &resStr);
		int32_t arg = cli_get_int32_argument((size_t)argc, &resInt);

		/* If it is a string
		 ------------------------------------------------------*/
		if(resStr){

			/* Allocate the argument string and copy from buffer
			 ------------------------------------------------------*/
			argv[argc] = (char*)os_heap_alloc( (len + 1) * sizeof(char) );
			strcpy(argv[argc], buffer);
		}

		/* If it is an integer
		 ------------------------------------------------------*/
		else if(resInt){

			/* Transform integer into string, allocate string argument and copy buffer
			 ------------------------------------------------------*/
			snprintf(buffer, sizeof(buffer), "%ld", arg);
			argv[argc] = (char*)os_heap_alloc( (strlen(buffer) + 1) * sizeof(char) );
			strcpy(argv[argc], buffer);
		}

		/* If none, break;
		 ------------------------------------------------------*/
		else{
			break;
		}

		/* otherwise count and continue
		 ------------------------------------------------------*/
		argc++;
	}

	/* Create process
	 ------------------------------------------------------*/
	os_err_e err = os_process_create(argv[0], argc, argv);
	if(err != OS_ERR_OK){
		PRINTLN("Error %ld", err);
	}
	else
		PRINTLN("Process created OK");
}

/**********************************************************
 * GLOBAL VARIABLES
 **********************************************************/

cliElement_t cliTasks[] = {
		cliActionElementDetailed("top", 		top, 		"", 	"Lists all processes",  							NULL),
		cliActionElementDetailed("task_top", 	task_top, 	"", 	"Lists all tasks",  								NULL),
		cliActionElementDetailed("kill", 		kill, 		"u", 	"Kill a task using PID",  							NULL),
		cliActionElementDetailed("exec", 		exec, 		"s...", "Executes an ELF file, passing arguments. Integers are transformed in string format",  		NULL),
		cliMenuTerminator()
};

#endif
