/*
 * cli.c
 *
 *  Created on: Feb 8, 2021
 *      Author: INGE2
 */

#include "common.h"
#include "main.h"

#if ( defined(CLI_EN) && (CLI_EN == 1) )

extern cliElement_t cliSystem[];
extern cliElement_t cliFlash[];

/**********************************************************
 * GLOBAL VARIABLES
 **********************************************************/

cliElement_t cliMainMenu[] = {
		cliSubMenuElement("flash", 		cliFlash, 		"Flash interface"),
		cliSubMenuElement("system", 	cliSystem,		"System options"),
		cliMenuTerminator()
};

#endif
