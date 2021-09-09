/*
 * console.h
 *
 *  Created on: Sep 1, 2021
 *      Author: sbela
 */

#ifndef INC_CONSOLE_H_
#define INC_CONSOLE_H_

#include <stdarg.h>
#include <stdio.h>
#include "fsl_uart_freertos.h"
#include "peripherals.h"

#define FIRMWARE_RECV_LEN	256

typedef enum InputState {
	IS_Command,
	IS_Firmware
} InputState;

#ifdef __cplusplus
extern "C" {
#endif

void HPrintf(const  char *format, ...);
int Printf(const  char *format, ...);
void uart_task(void *pvParameters);
const char *version(int ver);
void SetInputState(InputState state);
void InitConsole();

#ifdef __cplusplus
}
#endif

#endif /* INC_CONSOLE_H_ */
