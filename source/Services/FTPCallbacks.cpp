/*
 * FTPCallbacks.cpp
 *
 *  Created on: Sep 27, 2020
 *      Author: kolban,sbela
 */
#include <stdint.h>
#include <fstream>
#include <iomanip>
#include <string.h>
#include "FTPServer.h"
#include "ftp_service.h"
#include "console.h"

static inline bool ends_with(std::string const &value, std::string const &ending)
{
	if (ending.size() > value.size())
		return false;
	return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

void FTPFirmwareCallbacks::onStoreStart(std::string fileName)
{
	PrintFTP("\r\n>> FTPFirmwareCallbacks::onStoreStart: fileName=%s\r\n", fileName.c_str());
	PrintFTP("\r\n<< FTPFirmwareCallbacks::onStoreStart\r\n");
} // FTPFirmwareCallbacks#onStoreStart

size_t FTPFirmwareCallbacks::onStoreData(uint8_t *data, size_t size)
{
	PrintFTP("\r\n>> FTPFirmwareCallbacks::onStoreData: size=%u\r\n", size);
	PrintFTP("\r\n<< FTPFirmwareCallbacks::onStoreData\r\n");
	return 0;
} // FTPFirmwareCallbacks#onStoreData

void FTPFirmwareCallbacks::onStoreEnd()
{
	PrintFTP("\r\n>> FTPFirmwareCallbacks::onStoreEnd\r\n");
	PrintFTP("\r\n<< FTPFirmwareCallbacks::onStoreEnd\r\n");
} // FTPFirmwareCallbacks#onStoreEnd

void FTPFirmwareCallbacks::onRetrieveStart(std::string fileName)
{
	PrintFTP("\r\n>> FTPFirmwareCallbacks::onRetrieveStart\r\n");
	PrintFTP("\r\n<< FTPFirmwareCallbacks::onRetrieveStart\r\n");
} // FTPFirmwareCallbacks#onRetrieveStart

size_t FTPFirmwareCallbacks::onRetrieveData(uint8_t *data, size_t size)
{
	PrintFTP("\r\n>> FTPFirmwareCallbacks::onRetrieveData\r\n");
	PrintFTP("\r\n<< FTPFirmwareCallbacks::onRetrieveData: 0\r\n");
	return 0;
} // FTPFirmwareCallbacks#onRetrieveData

void FTPFirmwareCallbacks::onRetrieveEnd()
{
	PrintFTP("\r\n>> FTPFirmwareCallbacks::onRetrieveEnd\r\n");
	PrintFTP("\r\n<< FTPFirmwareCallbacks::onRetrieveEnd\r\n");
} // FTPFirmwareCallbacks#onRetrieveEnd

std::string FTPFirmwareCallbacks::onDir()
{
	PrintFTP("\r\n>> FTPFirmwareCallbacks::onDir\r\n");
	PrintFTP("\r\n<< FTPFirmwareCallbacks::onDir\r\n");
	return "";
} // FTPFirmwareCallbacks#onDir

void FTPFirmwareCallbacks::onDele(std::string fileName)
{
	PrintFTP("\r\n>> FTPFirmwareCallbacks::onDele [%s]\r\n", fileName.c_str());
	PrintFTP("\r\n<< FTPFirmwareCallbacks::onDele\r\n");
}

FTPFirmwareCallbacks::~FTPFirmwareCallbacks()
{
} // FTPFirmwareCallbacks#~FTPCallbacks
