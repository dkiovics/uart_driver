/*
 * myMain.cpp
 *
 *  Created on: May 8, 2024
 *      Author: dkiovics
 */


#include "main.h"
#include "uart.h"
#include <stdio.h>
#include <usart.h>

Uart* uart;

extern "C" {

volatile bool initComplete = false;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart1){
	if(initComplete)
		uart->handleRxCplt(huart1);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart1){
	if(initComplete)
		uart->handleTxCplt(huart1);
}

char inBuffer[1001];
char outBuffer[1100];

void myMain(){
	uart = new Uart(&huart1, USART1_IRQn, DMA2_Stream7_IRQn, 1000, 1000, "a");

	initComplete = true;

	while(true){
		if(uart->receive(inBuffer)){
			sprintf(outBuffer, "I received: %s", inBuffer);
			uart->transmit(outBuffer);
		}
	}
}


}

