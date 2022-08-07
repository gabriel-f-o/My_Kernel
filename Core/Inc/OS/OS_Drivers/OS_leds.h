/*
 * OS_leds.h
 *
 *  Created on: Aug 3, 2022
 *      Author: Gabriel
 */

#ifndef INC_OS_OS_DRIVERS_OS_LEDS_H_
#define INC_OS_OS_DRIVERS_OS_LEDS_H_

#include "stdbool.h"

/**********************************************
 * PUBLIC TYPES
 *********************************************/

/* Led enum (red and orange are reserved to OS usage)
 ---------------------------------------------------*/
typedef enum{
	OS_LEDS_BLUE,
	OS_LEDS_GREEN,
}os_leds_e;

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
void os_leds_set(os_leds_e led, bool state);


/***********************************************************************
 * OS LEDS get
 *
 * @brief This function get led state
 *
 * @param os_leds_e led : [in] Which led to get (green or blue)
 *
 * @return bool : state of the led (0 = off, 1 = on)
 **********************************************************************/
bool os_leds_get(os_leds_e led);


/***********************************************************************
 * OS LEDS toggle
 *
 * @brief This function toggles a led
 *
 * @param os_leds_e led : [in] Which led to toggle (green or blue)
 *
 **********************************************************************/
void os_leds_toggle(os_leds_e led);


#endif /* INC_OS_OS_DRIVERS_OS_LEDS_H_ */
