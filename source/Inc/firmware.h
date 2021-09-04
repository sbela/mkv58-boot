/*
 * firmware.h
 *
 *  Created on: Sep 2, 2021
 *      Author: sbela
 */

#ifndef INC_FIRMWARE_H_
#define INC_FIRMWARE_H_

#include <stddef.h>
#include <stdint.h>

#define FLASH_VALIDATE_KEY				0x6B65666B
#define FLASH_APP_START_ADDR			0x10020000
#define FLASH_APP_LENGTH				0x000e0000
#define APP_VECTOR_TABLE 				((uint32_t *)FLASH_APP_START_ADDR)
#define FLASH_PAGE_SIZE					8192

enum _vector_table_entries
{
	kInitialSP = 0, //!< Initial stack pointer.
	kInitialPC      //!< Reset vector.
};

#ifdef __cplusplus
extern "C" {
#endif

void SetBootToApp(int enable);
void BootToApp();
void ReBoot();
void FlashErase();
void FirmwareDataReceived(uint8_t *data, size_t len);
int InitFirmwareTransfer(int len);

#ifdef __cplusplus
}
#endif

#endif /* INC_FIRMWARE_H_ */