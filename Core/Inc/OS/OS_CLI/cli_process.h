/*
 * cli_process.h
 *
 *  Created on: Jul 9, 2022
 *      Author: Gabriel
 */

#ifndef INC_OS_OS_CLI_CLI_PROCESS_H_
#define INC_OS_OS_CLI_CLI_PROCESS_H_

/**********************************************
 * PUBLIC FUNCTIONS
 * ********************************************/

/***********************************************************************
 * CLI Receive char (IRQ)
 *
 * @brief This function receives a character and puts in the cli buffer
 *
 **********************************************************************/
void cli_rcv_char_cb_irq();

/**********************************************
 * PUBLIC FUNCTIONS
 *********************************************/

/***********************************************************************
 * CLI INIT
 *
 * @brief This function initializes the CLI
 *
 **********************************************************************/
void cli_init(void);

/***********************************************************************
 * CLI Process
 *
 * @brief This function treats the command sent by CLI
 *
 **********************************************************************/
void cli_process(void);

#endif /* INC_OS_OS_CLI_CLI_PROCESS_H_ */
