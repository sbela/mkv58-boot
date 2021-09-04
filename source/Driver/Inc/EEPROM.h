/*
 * EEPROM.h
 *
 *  Created on: 2014.09.15.
 *      Author: janyjozsef
 */

#ifndef EEPROM_H_
#define EEPROM_H_

#include "fsl_i2c.h"

typedef uint8_t BYTE;
// EEPROM
enum EEPROMRAMAddress
{
	SM_BOOT_EXEC,
	SM_STAY_IN_BOOT = 4,
	SM_BOOT_IP = 8,
	SM_ON_TIME = 20,
	SM_WORK_TIME = 24,
	SM_ID = 28,
	//---------------------------------------------------//
	SM_EEPROM_NODE = 0xFA	// 6 * 1byte (EEPROM MAC ADDR)
};

#ifdef __cplusplus
extern "C" {
#endif

void WriteDataToEEPROM(BYTE Addr, BYTE *Data, BYTE Len);
void ReadDataFromEEPROM(BYTE Addr, BYTE *Data, BYTE Len);
void ReadEEPROMNodeAddress(BYTE *NodeAddr);

#ifdef __cplusplus
}
#endif

#endif /* EEPROM_H_ */
