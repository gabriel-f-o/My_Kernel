/*
 * os_sl.c
 *
 *  Created on: Jul 31, 2022
 *      Author: Gabriel
 */

#include "OS/OS_Core/OS_Common.h"
#include "common.h"

/**********************************************
 * EXTERNAL FUNCTIONS
 *********************************************/

extern const os_fn_link_table_el_t os_link_table[];

/**********************************************
 * PRIVATE FUNCTIONS
 *********************************************/

static void*  __used pOs_sl_translate = &os_sl_translate;

/**********************************************
 * PUBLIC FUNCTIONS
 *********************************************/

/***********************************************************************
 * OS shared library translate
 *
 * @brief This function searches a linked function with its name and returns its reference
 *
 * @return void* : the reference to the function or NULL if it does not exist
 **********************************************************************/
void* os_sl_translate(char* name){

	/* Search for function
	 ------------------------------------------------------*/
	for(size_t i = 0; i < os_sl_linkTable_getSize(); i++){

		/* If the string maches, return the function's reference
		 ------------------------------------------------------*/
		if(strcmp(os_link_table[i].name, name) == 0){
			return os_link_table[i].fnPtr;
		}
	}

	/* Did not find function. Return NULL
	 ------------------------------------------------------*/
	return NULL;
}
