/*
 * network.c
 *
 *  Created on: Sep 5, 2021
 *      Author: sbela
 */

#include "lwip/opt.h"

#if LWIP_IPV4 && LWIP_RAW && LWIP_SOCKET

#include "ping.h"
#include "lwip/netifapi.h"
#include "lwip/tcpip.h"
#include "netif/ethernet.h"
#include "enet_ethernetif.h"

#include "board.h"
#include "fsl_phy.h"

#include "fsl_device_registers.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_phylan8742a.h"
#include "fsl_enet_mdio.h"

#include "EEPROM.h"
#include "console.h"

/* IP address configuration. */
#define configIP_ADDR0 192
#define configIP_ADDR1 168
#define configIP_ADDR2 100
#define configIP_ADDR3 10

/* Netmask configuration. */
#define configNET_MASK0 255
#define configNET_MASK1 255
#define configNET_MASK2 255
#define configNET_MASK3 0

/* Gateway address configuration. */
#define configGW_ADDR0 192
#define configGW_ADDR1 168
#define configGW_ADDR2 100
#define configGW_ADDR3 1

/* Address of PHY interface. */
#define AUTOSYS_PHY_ADDRESS BOARD_ENET0_PHY_ADDRESS

/* MDIO operations. */
#define AUTOSYS_MDIO_OPS enet_ops

/* PHY operations. */
#define AUTOSYS_PHY_OPS phy8742a_ops

/* ENET clock frequency. */
#define AUTOSYS_CLOCK_FREQ CLOCK_GetFreq(kCLOCK_CoreSysClk)

#ifndef AUTOSYS_NETIF_INIT_FN
/*! @brief Network interface initialization function. */
#define AUTOSYS_NETIF_INIT_FN ethernetif0_init
#endif /* AUTOSYS_NETIF_INIT_FN */

/*! @brief Stack size of the temporary lwIP initialization thread. */
#define INIT_THREAD_STACKSIZE 1024

/*! @brief Priority of the temporary lwIP initialization thread. */
#define INIT_THREAD_PRIO DEFAULT_THREAD_PRIO

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

static mdio_handle_t mdioHandle = { .ops = &AUTOSYS_MDIO_OPS };
static phy_handle_t phyHandle = { .phyAddr = AUTOSYS_PHY_ADDRESS, .mdioHandle = &mdioHandle, .ops = &AUTOSYS_PHY_OPS };

/*******************************************************************************
 * Code
 ******************************************************************************/

static uint8_t gl_NodeAddr[NETIF_MAX_HWADDR_LEN];
const uint8_t* MACAddress()
{
	return gl_NodeAddr;
}

void InitMACAddress()
{
	ReadEEPROMNodeAddress(gl_NodeAddr);
}

void PrintMACAddress()
{
	Printf("Node ID: %02x:%02x:%02x:%02x:%02x:%02x\r\n", gl_NodeAddr[0],
			gl_NodeAddr[1], gl_NodeAddr[2], gl_NodeAddr[3], gl_NodeAddr[4],
			gl_NodeAddr[5]);
}
/*!
 * @brief Initializes lwIP stack.
 */
static void stack_init(void *arg)
{
	static struct netif netif;
#if defined(FSL_FEATURE_SOC_LPC_ENET_COUNT) && (FSL_FEATURE_SOC_LPC_ENET_COUNT > 0)
    static mem_range_t non_dma_memory[] = NON_DMA_MEMORY_ARRAY;
#endif /* FSL_FEATURE_SOC_LPC_ENET_COUNT */
	ip4_addr_t netif_ipaddr, netif_netmask, netif_gw;
	ethernetif_config_t enet_config = {
			.phyHandle = &phyHandle,
	#if defined(FSL_FEATURE_SOC_LPC_ENET_COUNT) && (FSL_FEATURE_SOC_LPC_ENET_COUNT > 0)
        .non_dma_memory = non_dma_memory,
#endif /* FSL_FEATURE_SOC_LPC_ENET_COUNT */
			};

	memcpy(enet_config.macAddress, gl_NodeAddr, NETIF_MAX_HWADDR_LEN);

	LWIP_UNUSED_ARG(arg);
	mdioHandle.resource.csrClock_Hz = AUTOSYS_CLOCK_FREQ;

	IP4_ADDR(&netif_ipaddr, configIP_ADDR0, configIP_ADDR1, configIP_ADDR2, configIP_ADDR3);
	IP4_ADDR(&netif_netmask, configNET_MASK0, configNET_MASK1, configNET_MASK2, configNET_MASK3);
	IP4_ADDR(&netif_gw, configGW_ADDR0, configGW_ADDR1, configGW_ADDR2, configGW_ADDR3);

	tcpip_init(NULL, NULL);

	netifapi_netif_add(&netif, &netif_ipaddr, &netif_netmask, &netif_gw, &enet_config, AUTOSYS_NETIF_INIT_FN,
			tcpip_input);
	netifapi_netif_set_default(&netif);
	netifapi_netif_set_up(&netif);

	Printf("\r\n************************************************\r\n");
	Printf(" PING thread\r\n");
	Printf("************************************************\r\n");
	Printf(" IPv4 Address     : %u.%u.%u.%u\r\n", ((u8_t*)&netif_ipaddr)[0], ((u8_t*)&netif_ipaddr)[1],
			((u8_t*)&netif_ipaddr)[2], ((u8_t*)&netif_ipaddr)[3]);
	Printf(" IPv4 Subnet mask : %u.%u.%u.%u\r\n", ((u8_t*)&netif_netmask)[0], ((u8_t*)&netif_netmask)[1],
			((u8_t*)&netif_netmask)[2], ((u8_t*)&netif_netmask)[3]);
	Printf(" IPv4 Gateway     : %u.%u.%u.%u\r\n", ((u8_t*)&netif_gw)[0], ((u8_t*)&netif_gw)[1],
			((u8_t*)&netif_gw)[2], ((u8_t*)&netif_gw)[3]);
	Printf("************************************************\r\n");

	ping_init(&netif_gw);

	vTaskDelete(NULL);
}

/*!
 * @brief Main function
 */
void network_init(void *pvParameters)
{
	(void)pvParameters;
	/* Disable SYSMPU. */
	SYSMPU->CESR &= ~SYSMPU_CESR_VLD_MASK;

	mdioHandle.resource.csrClock_Hz = AUTOSYS_CLOCK_FREQ;

	/* Initialize lwIP from thread */
	if (sys_thread_new("stack_init", stack_init, NULL, INIT_THREAD_STACKSIZE, INIT_THREAD_PRIO) == NULL)
		LWIP_ASSERT("stack_init(): Task creation failed.", 0);
}
#endif
