/*
 * common.h
 *
 *  Created on: Jul 9, 2022
 *      Author: Gabriel
 */

#ifndef INC_COMMON_H_
#define INC_COMMON_H_

#include "OS/OS_Core/OS.h"

#include "OS/OS_CLI/cli.h"
#include "OS/OS_CLI/cli_conf.h"
#include "OS/OS_CLI/cli_process.h"

#include "OS/OS_FS/lfs.h"
#include "OS/OS_FS/lfs_flash.h"
#include "OS/OS_FS/lfs_util.h"
#include "OS/OS_FS/xmodem.h"
#include "OS/OS_FS/OS_fs.h"

#include "OS/OS_Drivers/OS_flash.h"
#include "OS/OS_Drivers/OS_leds.h"

#include "OS/OS_SL/os_sl.h"

#include "main.h"

#ifndef PRINT_ENABLE
#define PRINT_ENABLE 1
#endif


#if defined(PRINT_ENABLE) && PRINT_ENABLE == 1
//#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define PRINTF(S, ...) 		printf(S, ## __VA_ARGS__)
#define PRINT(S, ...) 		PRINTF("[%s:%03d]:" S, __FILENAME__, __LINE__, ## __VA_ARGS__)
#define PRINTLN(S, ...) 	PRINT (S "\r\n", ## __VA_ARGS__)
#else
#define PRINTF(S, ...)
#define PRINT(S, ...)
#define PRINTLN(S, ...)
#endif

#ifndef ASSERT
#define ASSERT(x)     if(!(x)) PRINTLN("Error occured");
#endif

extern int __io_putchar(int ch);

#endif /* INC_COMMON_H_ */
