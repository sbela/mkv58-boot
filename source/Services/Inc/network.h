/*
 * network.h
 *
 *  Created on: Sep 5, 2021
 *      Author: sbela
 */

#ifndef SERVICES_INC_NETWORK_H_
#define SERVICES_INC_NETWORK_H_

#ifdef __cplusplus
extern "C" {
#endif

void network_init(void*);
const uint8_t *MACAddress();
void PrintMACAddress();
void InitMACAddress();

#ifdef __cplusplus
}
#endif

#endif /* SERVICES_INC_NETWORK_H_ */
