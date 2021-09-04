/*
 * Copyright 2016-2021 AutoSys
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file    main.c
 * @brief   Application entry point.
 */
#include <stdarg.h>
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MKV58F24.h"
#include "fsl_debug_console.h"

#include "EEPROM.h"
#include "console.h"
#include "firmware.h"
#include "ftp_service.h"

/*
 * @brief   Application entry point.
 */
static void MainTask(void *pvParameters);
static uint8_t gl_NodeAddr[6];

int main(void)
{
	/* Init board hardware. */
	BOARD_InitBootPins();
	BOARD_InitBootClocks();
	BOARD_InitBootPeripherals();

	uint32_t boot_is_pushed = GPIO_PinRead(BOARD_BOOT_GPIO, BOARD_BOOT_PIN);
	HPrintf("\r\nBOOT pin is : [%s]\r\n", boot_is_pushed ? "PUSHED" : "NOT PUSHED");
	uint32_t stay_in_boot = 1;
	ReadDataFromEEPROM(SM_BOOT_EXEC, (BYTE*)&stay_in_boot, sizeof stay_in_boot);
	HPrintf("\r\nSTAY IN BOOT is : [%s]\r\n", stay_in_boot ? "SET" : "NOT SET");

	if (boot_is_pushed || stay_in_boot)
	{
		xTaskCreate(MainTask, "MainTask", 256, NULL, (tskIDLE_PRIORITY + 1), NULL);
		//xTaskCreate(ftp_server_task, "ftp_server_task", 256, NULL, (tskIDLE_PRIORITY + 1), NULL);
		vTaskStartScheduler();
	}
	else
		BootToApp();
	return 0; // Never reached!
}

static const char *version_date[] = { __DATE__, __TIME__ };
const char* version(int ver)
{
	return version_date[ver];
}

static void MainTask(void *pvParameters)
{
	(void)pvParameters;

	int ulFlashRates[4] = { 100, 20, 20, 20 };
	int ulFlashRate = 0;
	int FlashTimer = 0;

	ReadEEPROMNodeAddress(gl_NodeAddr);

	Printf("\r\n");
	Printf("*********************************\r\n");
	Printf("AS_IPCard Boot\r\n");
	Printf("V1.0\r\n");
	Printf("Created: 2021.09\r\n");
	Printf("Program: AutoSys boot program\r\n");
	Printf("Node ID: %02x:%02x:%02x:%02x:%02x:%02x\r\n", gl_NodeAddr[0],
			gl_NodeAddr[1], gl_NodeAddr[2], gl_NodeAddr[3], gl_NodeAddr[4],
			gl_NodeAddr[5]);
	Printf("BUILT: %s %s\r\n", version_date[0], version_date[1]);
	Printf("*********************************\r\n");
	Printf("- RUN: uart task:");
	Printf("0x%x\r\n", xTaskCreate(uart_task, "uart_task", 256, NULL, tskIDLE_PRIORITY, NULL));

	while (1)
	{
		vTaskDelay(10);
		if (++FlashTimer > ulFlashRates[ulFlashRate])
		{
			if (++ulFlashRate >= 4)
				ulFlashRate = 0;
			FlashTimer = 0;
			GPIO_PortToggle(BOARD_LED_GREEN_GPIO, (1u << BOARD_LED_GREEN_PIN));
		}
	}
}