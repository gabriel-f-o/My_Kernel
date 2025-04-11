/*
 * cli_process.c
 *
 *  Created on: Jul 9, 2022
 *      Author: Gabriel
 */

#include "common.h"
#include "main.h"
#include "usart.h"

/**********************************************
 * PRIVATE VARIABLES
 *********************************************/

static char cli_char;
static char cliBuffer[128];

/**********************************************
 * CALLBACK FUNCTIONS
 *********************************************/

/***********************************************************************
 * CLI Receive char (IRQ)
 *
 * @brief This function receives a character and puts in the cli buffer
 *
 **********************************************************************/
void cli_rcv_char_cb_irq(){
	cli_insert_char(cliBuffer, sizeof(cliBuffer), cli_char);
	HAL_UART_Transmit(&USART_CLI, (uint8_t*)&cli_char, 1, 10);
	HAL_UART_Receive_IT(&USART_CLI, (uint8_t*)&cli_char, 1);
}

/**********************************************
 * PUBLIC FUNCTIONS
 *********************************************/

/***********************************************************************
 * CLI INIT
 *
 * @brief This function initializes the CLI
 *
 **********************************************************************/
void cli_init(){
	memset(cliBuffer, 0, sizeof(cliBuffer));
	HAL_UART_Abort(&USART_CLI);
	HAL_UART_AbortReceive_IT(&USART_CLI);
	__HAL_UART_FLUSH_DRREGISTER(&USART_CLI);
	HAL_UART_Receive_IT(&USART_CLI, (uint8_t*)&cli_char, 1);
}

/***********************************************************************
 * CLI Process
 *
 * @brief This function treats the command sent by CLI
 *
 **********************************************************************/
void cli_process(){
	cli_treat_command(cliBuffer, sizeof(cliBuffer));
}
