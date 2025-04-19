/*
 * OS_Process.c
 *
 *  Created on: Apr 15, 2025
 *      Author: Gabriel
 */

#include "common.h"
#include "OS/OS_Core/OS.h"
#include "OS/OS_Core/OS_Internal.h"
#include "OS/OS_Core/OS_Process.h"

/**********************************************
 * PUBLIC VARIABLES
 *********************************************/

os_list_head_t os_process_list;				//Head of process list

/**********************************************
 * OS PRIVATE FUNCTIONS
 *********************************************/

//////////////////////////////////////////////// ELF LOADER //////////////////////////////////////////////////


/***********************************************************************
 * OS ELF load header
 *
 * @brief This function loads the elf file header
 *
 * @param os_elf_header_t* header 	: [out] header information
 * @param lfs_file_t* lfs_file		: [ in] File pointer to the elf file
 *
 * @return os_err_e : <0 if error
 **********************************************************************/
static os_err_e os_elf_loadHeader(os_elf_header_t* header, lfs_file_t* lfs_file){

	/* Rewind file to the beginning
	 ------------------------------------------------------*/
	int err = lfs_file_seek(&lfs, lfs_file, 0, LFS_SEEK_SET);
	if(err < 0){
		return OS_ERR_FS;
	}

	/* Read the header
	 ------------------------------------------------------*/
	err = lfs_file_read(&lfs, lfs_file, header, sizeof(*header));
	if(err < 0){
		return OS_ERR_FS;
	}

	/* Check magic number
	 ------------------------------------------------------*/
	if(header->e_ident.magic[0] != 0x7F || header->e_ident.magic[1] != 'E' || header->e_ident.magic[2] != 'L' || header->e_ident.magic[3] != 'F'){
		return OS_ERR_INVALID;
	}

	/* Check endianness, bit depth, and version
	 ------------------------------------------------------*/
	if(header->e_ident.class != 1 || header->e_ident.data != 1 || header->e_ident.version != 1){
		return OS_ERR_INVALID;
	}

	/* Check version and that its made for ARM
	 ------------------------------------------------------*/
	if(header->e_machine != 40 || header->e_version != 1){
		return OS_ERR_INVALID;
	}

	/* Return OK
	 ------------------------------------------------------*/
	return OS_ERR_OK;
}


/***********************************************************************
 * OS ELF load segments
 *
 * @brief This function loads all segments into RAM
 *
 * @param os_process_t* p 			: [ in] Process reference
 * @param lfs_file_t* lfs_file		: [ in] File pointer to the elf file
 *
 * @return os_err_e : <0 if error
 **********************************************************************/
static os_err_e os_elf_loadSegments(os_process_t* p, lfs_file_t* lfs_file){

	/* First, calculate how much RAM we need
	 ------------------------------------------------------*/
	uint32_t memToAlloc = 0;

	/* For each segment
	 ------------------------------------------------------*/
	for(uint32_t i = 0; i < p->elf_H.e_phnum; i++){

		/* read program header data
		 ------------------------------------------------------*/
		os_elf_programHeader_t data;
		int err  = lfs_file_seek(&lfs, lfs_file, (lfs_soff_t)(p->elf_H.e_phoff + i * p->elf_H.e_phentsize), LFS_SEEK_SET); //Seek to the program header position
		    err |= lfs_file_read(&lfs, lfs_file, &data, sizeof(data));
		if(err < 0){
			return OS_ERR_FS;
		}

		/* Check that it is a LOAD segment
		 ------------------------------------------------------*/
		if(data.p_type != 1)
			continue;

		/* align segment block as 8 byte
		 ------------------------------------------------------*/
		memToAlloc += (data.p_memsz + 7) & (~0x7UL);
	}

	/* Allocate all segments to make the free easier
	 ------------------------------------------------------*/
	p->segments = (uint8_t*) os_heap_alloc(memToAlloc);
	if(p->segments == NULL)
		return OS_ERR_INSUFFICIENT_HEAP;

	/* Initialize segments to 0 and Load into memory
	 ------------------------------------------------------*/
	size_t pos = 0;
	memset(p->segments, 0, memToAlloc);

	/* For each segment
	 ------------------------------------------------------*/
	for(uint32_t i = 0; i < p->elf_H.e_phnum; i++){

		/* Read program header data
		 ------------------------------------------------------*/
		os_elf_programHeader_t data;
		int err  = lfs_file_seek(&lfs, lfs_file, (lfs_soff_t)(p->elf_H.e_phoff + i * p->elf_H.e_phentsize), LFS_SEEK_SET); //Seek to the program header position
		 	err |= lfs_file_read(&lfs, lfs_file, &data, sizeof(data));
		if(err < 0){
			os_heap_free(p->segments);
			p->segments = NULL;
			return OS_ERR_FS;
		}

		/* Check it is LOAD segment
		 ------------------------------------------------------*/
		if(data.p_type != 1)
			continue;

		/* Read the entire segment into the heap
		 ------------------------------------------------------*/
		err  = lfs_file_seek(&lfs, lfs_file, (lfs_soff_t)data.p_offset, LFS_SEEK_SET); //Seek to the actual data
		err |= lfs_file_read(&lfs, lfs_file, &p->segments[pos], data.p_filesz);
		if(err < 0){
			os_heap_free(p->segments);
			p->segments = NULL;
			return OS_ERR_FS;
		}

		/* Calculate segment size
		 ------------------------------------------------------*/
		size_t segmentSize = (data.p_memsz + 7) & (~0x7UL);

		/* increment buffer position
		 ------------------------------------------------------*/
		pos += segmentSize;
	}

	return OS_ERR_OK;
}


/***********************************************************************
 * OS ELF Adjust Memory references
 *
 * @brief This function adjusts the Global Offset Table of the program. When compiled with Position Independent Code (-fPIC), the code gets all globals using the GOT
 * This GOT must be corrected to the actual address we are loading. This is also the case for some other sections
 *
 * @param os_process_t* p 			: [ in] Process reference
 * @param lfs_file_t* lfs_file		: [ in] File pointer to the elf file
 *
 * @return os_elf_prog_t : information needed to run an executable
 **********************************************************************/
static os_err_e os_elf_adjustMem(os_process_t* p, lfs_file_t* lfs_file){

	/* Load section header that contains the names of the sections
	 ------------------------------------------------------*/
	os_elf_sectionHeader_t names;
	int err  = lfs_file_seek(&lfs, lfs_file, (lfs_soff_t)(p->elf_H.e_shoff + (uint32_t)(p->elf_H.e_shstrndx * p->elf_H.e_shentsize) ), LFS_SEEK_SET); //Seek to the index of the section header that contains all names
		err |= lfs_file_read(&lfs, lfs_file, &names, sizeof(names));
	if(err < 0){
		return OS_ERR_FS;
	}

	/* For each section
	 ------------------------------------------------------*/
	for(uint32_t i = 0; i < p->elf_H.e_shnum; i++){

		/* read section header information
		 ------------------------------------------------------*/
		os_elf_sectionHeader_t data;
		int err  = lfs_file_seek(&lfs, lfs_file, (lfs_soff_t)(p->elf_H.e_shoff + i * p->elf_H.e_shentsize), LFS_SEEK_SET); //Seek to the section header
			err |= lfs_file_read(&lfs, lfs_file, &data, sizeof(data));
		if(err < 0){
			return OS_ERR_FS;
		}

		/* Get the name of the current section
		 ------------------------------------------------------*/
		char sect_name[32];
		memset(sect_name, 0, sizeof(sect_name));

		err  = lfs_file_seek(&lfs, lfs_file, (lfs_soff_t)(data.sh_name + names.sh_offset), LFS_SEEK_SET); //Seek to the string position in the file
		err |= lfs_file_read(&lfs, lfs_file, sect_name, sizeof(sect_name));
		if(err < 0){
			return OS_ERR_FS;
		}

		/* These sections need correction
		 ------------------------------------------------------*/
		if(strcmp(".got", sect_name) != 0 && strcmp(".preinit_array", sect_name) != 0 && strcmp(".init_array", sect_name) != 0 && strcmp(".fini_array", sect_name) != 0)
			continue;

		uint32_t* pMem = (uint32_t*) &p->segments[data.sh_addr];
		for(int j = 0; j < (int)data.sh_size; j += (int) sizeof(uint32_t)){ //Move in increments of 4 bytes
			pMem[j/4] = (uint32_t)p->segments + pMem[j/4];
		}

		if(strcmp(".got", sect_name) == 0){
			p->gotBaseAddr = (uint32_t)p->segments + data.sh_addr;
		}
	}

	/* Finally, calculate the entry point
	 ------------------------------------------------------*/
	uint32_t entry = p->elf_H.e_entry;
	entry += (uint32_t)p->segments;
	entry |= 0x01;

	p->entry_fn = (void*)entry;
	return OS_ERR_OK;
}


/**********************************************
 * OS PUBLIC FUNCTIONS
 *********************************************/

/***********************************************************************
 * OS Process get by PID
 *
 * @brief This function searches for a process with a giben PID
 *
 * @param uint16_t pid : [in] PID to search
 *
 * @return os_process_t* : reference to found process
 *
 **********************************************************************/
os_process_t* os_process_getByPID(uint16_t pid){
	if(pid == 0) return NULL;

	os_list_cell_t* it = os_process_list.head.next;
	while(it != NULL){
		if( ((os_process_t*)it->element)->PID == pid )
			return it->element;

		it = it->next;
	}

	return NULL;
}


/***********************************************************************
 * OS Create process
 *
 * @brief This function creates a process using its ELF file
 *
 * @param char* file   : [in] File's name
 * @param void* argc   : [in] Argument number to be passed to the task
 * @param char* argv[] : [in] Array of strings to be passed to the task
 *
 * @return os_err_e : An error code (0 = OK)
 *
 **********************************************************************/
os_err_e os_process_create(char* file, int argc, char* argv[]){

	/* Allocate process
	 --------------------------------------------------*/
	os_err_e ret = OS_ERR_OK;
	os_scheduler_state_e sch = os_scheduler_state_get();

	os_process_t* new_proc = (os_process_t*)os_heap_alloc(sizeof(os_process_t));
	if(new_proc == NULL){
		ret = OS_ERR_INSUFFICIENT_HEAP;
		goto exit;
	}

	/* Init thread list
	 --------------------------------------------------*/
	new_proc->thread_list = os_list_init();
	if(new_proc->thread_list == NULL){
		ret = OS_ERR_INSUFFICIENT_HEAP;
		goto exit;
	}

	/* Create a unique PID
	 ------------------------------------------------------*/
	uint16_t pid = 0;
	uint32_t attempts = 0;
	while(1){

		/* Generate PID using the tick
		 ------------------------------------------------------*/
		uint32_t ms = os_getMsTick() + attempts;
		pid = (uint16_t)( (ms & 0xFF) ^ ((ms >> 16) & 0xFF) );

		/* Check if PID exists
		 ------------------------------------------------------*/
		if(os_process_getByPID(pid) == NULL && pid != 0){
			break;
		}

		attempts++;
	}

	new_proc->PID = pid;

	/* Generate and copy name
	 ------------------------------------------------------*/
	uint32_t len = (uint32_t)snprintf(NULL, 0, "%s", file);
	new_proc->p_name = (char*)os_heap_alloc(len + 1);
	if(new_proc->p_name == NULL){
		ret = OS_ERR_INSUFFICIENT_HEAP;
		goto exit;
	}

	snprintf(new_proc->p_name, len + 1, "%s", file);

	/* Open file
	 --------------------------------------------------*/
	lfs_file_t lfs_file;
	int err = lfs_file_open(&lfs, &lfs_file, file, LFS_O_RDONLY);
	if(err < 0){
		PRINTLN("Open Error");
		ret = OS_ERR_FS;
		goto exit;
	}

	/* Load header information
	 --------------------------------------------------*/
	ret = os_elf_loadHeader(&new_proc->elf_H, &lfs_file);
	if(ret != OS_ERR_OK) {
		PRINTLN("Error loading header");
		goto exit_file;
	}

	/* Load segments information
	 --------------------------------------------------*/
	ret = os_elf_loadSegments(new_proc, &lfs_file);
	if(ret != OS_ERR_OK) {
		PRINTLN("Error loading data");
		goto exit_file;
	}

	/* Fix memory references
	 --------------------------------------------------*/
	ret = os_elf_adjustMem(new_proc, &lfs_file);
	if(ret != OS_ERR_OK) {
		PRINTLN("Error loading GOT");
		goto exit_file;
	}

	/* Stop scheduler to finish loading
	 ------------------------------------------------------*/
	os_scheduler_stop();

	/* Create main thread
	 ------------------------------------------------------*/
	os_handle_t t;
	ret = os_task_createProc(&t, NULL, new_proc->entry_fn, new_proc, OS_TASK_MODE_DELETE, 40, OS_DEFAULT_STACK_SIZE, argc, argv, new_proc->gotBaseAddr);
	if(ret != OS_ERR_OK) {
		PRINTLN("Error creating main task");
		goto exit_file;
	}

	/* Add to process thread list
	 ------------------------------------------------------*/
	ret = os_list_add(new_proc->thread_list, t, OS_LIST_LAST);
	if(ret != OS_ERR_OK) {
		PRINTLN("Error loading GOT");
		goto exit_task;
	}

	/* Add process to process list
	 ------------------------------------------------------*/
	ret = os_list_add(&os_process_list, new_proc, OS_LIST_LAST);
	if(ret != OS_ERR_OK) {
		PRINTLN("Error adding to process list");
		goto exit_thread_list;
	}

	/* Close file
	 ------------------------------------------------------*/
	if(lfs_file_close(&lfs, &lfs_file) < 0){
		PRINTLN("Close Error");
	}

	sch == OS_SCHEDULER_STOP ? os_scheduler_stop() : os_scheduler_start();

	return OS_ERR_OK;

	/* Cleanup in case of error
	 ------------------------------------------------------*/
exit_thread_list:

	if(os_list_remove(new_proc->thread_list, t) != OS_ERR_OK) {
		PRINTLN("Error cleaning thread list");
	}

exit_task:

	if(os_task_delete(t) != OS_ERR_OK) {
		PRINTLN("Error cleaning thread");
	}

exit_file:

	if(lfs_file_close(&lfs, &lfs_file) < 0){
		PRINTLN("Close Error");
	}

exit:

	if(new_proc->thread_list != NULL)
		os_list_clear(new_proc->thread_list);

	if(new_proc->p_name != NULL)
		os_heap_free(new_proc->p_name);

	if(new_proc->segments != NULL)
		os_heap_free(new_proc->segments);

	if(new_proc != NULL)
		os_heap_free(new_proc);

	sch == OS_SCHEDULER_STOP ? os_scheduler_stop() : os_scheduler_start();

	return ret;
}


/***********************************************************************
 * Kill a process
 *
 * @brief Kill a process by releasing its
 *
 * @param uint16_t pid : [in] PID
 *
 * @return os_err_e : An error code (0 = OK)
 *
 **********************************************************************/
os_err_e os_process_kill(os_process_t* proc){

	os_err_e ret = os_list_remove(&os_process_list, proc);
	if(ret != OS_ERR_OK)
		return ret;

	os_list_cell_t* it = ((os_list_head_t*)proc->thread_list)->head.next;
	while(it != NULL){
		ret = os_task_delete(it->element);
		if(ret != OS_ERR_OK)
			return ret;

		it = it->next;
	}

	os_list_clear(proc->thread_list);

	os_heap_free(proc->segments);
	os_heap_free(proc->p_name);
	os_heap_free(proc);

	return OS_ERR_OK;
}
