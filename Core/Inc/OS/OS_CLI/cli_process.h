/*
 * cli_process.h
 *
 *  Created on: Jul 9, 2022
 *      Author: Gabriel
 */

#ifndef INC_OS_OS_CLI_CLI_PROCESS_H_
#define INC_OS_OS_CLI_CLI_PROCESS_H_

#include "OS/OS.h"

void cli_rcv_char_cb_irq();
void cli_init();
void cli_process();


#endif /* INC_OS_OS_CLI_CLI_PROCESS_H_ */
