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
		"Address of the first byte",
		"Address to stop (other bytes in this sector will not be erased)",
		NULL,
};

/**********************************************************
 * PRIVATE FUNCTIONS
 **********************************************************/

static void read(){

	uint32_t addr = cli_get_uint32_argument(0, NULL);
	uint32_t len = cli_get_uint32_argument(1, NULL);

	uint8_t* buffer = (uint8_t*)os_heap_alloc(len);
	ASSERT(buffer != NULL);

	uint32_t tickBef = HAL_GetTick();
	int32_t readB = flash_read(addr, buffer, len);
	uint32_t tickAft = HAL_GetTick();
	ASSERT(readB >= 0);

	PRINTLN("Read %ld bytes in %lu ms", readB, tickAft - tickBef);
	for(int i = 0; i < len; i++){
		PRINTLN("   [%03d] = 0x%02X", i, buffer[i]);
	}

	ASSERT(os_heap_free(buffer) == OS_ERR_OK);
}

static void write(){

	uint8_t buffer[200];
	uint32_t addr = cli_get_uint32_argument(0, NULL);

	uint32_t realSize = cli_get_buffer_argument(1, buffer, sizeof(buffer), NULL);
	uint32_t tickBef = HAL_GetTick();
	int32_t writeB = flash_write(addr, buffer, realSize);
	uint32_t tickAft = HAL_GetTick();
	ASSERT(writeB >= 0);

	PRINTLN("Write %ld bytes in %lu ms", writeB, tickAft - tickBef);
	for(int i = 0; i < realSize; i++){
		PRINTLN("   [%03d] = 0x%02X", i, buffer[i]);
	}
}

static void erase(){
	uint32_t addrB = cli_get_uint32_argument(0, NULL);
	uint32_t addrE = cli_get_uint32_argument(1, NULL);

	uint32_t tickBef = HAL_GetTick();
	int32_t erase = flash_erase(addrB, addrE);
	uint32_t tickAft = HAL_GetTick();
	ASSERT(erase >= 0);

	PRINTLN("erase %ld sectors in %lu ms", erase, tickAft - tickBef);
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
