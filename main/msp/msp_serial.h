#pragma once

#include "driver/gpio.h"
#include "driver/uart.h"

#define UART_BAUD_RATE 115200
#define UART_PORT_NUM UART_NUM_0
#define UART_RXD 3
#define UART_TXD 1
#define UART_RTS UART_PIN_NO_CHANGE
#define UART_CTS UART_PIN_NO_CHANGE
#define BUFFER_SIZE 265
#define TASK_STACK_SIZE 2048


typedef void (*mspRegisterCallback)(uint8_t command, uint8_t* payload, uint8_t payloadLen);

void serialWrite(const uint8_t* data, size_t len);
void mspSerialInit(void);
void mspRegisterFn(mspRegisterCallback callback);
void mspUnRegisterFn(mspRegisterCallback callback);