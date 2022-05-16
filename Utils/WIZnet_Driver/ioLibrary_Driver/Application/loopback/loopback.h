#ifndef _LOOPBACK_H_
#define _LOOPBACK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "../../Ethernet/wizchip_conf.h"

#ifdef W5500_WITH_A7
// 20201015 taylor

#else
#if 1
// 20200527 taylor
#include "os_hal_uart.h"
#else
// mt3620 peri - 20200513
#include "../../../../lib/UART.h"
#include "../../../../lib/Print.h"
#endif
#endif

/* Loopback test debug message printout enable */
#define	_LOOPBACK_DEBUG_
//#undef _LOOPBACK_DEBUG_

/* DATA_BUF_SIZE define for Loopback example */
#ifndef DATA_BUF_SIZE
	#define DATA_BUF_SIZE			2048
#endif

/************************/
/* Select LOOPBACK_MODE */
/************************/
#define LOOPBACK_MAIN_NOBLOCK    0
#define LOOPBACK_MODE   LOOPBACK_MAIN_NOBLOCK


// int32_t tcp_server(uint8_t sn, uint8_t *buf, uint16_t port);

/* TCP server Loopback test example */
int32_t loopback_tcps(uint8_t sn, uint8_t* buf, uint16_t port);

/* TCP client Loopback test example */
int32_t loopback_tcpc(uint8_t sn, uint8_t* buf, uint8_t* destip, uint16_t destport);

/* UDP Loopback test example */
int32_t loopback_udps(uint8_t sn, uint8_t* buf, uint16_t port);

#ifdef __cplusplus
}
#endif

#endif
