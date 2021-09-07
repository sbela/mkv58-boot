/*
 * console.c
 *
 *  Created on: Sep 1, 2021
 *      Author: sbela
 */

#include "FreeRTOS.h"
#include "semphr.h"
#include "lwip/tcpip.h"

#include "console.h"
#include "firmware.h"
#include "mutexlocker.h"
#include "EEPROM.h"

static SemaphoreHandle_t xUARTSemaphore = nullptr;

static InputState input_state = IS_Command;

static int strpos(char *haystack, const char *needle)
{
	char *p = strstr(haystack, needle);
	if (p)
		return p - haystack;
	return -1;
}

void InitConsole()
{
	xUARTSemaphore = xSemaphoreCreateMutex();
}

static char CBuffer[UART0_BACKGROUND_BUFFER_SIZE] = { 0 };
int Printf(const char *format, ...)
{
	AS::MutexLocker m(xUARTSemaphore);
	va_list vArgs;
	va_start(vArgs, format);
	int len = vsnprintf(CBuffer, UART0_BACKGROUND_BUFFER_SIZE, format, vArgs);
	va_end(vArgs);
	if (len > 0)
		return UART_RTOS_Send(&UART0_rtos_handle, (const uint8_t*)CBuffer, len);
	return len;
}

static char HBuffer[UART0_BACKGROUND_BUFFER_SIZE] = { 0 };
static char *HBuffPntr;
void HPrintf(const char *format, ...)
{
	va_list vArgs;
	va_start(vArgs, format);
	int len = vsnprintf((char*)HBuffer, UART0_BACKGROUND_BUFFER_SIZE, format, vArgs);
	va_end(vArgs);

	if (len > 0)
	{
		HBuffPntr = &HBuffer[0];
		while ((*HBuffPntr) != 0)
		{
			while (!(UART0->S1 & UART_S1_TDRE_MASK))
				;
			UART0->D = *HBuffPntr++;
		}
	}
}

static const char prompt[] = "\r\n# ";
static const char command_to_long[] = "\r\ncommand too long";
#define COMMAND_LEN		128
static char command_buffer[COMMAND_LEN];
static void ProcessCommand(int len);
static void ProcessReceived(uint8_t *buffer, int len)
{
	static int command_ptr = 0;
	if ((command_ptr + len) < COMMAND_LEN)
	{
		if (*buffer == '\b')
		{
			command_buffer[command_ptr] = 0;
			command_ptr--;
			UART_RTOS_Send(&UART0_rtos_handle, (const uint8_t*)"\b \b", 3);
		}
		else if (*buffer == '\r' || *buffer == '\n')
		{
			UART_RTOS_Send(&UART0_rtos_handle, buffer, len);
			ProcessCommand(command_ptr);
			memset(command_buffer, 0, command_ptr);
			command_ptr = 0;
			UART_RTOS_Send(&UART0_rtos_handle, (const uint8_t*)prompt, sizeof prompt);
		}
		else
		{
			memcpy(command_buffer + command_ptr, buffer, len);
			command_ptr += len;
			UART_RTOS_Send(&UART0_rtos_handle, buffer, len);
		}
	}
	else
	{
		UART_RTOS_Send(&UART0_rtos_handle, (const uint8_t*)command_to_long, sizeof command_to_long);
		memset(command_buffer, 0, command_ptr);
		command_ptr = 0;
		UART_RTOS_Send(&UART0_rtos_handle, (const uint8_t*)prompt, sizeof prompt);
	}
}

static const char *send_ring_overrun = "\r\nRing buffer overrun!\r\n";
static const char *send_hardware_overrun = "\r\nHardware buffer overrun!\r\n";
void uart_task(void *pvParameters)
{
	int error;
	size_t n;
	uint8_t recv_buffer[8];
	memset(command_buffer, 0, COMMAND_LEN);

	Printf("- RUN: serial RX task\r\n%s", prompt);
	/* Receive user input and send it back to terminal. */
	do
	{
		error = UART_RTOS_Receive(&UART0_rtos_handle, recv_buffer, 1, &n);
		if (error == kStatus_UART_RxHardwareOverrun)
		{
			/* Notify about hardware buffer overrun */
			if (kStatus_Success
					!= UART_RTOS_Send(&UART0_rtos_handle,
							(uint8_t*)send_hardware_overrun,
							strlen(send_hardware_overrun)))
			{
				printf("TX Suspend 1\r\n");
				vTaskSuspend(NULL);
			}
		}
		if (error == kStatus_UART_RxRingBufferOverrun)
		{
			/* Notify about ring buffer overrun */
			if (kStatus_Success
					!= UART_RTOS_Send(&UART0_rtos_handle,
							(uint8_t*)send_ring_overrun,
							strlen(send_ring_overrun)))
			{
				printf("TX Suspend 2\r\n");
				vTaskSuspend(NULL);
			}
		}
		if (n > 0)
		{
			recv_buffer[n] = 0;
			if (input_state == IS_Command)
				ProcessReceived(recv_buffer, n);
			else
				// IS_Firmware
				FirmwareDataReceived(recv_buffer, n);
		}
	} while (kStatus_Success == error);

	UART_RTOS_Deinit(&UART0_rtos_handle);
	printf("TX Suspend 3\r\n");
	vTaskSuspend(NULL);
}

void SetInputState(InputState state)
{
	input_state = state;
}

//--------------------------------------------------------------------------------------//
extern "C" void vApplicationStackOverflowHook(xTaskHandle *pxTask, signed char *pcTaskName)
{
	Printf("\nOVR: %p [%s]\r\n", pxTask, pcTaskName);
	while (1)
		;
}

__attribute__ ((weak))
void HardFault_Handler(void)
{
	HPrintf("\r\n-----------[ HARD FAULT ]-------------\r\n");
	while (1)
		;
}

#define HELP(x)	Printf("\r\n\t" x);
static void ProcessCommand(int len)
{
	if (strpos(command_buffer, "firm:") == 0)
	{
		int firmware_length = atoi(command_buffer + strlen("firm:"));
		if ((firmware_length > 0) && (firmware_length < FLASH_APP_LENGTH))
		{
			if (InitFirmwareTransfer(firmware_length))
				input_state = IS_Firmware;
		}
		else
			Printf("\r\nInvalid firmware length: %d", firmware_length);
		return;
	}
	if (strpos(command_buffer, "bootapp") == 0)
	{
		BootToApp();
	}
	if (strpos(command_buffer, "boot") == 0)
	{
		NVIC_SystemReset();
	}
	if (strpos(command_buffer, "setboot") == 0)
	{
		if (command_buffer[strlen("setboot")] == ' ')
		{
			int boot = atoi(command_buffer + strlen("setboot ")) ? 1 : 0;
			WriteDataToEEPROM(SM_BOOT_EXEC, (BYTE*)&boot, sizeof boot);
		}
		uint32_t stay_in_boot = 1000;
		ReadDataFromEEPROM(SM_BOOT_EXEC, (BYTE*)&stay_in_boot, sizeof stay_in_boot);
		Printf("\r\nBoot to app [%d]", stay_in_boot);
	}
	if (strpos(command_buffer, "ver") == 0)
	{
		Printf("BUILT: %s %s\r\n", version(0), version(1));
		return;
	}
	if (strpos(command_buffer, "ip") == 0)
	{
		ip4_addr_t ip;
		if (command_buffer[strlen("ip")] == ' ')
		{
			ip.addr = ipaddr_addr(command_buffer + strlen("ip "));
			WriteDataToEEPROM(SM_IP, (BYTE*)&ip.addr, 4);
		}
		ReadDataFromEEPROM(SM_IP, (BYTE*)&ip.addr, 4);
		Printf("\r\nIP: %s", ip4addr_ntoa(&ip));
		return;
	}
	if (strpos(command_buffer, "mask") == 0)
	{
		ip4_addr_t ip;
		if (command_buffer[strlen("mask")] == ' ')
		{
			ip.addr = ipaddr_addr(command_buffer + strlen("mask "));
			WriteDataToEEPROM(SM_MASK, (BYTE*)&ip.addr, 4);
		}
		ReadDataFromEEPROM(SM_MASK, (BYTE*)&ip.addr, 4);
		Printf("\r\nMASK: %s", ip4addr_ntoa(&ip));
		return;
	}
	if (strpos(command_buffer, "gw") == 0)
	{
		ip4_addr_t ip;
		if (command_buffer[strlen("gw")] == ' ')
		{
			ip.addr = ipaddr_addr(command_buffer + strlen("gw "));
			WriteDataToEEPROM(SM_GW, (BYTE*)&ip.addr, 4);
		}
		ReadDataFromEEPROM(SM_GW, (BYTE*)&ip.addr, 4);
		Printf("\r\nGW: %s", ip4addr_ntoa(&ip));
		return;
	}
	if (strpos(command_buffer, "help") == 0)
	{
		HELP("boot - reboot");
		HELP("bootapp - boots application");
		HELP("setboot - sets boot flag 1: stays in loader 0: boot to application");
		HELP("ver - current firmware version");
		HELP("ip - sets/gets current ip address");
		HELP("mask - sets/gets current network mask");
		HELP("gw - sets/gets current gateway address");
		return;
	}
}
