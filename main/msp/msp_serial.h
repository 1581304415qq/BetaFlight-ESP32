#pragma once

#include "soc/gpio_num.h"
#include "driver/uart.h"

#define UART_BAUD_RATE 115200
#define UART_PORT_NUM UART_NUM_0
#define UART_RXD GPIO_NUM_44
#define UART_TXD GPIO_NUM_43
#define UART_RTS UART_PIN_NO_CHANGE
#define UART_CTS UART_PIN_NO_CHANGE
#define BUFFER_SIZE 265
#define TASK_STACK_SIZE 2048*2


typedef void (*mspRegisterCallback)(uint8_t version, uint16_t command, uint8_t* payload, uint16_t payloadLen);

void serialWrite(const uint8_t* data, size_t len);
void mspSerialInit(void);
void mspRegisterFn(mspRegisterCallback callback);
void mspUnRegisterFn(mspRegisterCallback callback);