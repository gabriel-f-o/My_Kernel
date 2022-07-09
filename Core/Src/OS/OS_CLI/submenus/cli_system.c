/*
 * cli_system.c
 *
 *  Created on: Jul 9, 2022
 *      Author: Gabriel
 */


#include "common.h"
#include "main.h"

#if ( defined(CLI_EN) && (CLI_EN == 1) )

/**********************************************************
 * PRIVATE FUNCTIONS
 **********************************************************/

static void reset(){
	NVIC_SystemReset();
}

/**********************************************************
 * GLOBAL VARIABLES
 **********************************************************/

cliElement_t cliSystem[] = {
		cliActionElementDetailed("reset", 	reset, 	"", 	"Reset device",  		NULL),
		cliMenuTerminator()
};

#endif
