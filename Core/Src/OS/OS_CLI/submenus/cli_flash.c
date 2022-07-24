/*
 * cli_flash.c
 *
 *  Created on: Jul 9, 2022
 *      Author: Gabriel
 */


#include "common.h"
#include "main.h"

#if ( defined(CLI_EN) && (CLI_EN == 1) )

/**********************************************************
 * PRIVATE VARIABLES
 **********************************************************/

static char const * const readWriteArgs[] = {
		"Address of the first byte",
		"Total bytes to read / write",
		NULL,
};

static char const * const eraseArgs[] = {
		"Address of the first byte. Must be aligned with a sector",
		"Number of sectors to erase",
		NULL,
};

/**********************************************************
 * PRIVATE FUNCTIONS
 **********************************************************/

static void read(){

	/* Get arguments
	 --------------------------------------------------*/
	uint32_t addr = cli_get_uint32_argument(0, NULL);
	uint32_t len = cli_get_uint32_argument(1, NULL);

	/* Allocate buffer
	 --------------------------------------------------*/
	uint8_t* buffer = (uint8_t*)os_heap_alloc(len);
	ASSERT(buffer != NULL);

	/* Read from flash
	 --------------------------------------------------*/
	int32_t readB = os_flash_read(addr, buffer, len);
	ASSERT(readB >= 0);

	/* Show values
	 --------------------------------------------------*/
	PRINTLN("Read %ld bytes", readB);
	for(int i = 0; i < readB; i++){
		PRINTLN("   [%03d] = 0x%02X", i, buffer[i]);
	}

	ASSERT(os_heap_free(buffer) == OS_ERR_OK);
}

static void write(){

	/* Get arguments
	 --------------------------------------------------*/
	uint8_t buffer[200];
	uint32_t addr = cli_get_uint32_argument(0, NULL);
	uint32_t realSize = cli_get_buffer_argument(1, buffer, sizeof(buffer), NULL);

	/* Write into flash
	 --------------------------------------------------*/
	int32_t writeB = os_flash_write(addr, buffer, realSize);
	ASSERT(writeB >= 0);

	/* Show written values
	 --------------------------------------------------*/
	PRINTLN("Write %ld bytes", writeB);
	for(int i = 0; i < realSize; i++){
		PRINTLN("   [%03d] = 0x%02X", i, buffer[i]);
	}
}

static void erase(){

	/* Get arguments
	 --------------------------------------------------*/
	uint32_t addrB = cli_get_uint32_argument(0, NULL);
	uint32_t num = cli_get_uint32_argument(1, NULL);

	/* Erase sectors
	 --------------------------------------------------*/
	int32_t erase = os_flash_erase(addrB, num);
	ASSERT(erase >= 0);

	/* Feedback
	 --------------------------------------------------*/
	PRINTLN("erase %ld sectors", erase);
}

/**********************************************************
 * GLOBAL VARIABLES
 **********************************************************/

cliElement_t cliFlash[] = {
		cliActionElementDetailed("read", 	read, 	"uu", 	"read bytes from flash",  		readWriteArgs),
		cliActionElementDetailed("write", 	write, 	"ub", 	"write bytes to flash",  		readWriteArgs),
		cliActionElementDetailed("erase", 	erase, 	"uu", 	"write bytes to flash",  		eraseArgs),
		cliMenuTerminator()
};

#endif
