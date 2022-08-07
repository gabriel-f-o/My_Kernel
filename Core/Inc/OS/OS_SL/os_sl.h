/*
 * os_sl.h
 *
 *  Created on: Aug 1, 2022
 *      Author: Gabriel
 */

#ifndef INC_OS_OS_SL_OS_SL_H_
#define INC_OS_OS_SL_OS_SL_H_

#include "stdlib.h"
#include "stdio.h"
#include "stdint.h"

/**********************************************
 * DEFINES
 *********************************************/

#define OS_LINK_FN(n, fn)		{ .name = n, .fnPtr = fn }

/**********************************************
 * PUBLIC TYPES
 *********************************************/

/* Structure to link a function to a name
 ---------------------------------------------------*/
typedef struct{
	void*	fnPtr;	//Function reference
	char*	name;	//Name to be accessed from outside
} os_fn_link_table_el_t;

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
void* os_sl_translate(char* name);


/***********************************************************************
 * OS Link Table get size
 *
 * @brief This function gets the link table size
 *
 * @return size_t : size of the function link table
 *
 **********************************************************************/
size_t os_sl_linkTable_getSize();


#endif /* INC_OS_OS_SL_OS_SL_H_ */
