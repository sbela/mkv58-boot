/*
 * firmware.c
 *
 *  Created on: Sep 2, 2021
 *      Author: sbela
 */

#include "firmware.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "pin_mux.h"
#include "fsl_flash.h"
#include "fsl_smc.h"
#include "EEPROM.h"
#include "console.h"

void ReBoot()
{
	Printf("\r\nReset HW!");
	NVIC_SystemReset();
}

static flash_config_t flash_config;
static uint8_t firmware_flash_page[FLASH_PAGE_SIZE];

static inline int BootFlashErase()
{
	Printf("\r\nBootloader FlashErase!");
	flash_prot_state_t security_state;
	int __FLASH_CONFIG_START__ = 0x400;
	*((int*)__FLASH_CONFIG_START__) = 0xFFFFFFFF;
	status_t status = FLASH_IsProtected(&flash_config, FLASH_APP_START_ADDR, FLASH_APP_LENGTH, &security_state);
	if (status == kStatus_FTFx_Success)
	{
		if (security_state != kFLASH_ProtectionStateUnprotected)
		{
			int backdoorKey = FLASH_VALIDATE_KEY;
			status = FTFx_CMD_SecurityBypass(&flash_config.ftfxConfig[0], (const uint8_t*)&backdoorKey);
			if (status == kStatus_FTFx_Success)
			{
				status = FLASH_IsProtected(&flash_config, FLASH_APP_START_ADDR, FLASH_APP_LENGTH, &security_state);
				if (status != kStatus_FTFx_Success)
					Printf("\r\nBootloader FlashErase... Security STILL ON[%d]!", security_state);
			}
			else
				Printf("\r\nBootloader CMD SecurityBypass Failed! [%d]", status);
		}
		if (security_state == kFLASH_ProtectionStateUnprotected)
		{
			__disable_irq();
			status = FLASH_Erase(&flash_config, FLASH_APP_START_ADDR, FLASH_APP_LENGTH, FLASH_VALIDATE_KEY);
			__enable_irq();
			if (status != kStatus_FTFx_Success)
				Printf("\r\nFlashErase...[%d] [%08x]!", status, security_state);
			return status == 0 ? 1 : 0;
		}
		else
			Printf("\r\nBootloader FlashErase... Security ON[%d]!", security_state);
	}
	else
		Printf("\r\nBootloader FlashErase... Failed to get FLASH_IsProtected! [%d]", status);
	return 0;
}

static int flash_init_done = 0;
void FlashErase()
{
	status_t status = kStatus_FTFx_Success;
	if (flash_init_done == 0)
	{
		status = FLASH_Init(&flash_config);
		Printf("\r\nFlash init: %s [%d]\r\nFlash page size: %d\r\n", status == kStatus_FTFx_Success ? "Success" : "Failed!", status,
				flash_config.ftfxConfig[0].flashDesc.sectorSize);
		flash_init_done = 1;
	}

	if (status == kStatus_FTFx_Success)
	{
		Printf("\r\nFlashErase! [%08x]:[%08x]\r\n", FLASH_APP_START_ADDR, FLASH_APP_LENGTH);
		vTaskDelay(10);
		flash_prot_state_t security_state;
		status = FLASH_IsProtected(&flash_config, FLASH_APP_START_ADDR, FLASH_APP_LENGTH, &security_state);
		if (status == kStatus_FTFx_Success)
		{
			if (security_state == kFLASH_ProtectionStateUnprotected)
			{
				__disable_irq();
				status = FLASH_Erase(&flash_config, FLASH_APP_START_ADDR, FLASH_APP_LENGTH, FLASH_VALIDATE_KEY);
				__enable_irq();
				if (status != kStatus_FTFx_Success)
					Printf("\r\nFlashErase:[%d] [%08x]! [%08x]:[%08x]", status, security_state, FLASH_APP_START_ADDR, FLASH_APP_LENGTH);
				vTaskDelay(10);
			}
			else
				Printf("\r\nFlashErase... Security ON[%d]!", security_state);
		}
		else
			Printf("\r\nFlashErase... Failed to get FLASH_IsProtected! [%d]", status);
	}
}

/** \brief  Clear Enabled IRQs

 The function clears all device IRQs
 */
__STATIC_INLINE void NVIC_ClearEnabledIRQs(void)
{
	NVIC->ICER[0] = 0xFFFFFFFF;
	NVIC->ICER[1] = 0xFFFFFFFF;
	NVIC->ICER[2] = 0xFFFFFFFF;
	NVIC->ICER[3] = 0xFFFFFFFF;
	NVIC->ICER[4] = 0xFFFFFFFF;
	NVIC->ICER[5] = 0xFFFFFFFF;
	NVIC->ICER[6] = 0xFFFFFFFF;
	NVIC->ICER[7] = 0xFFFFFFFF;
}

/** \brief  Clear All Pending Interrupts

 The function clears the pending bits of all external interrupts.

 */
__STATIC_INLINE void NVIC_ClearAllPendingIRQs(void)
{
	NVIC->ICPR[0] = 0xFFFFFFFF;
	NVIC->ICPR[1] = 0xFFFFFFFF;
	NVIC->ICPR[2] = 0xFFFFFFFF;
	NVIC->ICPR[3] = 0xFFFFFFFF;
	NVIC->ICPR[4] = 0xFFFFFFFF;
	NVIC->ICPR[5] = 0xFFFFFFFF;
	NVIC->ICPR[6] = 0xFFFFFFFF;
	NVIC->ICPR[7] = 0xFFFFFFFF;
}
// See bl_shutdown_cleanup.h for documentation of this function.
static void shutdown_cleanup()
{
	// Turn off global interrupt
	__disable_irq();
	NVIC_ClearEnabledIRQs();
	NVIC_ClearAllPendingIRQs();
	SCB->VTOR = 0;
	// Restore global interrupt.
	__enable_irq();
	// Memory barriers for good measure.
	__ISB();
	__DSB();
}

static void jump_to_application(uint32_t applicationAddress, uint32_t stackPointer)
{
	shutdown_cleanup();

	// Create the function call to the user application.
	// Static variables are needed since changed the stack pointer out from under the compiler
	// we need to ensure the values we are using are not stored on the previous stack
	static uint32_t s_stackPointer = 0;
	s_stackPointer = stackPointer;
	static void (*farewellBootloader)(void) = 0;
	farewellBootloader = (void (*)(void))applicationAddress;

	// Set the VTOR to the application vector table address.
	SCB->VTOR = (uint32_t)APP_VECTOR_TABLE;

	// Set stack pointers to the application stack pointer.
	__set_MSP(s_stackPointer);
	__set_PSP(s_stackPointer);

	// Jump to the application.
	farewellBootloader();
	// Dummy fcuntion call, should never go to this fcuntion call
	shutdown_cleanup();
}

static void get_user_application_entry(uint32_t *appEntry, uint32_t *appStack)
{
	assert(appEntry);
	assert(appStack);
	*appEntry = APP_VECTOR_TABLE[kInitialPC];
	*appStack = APP_VECTOR_TABLE[kInitialSP];
}

void BootToApp()
{
	HPrintf("\r\nBooting: APP!");
	// Get the user application entry point and stack pointer.
	static uint32_t applicationAddress, stackPointer;
	get_user_application_entry(&applicationAddress, &stackPointer);
	jump_to_application(applicationAddress, stackPointer);
}

void SetBootToApp(int enable)
{
	WriteDataToEEPROM(SM_BOOT_EXEC, (BYTE*)&enable, 4);
	vTaskDelay(10);
	ReadDataFromEEPROM(SM_BOOT_EXEC, (BYTE*)&enable, 4);
	Printf("\r\n\tBoot to App is %s SET!", enable ? "" : "NOT");
}

static TimerHandle_t firmwareTimer = 0;
static void vFirmwareTimeoutCallback(TimerHandle_t xTimer)
{
	if (xTimer == firmwareTimer)
	{
		xTimerDelete(firmwareTimer, 0);
		SetInputState(IS_Command);
		HPrintf("FIRMWARE-TIMEOUT:\r\nBootloader 'Firmware receive' timeout!\r\n# ");
		firmwareTimer = 0;
	}
}

#define FIRMWARE_TIMER_TIMEOUT 5000
static int flash_page_pos = 0;
static int flash_program_start = 0;
static int flash_program_transfer_len = 0;
int InitFirmwareTransfer(int len)
{
	Printf("\r\nFirmware update started: %d byte!", len);
	status_t status = FLASH_Init(&flash_config);

	Printf("\r\nFlash init: %s [%d]\r\nFlash page size: %d\r\n", status == kStatus_FTFx_Success ? "Success" : "Failed!", status,
			flash_config.ftfxConfig[0].flashDesc.sectorSize);

	if (status == kStatus_FTFx_Success)
	{
		if (BootFlashErase())
		{
			firmwareTimer = xTimerCreate("FirmwareTimer", FIRMWARE_TIMER_TIMEOUT, pdFALSE, NULL, vFirmwareTimeoutCallback);
			memset(firmware_flash_page, 0, FLASH_PAGE_SIZE);
			flash_page_pos = 0;
			flash_program_start = FLASH_APP_START_ADDR;
			flash_program_transfer_len = len;
			SetBootToApp(0);
			Printf("FIRMWARE-START:");
			if (firmwareTimer && (xTimerStart(firmwareTimer , 0) != pdPASS))
				Printf("\r\nFirmware timeout timer start failed!");
			return 1;
		}
	}
	return 0;
}

void FirmwareDataReceived(uint8_t *data, size_t len)
{
	status_t status = kStatus_Success;
	while (len--)
	{
		if (flash_page_pos < FLASH_PAGE_SIZE)
		{
			firmware_flash_page[flash_page_pos++] = *data++;
			if (flash_page_pos % 8 == 0)
				Printf("A"); // ACK
		}

		if (flash_page_pos == FLASH_PAGE_SIZE)
		{
			Printf("*");
			__disable_irq();
			status = FLASH_Program(&flash_config, flash_program_start, firmware_flash_page, FLASH_PAGE_SIZE);
			__enable_irq();
			if (status != kStatus_Success)
				Printf("FLASH_Program failed: [%d] at [%08x]!\r\n", status, flash_program_start);
			memset(firmware_flash_page, 0, FLASH_PAGE_SIZE);
			flash_program_start += FLASH_PAGE_SIZE;
			flash_page_pos = 0;
		}

		if (--flash_program_transfer_len == 0)
		{
			if (flash_page_pos)
			{
				Printf("*");
				__disable_irq();
				status = FLASH_Program(&flash_config, flash_program_start, firmware_flash_page, flash_page_pos);
				__enable_irq();
				if (status != kStatus_Success)
					Printf("FLASH_Program failed: [%d] at [%08x]!\r\n", status, flash_program_start);
			}
			if (firmwareTimer)
				xTimerDelete(firmwareTimer, 0);

			SetInputState(IS_Command);
			Printf("\r\nBootloader Firmware received!\r\n# ");
			if (status == kStatus_Success)
				BootToApp();
		}
	}
}

void FirmwareDataCopyToApp()
{
	Printf("\r\nFirmware copy started: %d byte!", len);
	status_t status = FLASH_Init(&flash_config);

	Printf("\r\nFlash init: %s [%d]\r\nFlash page size: %d\r\n", status == kStatus_FTFx_Success ? "Success" : "Failed!", status,
			flash_config.ftfxConfig[0].flashDesc.sectorSize);

	if (status == kStatus_FTFx_Success)
	{
		if (BootFlashErase())
		{
			memset(firmware_flash_page, 0, FLASH_PAGE_SIZE);
			flash_page_pos = 0;
			flash_program_start = FLASH_APP_START_ADDR;
			flash_program_transfer_len = FLASH_APP_LENGTH;
			SetBootToApp(0);
			FirmwareDataReceived(FLASH_APP_LENGTH);
		}
	}
}
