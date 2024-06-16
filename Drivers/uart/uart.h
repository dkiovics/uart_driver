/*
 * uart.h
 *
 *  Created on: Apr 11, 2023
 *      Author: dkiovics
 */

#ifndef UART_UART_H_
#define UART_UART_H_


#include "stm32f4xx_hal.h"

class Uart {
public:

	/**
	 * @brief Constructs an Uart object.
	 *
	 * @param huart - the HAL U(S)ART handle pointer
	 * @param uartIr - the uart IRQn_Type handle
	 * @param txDMAIr - the uart tx DMA IRQn_Type handle
	 * @param txBufferLength - the length of the transmit circular buffer
	 * @param rxBufferLength - the length of the receive circular buffer
	 * @param ignoreableChars - the list of characters that should be ignored (and not included) in the receive buffer
	 */
	Uart(UART_HandleTypeDef *huart, IRQn_Type uartIr, IRQn_Type txDMAIr, uint16_t txBufferLength, uint16_t rxBufferLength, const char* ignoreableChars);

	/**
	 * @brief Must be called when an uart dma transmit complete interrupt occurs
	 *
	 * @param huart - the IT uart handle
	 */
	void handleTxCplt(UART_HandleTypeDef *huart);

	/**
	 * @brief Must be called when an uart IT receive complete interrupt occurs
	 *
	 * @param huart - the IT uart handle
	 */
	void handleRxCplt(UART_HandleTypeDef *huart);

	/**
	 * @brief Transmits the given data in a non-blocking fashion.
	 *
	 * @param str - the string to transmit
	 */
	void transmit(const char *str);

	/**
	 * @brief Tries to read data from the receive buffer.
	 *
	 * @param data - the target buffer pointer, the new data will be written here, and delimited by a \0.
	 * @return whether there was any data ready to read.
	 */
	bool receive(char* data);

private:
	UART_HandleTypeDef* const huart;
	const IRQn_Type uartIr;
	const IRQn_Type txDMAIr;

	const int32_t txBufferLength;
	const int32_t rxBufferLength;

	volatile char* txCircularBuffer;
	volatile int32_t startOfTxData = -1;
	volatile int32_t endOfTxData;
	volatile bool txInProgress = false;

	volatile char* rxCircularBuffer;
	volatile int32_t startOfRxData = 0;
	volatile int32_t rxPtr = 0;
	volatile bool rxPtrOverflow = false;
	volatile int32_t mostRecentNewLinePos = -1;

	const char* const ignoreableChars;
};

#endif /* UART_UART_H_ */
