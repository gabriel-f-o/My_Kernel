/*
 * flash.c
 *
 *  Created on: Jul 9, 2022
 *      Author: Gabriel
 */


#include "common.h"

#define FLASH_MAX_ADDR		0x8100000UL
#define FLASH_BASE_ADDR		0x8000000UL
#define FLASH_TOTAL_SIZE	0x0100000UL

int32_t flash_write(uint32_t addr, uint8_t buffer[], size_t len){

	if(len == 0) return -2;
	if(buffer == NULL) return -2;
	if(addr < FLASH_BASE_ADDR) return -2;
	if(addr >= FLASH_MAX_ADDR) return -2;

	HAL_StatusTypeDef ret = FLASH_WaitForLastOperation(1000);
	ASSERT(ret == HAL_OK);
	if(ret != HAL_OK)
		return -1;

	ret = HAL_FLASH_Unlock();
	ASSERT(ret == HAL_OK);
	if(ret != HAL_OK)
		return -1;

	int32_t pos = 0;
	while(1){

		uint32_t writePermitted = 0;
		writePermitted = ( (addr + 1) <= FLASH_MAX_ADDR ) 				   ? 1 : writePermitted;
		writePermitted = ( (addr % 2) == 0 && addr + 2 <= FLASH_MAX_ADDR ) ? 2 : writePermitted;
		writePermitted = ( (addr % 4) == 0 && addr + 4 <= FLASH_MAX_ADDR ) ? 4 : writePermitted;

		uint32_t lenToWrite = writePermitted < len ? writePermitted : len;
		lenToWrite = lenToWrite == 3 ? 2 : lenToWrite;

		if(lenToWrite == 0 || len == 0) return pos;

		uint32_t writeFlag = FLASH_TYPEPROGRAM_BYTE;
			 	 writeFlag = lenToWrite == 2 ? FLASH_TYPEPROGRAM_HALFWORD : writeFlag;
			 	 writeFlag = lenToWrite == 4 ?     FLASH_TYPEPROGRAM_WORD : writeFlag;

		uint64_t data = 0;
		memcpy(&data, &buffer[pos], lenToWrite);

		ret = HAL_FLASH_Program(writeFlag, addr, data);
		ASSERT(ret == HAL_OK);
		if(ret != HAL_OK)
			return -1;

		pos += (int32_t)lenToWrite;
		addr += lenToWrite;
		len -= lenToWrite;

		HAL_StatusTypeDef ret = FLASH_WaitForLastOperation(1000);
		ASSERT(ret == HAL_OK);
		if(ret != HAL_OK)
			return pos;
	}

	ret = HAL_FLASH_Lock();
	ASSERT(ret == HAL_OK);
	return pos;
}


int32_t flash_read(uint32_t addr, uint8_t buffer[], size_t len){

	if(len == 0) return -2;
	if(buffer == NULL) return -2;
	if(addr < FLASH_BASE_ADDR) return -2;
	if(addr >= FLASH_MAX_ADDR) return -2;

	size_t readBytes = len;
	if(addr + len > FLASH_MAX_ADDR){
		readBytes -= addr + len - FLASH_MAX_ADDR;
	}

	memcpy(buffer, (void*)addr, readBytes);

	return (int32_t)readBytes;
}


int32_t flash_erase(uint32_t addrBeg, uint32_t addrEnd){

	if(addrBeg < FLASH_BASE_ADDR) return -2;
	if(addrBeg >= FLASH_MAX_ADDR) return -2;
	if(addrEnd < FLASH_BASE_ADDR) return -2;
	if(addrBeg >= addrEnd) return -2;

	HAL_StatusTypeDef ret = FLASH_WaitForLastOperation(1000);
	ASSERT(ret == HAL_OK);
	if(ret != HAL_OK)
		return -1;

	ret = HAL_FLASH_Unlock();
	ASSERT(ret == HAL_OK);
	if(ret != HAL_OK)
		return -1;

	uint32_t addrCur = addrBeg;
	int32_t sectNum = 0;
	while(addrCur < addrEnd && addrCur < FLASH_MAX_ADDR){

		uint32_t Sector = 0;
		sectNum++;
		if(0x08000000 <= addrCur && addrCur <= 0x08003FFF){
			Sector = 0;
			addrCur += 16*1024;
		}
		else if(0x08004000 <= addrCur && addrCur <= 0x08007FFF) {
			Sector = 1;
			addrCur += 16*1024;
		}
		else if(0x08008000 <= addrCur && addrCur <= 0x0800BFFF) {
			Sector =  2;
			addrCur += 16*1024;
		}
		else if(0x0800C000 <= addrCur && addrCur <= 0x0800FFFF) {
			Sector =  3;
			addrCur += 16*1024;
		}
		else if(0x08010000 <= addrCur && addrCur <= 0x0801FFFF) {
			Sector =  4;
			addrCur += 64*1024;
		}
		else if(0x08020000 <= addrCur && addrCur <= 0x0803FFFF) {
			Sector =  5;
			addrCur += 128*1024;
		}
		else if(0x08040000 <= addrCur && addrCur <= 0x0805FFFF) {
			Sector =  6;
			addrCur += 128*1024;
		}
		else if(0x08060000 <= addrCur && addrCur <= 0x0807FFFF) {
			Sector =  7;
			addrCur += 128*1024;
		}
		else if(0x08080000 <= addrCur && addrCur <= 0x0809FFFF) {
			Sector =  8;
			addrCur += 128*1024;
		}
		else if(0x080A0000 <= addrCur && addrCur <= 0x080BFFFF) {
			Sector =  9;
			addrCur += 128*1024;
		}
		else if(0x080C0000 <= addrCur && addrCur <= 0x080DFFFF) {
			Sector =  10;
			addrCur += 128*1024;
		}
		else if(0x080E0000 <= addrCur && addrCur <= 0x080FFFFF) {
			Sector =  11;
			addrCur += 128*1024;
		}


		FLASH_Erase_Sector(Sector, FLASH_VOLTAGE_RANGE_3);

		HAL_StatusTypeDef ret = FLASH_WaitForLastOperation(10000);
		ASSERT(ret == HAL_OK);
		if(ret != HAL_OK)
			return sectNum;
	}

	ret = HAL_FLASH_Lock();
	ASSERT(ret == HAL_OK);
	if(ret != HAL_OK)
		return -1;

	return sectNum;
}
