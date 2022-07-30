/*
 * xmodem.c
 *
 *  Created on: Jul 16, 2022
 *      Author: Gabriel
 */

#include "common.h"
#include "usart.h"

/**********************************************************
 * DEFINES
 **********************************************************/

#define 	XMODEM_CMD_SOH   	(0x01)	//Start of Header.
#define 	XMODEM_CMD_EOT   	(0x04)	//End of Transmission.
#define 	XMODEM_CMD_ACK   	(0x06)	//Acknowledge.
#define 	XMODEM_CMD_NAK   	(0x15)	//Not Acknowledge.
#define 	XMODEM_CMD_CAN   	(0x18)	//Cancel.
#define 	XMODEM_CMD_CTRL_C   (0x03)	//Ctrl+C.
#define 	XMODEM_CMD_C   		(0x43)
#define 	XMODEM_DATA_SIZE    128

#define 	XMODEM_START_TIMEOUT 30 // timeout for communication start in seconds

/**********************************************************
 * PRIVATE TYPES
 **********************************************************/

typedef enum {
	XMODEM_STATE_WAIT_TO_START,
	XMODEM_STATE_IN_PROGRESS,
	XMODEM_STATE_COMPLETED,
	XMODEM_STATE_FAILED
} XmodemState_t;

typedef struct{
	uint8_t 	header;		//Packet header (XMODEM_CMD_SOH)
	uint8_t 	packetNumber; //Packet sequence number
	uint8_t 	packetNumberC;	//Complement of packet sequence number
	uint8_t 	data [XMODEM_DATA_SIZE]; //Payload
	uint8_t 	crcH; //CRC high byte.
	uint8_t 	crcL; //CRC low byte.
}__packed XmodemPacket_t;

/**********************************************************
 * GLOBAL VARIABLES
 **********************************************************/

os_handle_t xmodem_evt_rcv;

/**********************************************************
 * PRIVATE VARIABLES
 **********************************************************/

static uint8_t packetNumber;
static bool started;
static os_xmodem_state xmodem_state;

/**********************************************************
 * PRIVATE FUNCTIONS
 **********************************************************/

/***********************************************************************
 * XMODEM CRC
 *
 * @brief This functions calculates the CRC using the previous result
 *
 * @param uint8_t newByte : 	[in] New byte to calculate
 * @param uint16_t prevResult : [in] Previous value of the CRC
 *
 * @return uint16_t : new CRC value
 *
 **********************************************************************/
static uint16_t xmodemCrc(const uint8_t newByte, uint16_t prevResult)
{
	prevResult = (uint16_t)((prevResult >> 8) | (prevResult << 8));
	prevResult = (uint16_t)(newByte ^ prevResult);
	prevResult ^= (uint16_t)((prevResult & 0xff) >> 4);
	prevResult ^= (uint16_t)((prevResult << 8) << 4);

	prevResult = (uint16_t)((((uint8_t) ((uint8_t) ((uint8_t) (prevResult & 0xff)) << 5)) | ((uint16_t) ((uint8_t) ((uint8_t) (prevResult & 0xff)) >> 3) << 8)) ^ prevResult);

	return prevResult;
}


/***********************************************************************
 * XMODEM CRC stream
 *
 * @brief This functions calculates the CRC of a buffer using the previous result
 *
 * @param uint8_t *buffer	  : [in] buffer to calculate
 * @param size_t length		  : [in] Size of the buffer
 * @param uint16_t prevResult : [in] Previous value of the CRC
 *
 * @return uint16_t : new CRC value
 *
 **********************************************************************/
static uint16_t xmodemCrcStream(const uint8_t *buffer, size_t length, uint16_t prevResult)
{
	size_t position = 0;
	for (; position < length; position++) {
		prevResult = xmodemCrc(buffer[position], prevResult);
	}

	return prevResult;
}


/***********************************************************************
 * XMODEM Send response
 *
 * @brief This functions sends a response to the transmitter
 *
 * @param uint8_t packet : 	[in] The response to send
 *
 **********************************************************************/
static void sendResponse(uint8_t packet)
{
	HAL_UART_Transmit(&USART_CLI, &packet, sizeof(packet), 1000);

	if (packet == XMODEM_CMD_CAN) {
		// If packet is CAN, send three times
		HAL_UART_Transmit(&USART_CLI, &packet, sizeof(packet), 1000);
		HAL_UART_Transmit(&USART_CLI, &packet, sizeof(packet), 1000);
	}
}


/***********************************************************************
 * XMODEM Parse packet
 *
 * @brief This functions checks a packet and returns a status
 *
 * @param XmodemPacket_t *packet : [ in] Packet to verify
 * @param uint8_t *response		 : [out] reponse
 *
 **********************************************************************/
static void xmodemParsePacket(XmodemPacket_t *packet, uint8_t *response)
{
	uint16_t crc16 = 0;

	switch (packet->header) {
	case XMODEM_CMD_SOH:
		// Packet number must start at 1, and must monotonically increase
		if (!started) {
			if (packet->packetNumber != 0x01) {
				*response = XMODEM_CMD_NAK;
				return;
			}
			started = true;
		} else {
			if (packet->packetNumber == packetNumber) {
				*response = XMODEM_CMD_ACK;
				return;
			} else if (packet->packetNumber != (uint8_t)(packetNumber + 1)) {
				*response = XMODEM_CMD_NAK;
				return;
			}
		}

		// Byte 3 is the two's complement of the packet number in the second byte
		if (packet->packetNumber + packet->packetNumberC != 0xFF) {
			*response = XMODEM_CMD_NAK;
			return;
		}

		// Bytes 132-133 contain a 16-bit CRC over the data bytes
		crc16 = xmodemCrcStream(packet->data, XMODEM_DATA_SIZE, crc16);

		if (((crc16 >> 8) & 0xFF) != packet->crcH) {
			*response = XMODEM_CMD_NAK;
			return;
		}

		if ((crc16 & 0xFF) != packet->crcL) {
			*response = XMODEM_CMD_NAK;
			return;
		}

		packetNumber = packet->packetNumber;
		*response = XMODEM_CMD_ACK;
		return;

	case XMODEM_CMD_EOT:
		*response = XMODEM_CMD_ACK;
		return;
		break;
	case XMODEM_CMD_CAN:
	case XMODEM_CMD_C:
	case XMODEM_CMD_CTRL_C:
		*response = XMODEM_CMD_CAN;
		return;
	default:
		*response = XMODEM_CMD_CAN;
		return;
	}
}

/**********************************************************
 * PUBLIC FUNCTIONS
 **********************************************************/

/***********************************************************************
 * XMODEM Get State
 *
 * @brief This functions gets the state of the XMODEM protocol
 *
 * @return bool : (1) on going, (0) idle
 *
 **********************************************************************/
os_xmodem_state xModem_getState(){
	return xmodem_state;
}

/***********************************************************************
 * XMODEM Receive
 *
 * @brief This functions receives a file via XMODEM protocol
 *
 * @param char* path : [in] the name of the file to create
 *
 **********************************************************************/
void* xModem_rcv(char* path){

	if(os_evt_create(&xmodem_evt_rcv, OS_EVT_MODE_AUTO, NULL) != OS_ERR_OK){
		return NULL;
	}

	os_err_e err;
	XmodemState_t state = XMODEM_STATE_WAIT_TO_START;
	uint8_t packet[133];
	uint8_t response = 0;
	int waitToStart = 0;
	uint16_t receivedBytes;
	uint32_t address = 0;

	lfs_file_t lfs_file;
	lfs_remove(&lfs, path);
	int32_t file_error = lfs_file_open(&lfs, &lfs_file, path, LFS_O_RDWR | LFS_O_CREAT);
	if(file_error<0) {
		os_evt_delete(xmodem_evt_rcv);
		return NULL;
	}

	HAL_UART_Abort(&USART_CLI);
	HAL_UART_Abort_IT(&USART_CLI);
	HAL_UART_AbortReceive(&USART_CLI);
	HAL_UART_AbortReceive_IT(&USART_CLI);
	__HAL_UART_FLUSH_DRREGISTER(&USART_CLI);

	xmodem_state = OS_XMODEM_STATE_RUN;

	packetNumber = 0;
	started = false;

	while(state != XMODEM_STATE_COMPLETED && state != XMODEM_STATE_FAILED) {
		if (state == XMODEM_STATE_WAIT_TO_START) {
			sendResponse(XMODEM_CMD_C);
			waitToStart++;
			if (waitToStart > XMODEM_START_TIMEOUT) {
				sendResponse(XMODEM_CMD_CAN);
				state = XMODEM_STATE_FAILED;
			}
		} else {
			if (XMODEM_STATE_IN_PROGRESS) {
				sendResponse(response);
			}
		}
		HAL_UART_Receive_IT(&USART_CLI, packet, 1);
		os_obj_single_wait(xmodem_evt_rcv, 1000, &err);
		receivedBytes = err == OS_ERR_OK ? 1 : 5;
		if (receivedBytes == 1 && packet[0] == XMODEM_CMD_SOH) {
			state = XMODEM_STATE_IN_PROGRESS;
			HAL_UART_Receive_IT(&USART_CLI, packet + 1, XMODEM_DATA_SIZE + 4);
			os_obj_single_wait(xmodem_evt_rcv, 1000, &err);
			receivedBytes = err == OS_ERR_OK ? (XMODEM_DATA_SIZE + 4) : 5;
			if (receivedBytes == (XMODEM_DATA_SIZE + 4)) {
				xmodemParsePacket((XmodemPacket_t *)packet, &response);
				if(response == XMODEM_CMD_ACK) {
					int err = lfs_file_write(&lfs, &lfs_file, ((XmodemPacket_t *)packet)->data, XMODEM_DATA_SIZE);
					if (err >= 0) {
						address += XMODEM_DATA_SIZE;
					} else {
						sendResponse(XMODEM_CMD_CAN);
						state = XMODEM_STATE_FAILED;
					}
				} else {
					sendResponse(XMODEM_CMD_CAN);
					state = XMODEM_STATE_FAILED;
				}
			} else {
				// if neither ACK nor CAN, the only possibility is NAK - which is not
				// handled currently, since hard to test whether it work
				sendResponse(XMODEM_CMD_CAN);
				state = XMODEM_STATE_FAILED;
			}
		} else {
			if (receivedBytes == 1 && ((XmodemPacket_t *)packet)->header == XMODEM_CMD_EOT) {
				sendResponse(XMODEM_CMD_ACK);
				state = XMODEM_STATE_COMPLETED;
			}
		}
		if (state == XMODEM_STATE_IN_PROGRESS && !(receivedBytes == 1 || receivedBytes == 132)) {
			sendResponse(XMODEM_CMD_CAN);
			state = XMODEM_STATE_FAILED;
		}
	}

	int lfs_err = lfs_file_close(&lfs, &lfs_file);
	if(lfs_err < 0)
		PRINTLN("LFS close return error ! %d", lfs_err);

	if(state == XMODEM_STATE_FAILED){
		lfs_remove(&lfs, path);
	}

	os_heap_free(path);
	xmodem_state = OS_XMODEM_STATE_STOP;

	os_evt_delete(xmodem_evt_rcv);

	cli_init();
	return NULL;
}
