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

/**********************************************************
 * PRIVATE FUNCTIONS
 **********************************************************/

static void top(){

	/* For each task
	 ------------------------------------------------------*/
	os_list_cell_t* it = os_head.head.next;
	PRINTLN("Curent Tasks : ");
	PRINTLN("PID,   name");
	while(it != NULL){
		PRINTLN("%05d, %s", ((os_task_t*)it->element)->pid, ((os_task_t*)it->element)->obj.name == NULL ? "No name" : ((os_task_t*)it->element)->obj.name);
		it = it->next;
	}
}

static void exec(){

	/* Get arguments
	 --------------------------------------------------*/
	char name[50];
	size_t len = cli_get_string_argument(0, (uint8_t*)name, sizeof(name), NULL);
	uint32_t addr = cli_get_uint32_argument(1, NULL);

	/* Open file
	 --------------------------------------------------*/
	lfs_file_t file;
	int err = lfs_file_open(&lfs, &file, name, LFS_O_RDONLY);
	if(err < 0){
		PRINTLN("Open Error");
		return;
	}

	/* Allocate name
	 --------------------------------------------------*/
	char* taskName = (char*)os_heap_alloc(len);
	if(taskName == NULL){
		PRINTLN("Aloc error");
		return;
	}

	snprintf(taskName, len, "%s", name);

	/* Allocate code space
	 --------------------------------------------------*/
	uint8_t* code = (uint8_t*)os_heap_alloc((uint32_t)lfs_file_size(&lfs, &file));
	if(code == NULL){
		PRINTLN("Aloc error");
		os_heap_free(taskName);
		return;
	}

	/* Load file in RAM
	 --------------------------------------------------*/
	lfs_ssize_t rErr = lfs_file_read(&lfs, &file, code, (lfs_size_t)lfs_file_size(&lfs, &file));
	if(rErr < 0){
		PRINTLN("read Error");
		return;
	}

	/* Close file
	 --------------------------------------------------*/
	err = lfs_file_close(&lfs, &file);
	if(err < 0)
		PRINTLN("close error");

	/* Calculate main address
	 --------------------------------------------------*/
	addr -= FLASH_BASE_ADDR;
	addr += (uint32_t)&code[0];

	/* Create task
	 --------------------------------------------------*/
	os_handle_t h;
	os_task_create(&h, taskName, (void* (*)(void*))(addr), 10, OS_DEFAULT_STACK_SIZE, NULL);
}

static void kill(){

	/* Get argument
	 ------------------------------------------------------*/
	uint16_t pid = cli_get_uint16_argument(0, NULL);

	/* Get task by PID
	 ------------------------------------------------------*/
	os_handle_t h = os_task_getByPID(pid);

	/* Delete task
	 ------------------------------------------------------*/
	os_task_delete(h);

	if(h == NULL)
		PRINTLN("Task PID %d not found", pid);
	else{
		PRINTLN("Task PID %d killed", pid);
	}
}

/**********************************************************
 * GLOBAL VARIABLES
 **********************************************************/

cliElement_t cliTasks[] = {
		cliActionElementDetailed("exec", 		exec, 		"su", 	"Executes a file",  								NULL),
		cliActionElementDetailed("top", 		top, 		"", 	"Lists all tasks",  								NULL),
		cliActionElementDetailed("kill", 		kill, 		"u", 	"Kill a task using PID",  								NULL),
		cliMenuTerminator()
};

#endif
