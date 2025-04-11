/*
 * cli_lfs.c
 *
 *  Created on: Jul 10, 2022
 *      Author: Gabriel
 */


#include "common.h"
#include "main.h"

#if ( defined(CLI_EN) && (CLI_EN == 1) )

/**********************************************************
 * PUBLIC VARIABLES
 **********************************************************/

char* cur_dir = ".";

/**********************************************************
 * PRIVATE VARIABLES
 **********************************************************/

static lfs_file_t lfs_file;

/**********************************************************
 * PRIVATE FUNCTIONS
 **********************************************************/

static void open(){

	/* Get arguments
	 --------------------------------------------------*/
	char name[50];
	cli_get_string_argument(0, (uint8_t*)name, sizeof(name), NULL);

	/* Open file
	 --------------------------------------------------*/
	int err = lfs_file_open(&lfs, &lfs_file, name, LFS_O_CREAT | LFS_O_RDWR);
	if(err < 0)
		PRINTLN("lfs open '%s' error %d", name, err);
	else{
		PRINTLN("Open '%s' OK", name);
	}
}

static void bRead(){

	/* Get arguments
	 --------------------------------------------------*/
	uint32_t nBytes = cli_get_uint32_argument(0, NULL);

	/* Allocate buffer
	 --------------------------------------------------*/
	uint8_t* buffer = os_heap_alloc(nBytes);

	/* read from file
	 --------------------------------------------------*/
	lfs_ssize_t err = lfs_file_read(&lfs, &lfs_file, buffer, nBytes);
	if(err < 0)
		PRINTLN("lfs read error %ld", err);
	else{
		/* Show values
		 --------------------------------------------------*/
		PRINTLN("Read %ld bytes", err);
		for(int i = 0; i < nBytes; i++){
			PRINTLN("   [%03d] = 0x%02X", i, buffer[i]);
		}
	}


	/* free buffer
	 --------------------------------------------------*/
	os_heap_free(buffer);
}

static void read(){

	/* Get arguments
	 --------------------------------------------------*/
	uint32_t nBytes = cli_get_uint32_argument(0, NULL);

	/* Allocate buffer
	 --------------------------------------------------*/
	uint8_t* buffer = os_heap_alloc(nBytes+1);
	buffer[nBytes] = '\0';

	/* read from file
	 --------------------------------------------------*/
	lfs_ssize_t err = lfs_file_read(&lfs, &lfs_file, buffer, nBytes);
	if(err < 0)
		PRINTLN("lfs read error %ld", err);
	else{

		buffer[err] = '\0';
		/* Show values
		 --------------------------------------------------*/
		PRINTLN("Read %ld bytes", err);
		PRINTLN("File : %s", buffer);
	}

	/* free buffer
	 --------------------------------------------------*/
	os_heap_free(buffer);
}

static void write(){

	/* Get arguments
	 --------------------------------------------------*/
	uint8_t buffer[200];
	uint32_t nBytes = cli_get_string_argument(0, buffer, sizeof(buffer), NULL);

	/* write into file
	 --------------------------------------------------*/
	lfs_ssize_t err = lfs_file_write(&lfs, &lfs_file, buffer, nBytes);
	if(err < 0)
		PRINTLN("lfs write error %ld", err);
	else{
		PRINTLN("write OK");
	}
}

static void seek(){

	/* Get arguments
	 --------------------------------------------------*/
	lfs_soff_t pos = (lfs_soff_t)cli_get_int32_argument(0, NULL);

	/* write into file
	 --------------------------------------------------*/
	lfs_soff_t err = lfs_file_seek(&lfs, &lfs_file, pos, LFS_SEEK_SET);
	if(err < 0)
		PRINTLN("lfs seek error %ld", err);
	else{
		PRINTLN("seek OK");
	}
}

static void close(){

	/* Close current file
	 --------------------------------------------------*/
	int err = lfs_file_close(&lfs, &lfs_file);
	if(err < 0)
		PRINTLN("lfs close error %d", err);
	else{
		PRINTLN("Close OK");
	}
}

static void delete(){

	/* Get arguments
	 --------------------------------------------------*/
	char name[50];
	cli_get_string_argument(0, (uint8_t*)name, sizeof(name), NULL);

	/* Remove file / dir
	 --------------------------------------------------*/
	int err = lfs_remove(&lfs, name);
	if(err < 0)
		PRINTLN("lfs remove '%s' error %d", name, err);
	else{
		PRINTLN("Remove '%s' OK", name);
	}
}

static void ls(){

	/* Open current directory
	 --------------------------------------------------*/
	lfs_dir_t dir;
	int err = lfs_dir_open(&lfs, &dir, cur_dir);
	if(err < 0){
		PRINTLN("Dir open err %d", err);
		return;
	}

	/* For each file in the directory
	 --------------------------------------------------*/
	struct lfs_info info;
	while(1){

		/* Get its info
		 --------------------------------------------------*/
		err = lfs_dir_read(&lfs, &dir, &info);
		if(err < 0){
			PRINTLN("Dir open err %d", err);
			break;
		}

		/* If it has no name, stop
		 --------------------------------------------------*/
		if(strlen(info.name) == 0)
			break;

		/* Print filename and size
		 --------------------------------------------------*/
		PRINTLN("[%s] %s, size = %lu", info.type == LFS_TYPE_REG ? "File" : "Dir", info.name, info.size);
	};

	/* Close directory
	 --------------------------------------------------*/
	err = lfs_dir_close(&lfs, &dir);
	if(err < 0){
		PRINTLN("Dir close err %d", err);
	}
}

static void xmodem(){

	/* Do not execute twice
	 --------------------------------------------------*/
	if(xModem_getState() == 1) return;

	/* Get arguments
	 --------------------------------------------------*/
	char name[50];
	size_t len = cli_get_string_argument(0, (uint8_t*)name, sizeof(name), NULL);

	/* Store name in heap
	 --------------------------------------------------*/
	char* arg = os_heap_alloc(len + 1);
	snprintf(arg, len + 1, "%s", name);

	/* Open file
	 --------------------------------------------------*/
	os_handle_t h;
	os_task_create(&h, "xmodem", (void*)xModem_rcv, OS_TASK_MODE_DELETE, 11, 5 * OS_DEFAULT_STACK_SIZE, (int)arg, NULL);
}

/**********************************************************
 * GLOBAL VARIABLES
 **********************************************************/

cliElement_t cliLfs[] = {
		cliActionElementDetailed("open", 		open, 		"s", 	"Open a file by name",  							NULL),
		cliActionElementDetailed("bRead", 		bRead, 		"u", 	"Binary Read from the opened file",  				NULL),
		cliActionElementDetailed("read", 		read, 		"u", 	"String Read from the opened file",  				NULL),
		cliActionElementDetailed("write", 		write, 		"s", 	"Write into the opened file",  						NULL),
		cliActionElementDetailed("seek", 		seek, 		"i", 	"Seek an absolute position in the opened file",  	NULL),
		cliActionElementDetailed("del", 		delete,		"s", 	"Delete a file by name",  							NULL),
		cliActionElementDetailed("close", 		close, 		"", 	"Closes the current file",  						NULL),
		cliActionElementDetailed("ls", 			ls, 		"", 	"Prints all files in a directory",  				NULL),
		cliActionElementDetailed("xmodem", 		xmodem, 	"s", 	"Receives a File via xmodem",  						NULL),
		cliMenuTerminator()
};

#endif
