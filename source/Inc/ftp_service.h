/*
 * ftp_service.h
 *
 *  Created on: 2019. apr. 12.
 *      Author: sbela
 */

#ifndef _FTP_SERVICE_H
#define _FTP_SERVIVE_H

#include <string.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <ctype.h>

#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "console.h"

#define PrintFTP	Printf

#ifdef __cplusplus
extern "C" {
#endif
#define FTP_USERNAME		"autosys"
#define FTP_PASSWORD		"sysauto"

void ftp_server_task(void *pvParameter);
#ifdef __cplusplus
}
#endif

#endif //_FTP_SERVICE_H
