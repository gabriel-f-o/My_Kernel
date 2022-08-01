/*
 * os_sl.c
 *
 *  Created on: Jul 31, 2022
 *      Author: Gabriel
 */

#include "OS/OS_Core/OS_Common.h"
#include "common.h"

#define OS_JEBA(n, fn)		{ .name = n, .fnPtr = fn }

void* os_sl_translate(char* );

void* __section(".slPtr") __used pOs_sl_translate = os_sl_translate;

typedef struct{
	void*	fnPtr;
	char*	name;
} os_jeba_t;


static const os_jeba_t teste[] = {
		OS_JEBA("__io_putchar", __io_putchar),
};

void* os_sl_translate(char* name){
	for(int i = 0; i < sizeof(teste)/sizeof(*teste); i++){
		if(strcmp(teste[i].name, name) == 0){
			return teste[i].fnPtr;
		}
	}

	return NULL;
}
