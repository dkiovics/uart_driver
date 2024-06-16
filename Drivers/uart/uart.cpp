/*
 * uart.cpp
 *
 *  Created on: Jun 15, 2024
 *      Author: dkiovics
 */

#include "uart.h"


#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>


Uart::Uart(UART_HandleTypeDef *huart, IRQn_Type uartIr, IRQn_Type txDMAIr, uint16_t txBufferLength,
		uint16_t rxBufferLength, const char* ignoreableChars): huart(huart), uartIr(uartIr), txDMAIr(txDMAIr),
		txBufferLength(txBufferLength), rxBufferLength(rxBufferLength), ignoreableChars(ignoreableChars) {

	endOfTxData = txBufferLength - 1;

	txCircularBuffer = (char*)malloc(txBufferLength);
	rxCircularBuffer = (char*)malloc(rxBufferLength);

	HAL_UART_Receive_IT(huart, (uint8_t*)rxCircularBuffer, 1);
}


void Uart::handleTxCplt(UART_HandleTypeDef *huart){
	if(this->huart != huart)
		return;
	if(startOfTxData == -1){
		txInProgress = false;
		return;
	}
	int32_t charCount;
	if(startOfTxData <= endOfTxData){
		charCount = endOfTxData - startOfTxData + 1;
		HAL_UART_Transmit_DMA(huart, (uint8_t*)txCircularBuffer + startOfTxData, charCount);
		startOfTxData = -1;
	}
	else{
		charCount = txBufferLength - startOfTxData;
		HAL_UART_Transmit_DMA(huart, (uint8_t*)txCircularBuffer + startOfTxData, charCount);
		startOfTxData = 0;
	}
}


void Uart::transmit(const char *str){
	int32_t size = strlen(str);
	if(size > txBufferLength || size == 0)
		return;

	int32_t spaceTillBufferEnd = txBufferLength - endOfTxData - 1;

	if(spaceTillBufferEnd >= size){
		memcpy((uint8_t*)txCircularBuffer + endOfTxData + 1, (const uint8_t*)str, size);
		HAL_NVIC_DisableIRQ(uartIr);
		HAL_NVIC_DisableIRQ(txDMAIr);
		if(startOfTxData == -1){
			if(txInProgress){
				startOfTxData = endOfTxData + 1;
			}else{
				HAL_UART_Transmit_DMA(huart, (uint8_t*)txCircularBuffer + endOfTxData + 1, size);
				txInProgress = true;
			}
		}
		endOfTxData = endOfTxData + size;
		HAL_NVIC_EnableIRQ(uartIr);
		HAL_NVIC_EnableIRQ(txDMAIr);
	}else{
		if(spaceTillBufferEnd > 0)
			memcpy((uint8_t*)txCircularBuffer + endOfTxData + 1, (const uint8_t*)str, spaceTillBufferEnd);
		memcpy((uint8_t*)txCircularBuffer, (const uint8_t*)str + spaceTillBufferEnd, size - spaceTillBufferEnd);
		HAL_NVIC_DisableIRQ(uartIr);
		HAL_NVIC_DisableIRQ(txDMAIr);
		if(startOfTxData == -1){
			if(spaceTillBufferEnd == 0){
				if(txInProgress){
					startOfTxData = 0;
				}else{
					txInProgress = true;
					HAL_UART_Transmit_DMA(huart, (uint8_t*)txCircularBuffer, size);
				}
				endOfTxData = size - 1;
			}else{
				if(txInProgress){
					startOfTxData = endOfTxData + 1;
				}else{
					txInProgress = true;
					startOfTxData = 0;
					HAL_UART_Transmit_DMA(huart, (uint8_t*)txCircularBuffer + endOfTxData + 1, spaceTillBufferEnd);
				}
				endOfTxData = size - spaceTillBufferEnd - 1;
			}
		}else{
			endOfTxData = size - spaceTillBufferEnd - 1;
		}
		HAL_NVIC_EnableIRQ(uartIr);
		HAL_NVIC_EnableIRQ(txDMAIr);
	}
}

void Uart::handleRxCplt(UART_HandleTypeDef *huart){
	if(this->huart != huart)
		return;

	char c = rxCircularBuffer[rxPtr];

	const char* ptr = ignoreableChars;
	while(*ptr){
		if(c == *ptr){
			HAL_UART_Receive_IT(huart, (uint8_t*)rxCircularBuffer + rxPtr, 1);
			return;
		}
		ptr++;
	}

	if(rxCircularBuffer[rxPtr] == '\n'){
		mostRecentNewLinePos = rxPtr;
	}

	rxPtr++;
	if(rxPtr == rxBufferLength){
		rxPtrOverflow = 1;
		rxPtr = 0;
	}

	if(rxPtr == mostRecentNewLinePos)
		mostRecentNewLinePos = -1;

	if(rxPtr == startOfRxData){
		startOfRxData++;
		if(startOfRxData == rxBufferLength)
			startOfRxData = 0;
	}

	HAL_UART_Receive_IT(huart, (uint8_t*)rxCircularBuffer + rxPtr, 1);
}


bool Uart::receive(char* data){
	HAL_NVIC_DisableIRQ(uartIr);
	int32_t newLine = mostRecentNewLinePos;
	int32_t startOfData = startOfRxData;
	if(newLine == -1){
		HAL_NVIC_EnableIRQ(uartIr);
		return false;
	}
	mostRecentNewLinePos = -1;
	startOfRxData = newLine+1;
	if(startOfRxData == rxBufferLength)
		startOfRxData = 0;
	HAL_NVIC_EnableIRQ(uartIr);

	if(startOfData > newLine){
		uint32_t diff = rxBufferLength - startOfData;
		memcpy(data, (const uint8_t*)rxCircularBuffer + startOfData, diff);
		memcpy(data + diff, (const uint8_t*)rxCircularBuffer, newLine + 1);
		data[diff + newLine + 1] = '\0';
	}else{
		memcpy(data, (const uint8_t*)rxCircularBuffer + startOfData, newLine - startOfData + 1);
		data[newLine - startOfData + 1] = '\0';
	}

	return true;
}

