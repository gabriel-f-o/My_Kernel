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
extern cliElement_t cliTasks[];
extern cliElement_t cliLfs[];

/**********************************************************
 * GLOBAL VARIABLES
 **********************************************************/

cliElement_t cliMainMenu[] = {
		cliSubMenuElement("tasks", 		cliTasks, 		"Task interface"),
		cliSubMenuElement("flash", 		cliFlash, 		"Flash interface"),
		cliSubMenuElement("fs", 		cliLfs, 		"File system interface"),
		cliSubMenuElement("system", 	cliSystem,		"System options"),
		cliMenuTerminator()
};

#endif
