/*
 * os_leds.c
 *
 *  Created on: Aug 3, 2022
 *      Author: Gabriel
 */

#include "OS/OS_Drivers/OS_leds.h"
#include "main.h"

/**********************************************
 * PUBLIC FUNCTIONS
 *********************************************/

/***********************************************************************
 * OS LEDS set
 *
 * @brief This function set led state
 *
 * @param os_leds_e led : [in] Which led to set (green or blue)
 * @param bool state	: [in] State (0 = off, 1 = on)
 *
 **********************************************************************/
void os_leds_set(os_leds_e led, bool state){

	/* Sed led state
	 ---------------------------------------------------*/
	switch(led){
		case OS_LEDS_GREEN : HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, 	state); break;
		case OS_LEDS_BLUE  : HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, 	LED_BLUE_Pin, 	state); break;
		//case OS_LEDS_ORANGE : HAL_GPIO_WritePin(LED_ORANGE_GPIO_Port, LED_ORANGE_Pin, state); break;
		//case OS_LEDS_RED : HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, state); break;
		default : break;
	}
}


/***********************************************************************
 * OS LEDS get
 *
 * @brief This function get led state
 *
 * @param os_leds_e led : [in] Which led to get (green or blue)
 *
 * @return bool : state of the led (0 = off, 1 = on)
 **********************************************************************/
bool os_leds_get(os_leds_e led){

	/* Get state
	 ---------------------------------------------------*/
	GPIO_PinState state = GPIO_PIN_RESET;
	switch(led){
		case OS_LEDS_GREEN : state = HAL_GPIO_ReadPin(LED_GREEN_GPIO_Port, LED_GREEN_Pin); break;
		case OS_LEDS_BLUE  : state = HAL_GPIO_ReadPin(LED_BLUE_GPIO_Port,  LED_BLUE_Pin ); break;
		//case OS_LEDS_ORANGE : HAL_GPIO_ReadPin(LED_ORANGE_GPIO_Port, LED_ORANGE_Pin); break;
		//case OS_LEDS_RED : HAL_GPIO_ReadPin(LED_RED_GPIO_Port, LED_RED_Pin); break;
		default : break;
	}

	return state;
}


/***********************************************************************
 * OS LEDS toggle
 *
 * @brief This function toggles a led
 *
 * @param os_leds_e led : [in] Which led to toggle (green or blue)
 *
 **********************************************************************/
void os_leds_toggle(os_leds_e led){

	/* Toggle led
	 ---------------------------------------------------*/
	switch(led){
		case OS_LEDS_GREEN : HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin); break;
		case OS_LEDS_BLUE  : HAL_GPIO_TogglePin(LED_BLUE_GPIO_Port,  LED_BLUE_Pin); break;
		//case OS_LEDS_ORANGE : HAL_GPIO_WritePin(LED_ORANGE_GPIO_Port, LED_ORANGE_Pin, state); break;
		//case OS_LEDS_RED : HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, state); break;
		default : break;
	}
}
