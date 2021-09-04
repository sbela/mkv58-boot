/*
 * EEPROM.c
 *
 *  Created on: 2018. jún. 24.
 *      Author: janyjozsef
 */

#include "EEPROM.h"
#include "fsl_i2c.h"
#include "peripherals.h"
#include "console.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
static i2c_master_transfer_t masterXfer;
static status_t i2C_status;

void WriteDataToEEPROM(BYTE Addr, BYTE *Data, BYTE Len)
{
	masterXfer.direction = kI2C_Write;
	masterXfer.slaveAddress = 0x50U;
	masterXfer.subaddress = Addr;
	masterXfer.subaddressSize = 1;
	masterXfer.data = Data;
	masterXfer.dataSize = Len;
	masterXfer.flags = kI2C_TransferDefaultFlag;

	i2C_status = I2C_MasterTransferBlocking(I2C0_PERIPHERAL, &masterXfer);
	if (i2C_status != kStatus_Success)
	{
		Printf("\r\nI2C master: error during write transaction: %d\r\n",
				i2C_status);
	}
	vTaskDelay(4);
}

void ReadDataFromEEPROM(BYTE Addr, BYTE *Data, BYTE Len)
{
	masterXfer.direction = kI2C_Read;
	masterXfer.slaveAddress = 0x50U;
	masterXfer.subaddress = Addr;
	masterXfer.subaddressSize = 1;
	masterXfer.data = Data;
	masterXfer.dataSize = Len;
	masterXfer.flags = kI2C_TransferDefaultFlag;

	i2C_status = I2C_MasterTransferBlocking(I2C0_PERIPHERAL, &masterXfer);
	if (i2C_status != kStatus_Success)
	{
		Printf("\r\nI2C master: error during read transaction: %d\r\n",
				i2C_status);
	}
}

void ReadEEPROMNodeAddress(BYTE *NodeAddr)
{
	ReadDataFromEEPROM(SM_EEPROM_NODE, NodeAddr, 6);
	if ((NodeAddr[0] < 255) && (NodeAddr[1] < 255) && (NodeAddr[1] > 0))
		return;

	Printf("\r\nError MAC read: %02x:%02x:%02x:%02x:%02x:%02x\r\n", NodeAddr[0],
			NodeAddr[1], NodeAddr[2], NodeAddr[3], NodeAddr[4], NodeAddr[5]);

	NodeAddr[0] = 0x10;
	NodeAddr[1] = 0x24;
	NodeAddr[2] = 0x8E;
	NodeAddr[3] = 0x2B;
	NodeAddr[4] = 0x51;
	NodeAddr[5] = 0xFF;		// 5-től 0xFF-ig ????
}
