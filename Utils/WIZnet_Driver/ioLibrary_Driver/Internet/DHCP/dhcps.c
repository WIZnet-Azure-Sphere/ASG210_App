#include "../../Ethernet/socket.h"
#include "dhcps.h"
#include <string.h>
#include <stdlib.h>

#ifdef W5500_WITH_A7
#include <applibs/log.h>
#define printf Log_Debug
#else
#include "printf.h"
#endif

//static struct dhcp_server_state dhcp_server_state_machine;
static uint8_t dhcp_server_state_machine = DHCP_SERVER_STATE_IDLE;
/* recorded the client MAC addr(default sudo mac) */
//static uint8_t dhcps_record_first_client_mac[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
/* recorded transaction ID (default sudo id)*/
static uint8_t dhcp_recorded_xid[4] = {0xff, 0xff, 0xff, 0xff}; 

/* UDP Protocol Control Block(PCB) */
static struct udp_pcb *dhcps_pcb;

static ip_addr dhcps_send_broadcast_address;
static ip_addr dhcps_local_address;

static ip_addr dhcps_local_mask;
static ip_addr dhcps_local_gateway;
static ip_addr dhcps_network_id;
static ip_addr dhcps_subnet_broadcast; 
static ip_addr dhcps_allocated_client_address;
#if 1
// 20200611
static ip_addr dhcps_renewal_client_address;
#endif
static int dhcps_addr_pool_set = 0;
static ip_addr dhcps_addr_pool_start;
static ip_addr dhcps_addr_pool_end;
#if 0
static ip_addr dhcps_owned_first_ip;
static ip_addr dhcps_owned_last_ip;
static uint8_t dhcps_num_of_available_ips;
#endif

#ifndef W5500_WITH_A7
#define USE_READ_SYSRAM
#endif

#ifdef USE_READ_SYSRAM
static dhcps_msg* __attribute__((unused, section(".sysram"))) dhcp_message_repository;
#else
static dhcps_msg *dhcp_message_repository;
#endif
static int dhcp_message_total_options_lenth;

/* allocated IP range */  
static struct table  ip_table;
static ip_addr client_request_ip;

static uint8_t dhcp_client_ethernet_address[16];
static uint8_t bound_client_ethernet_address[16];

extern wiz_NetInfo gWIZNETINFO;

uint8_t DHCPs_CHADDR[6]; // DHCP Server MAC address.
uint8_t DHCPs_SOCKET; // Socket number for DHCP
uint32_t DHCPs_XID;      // Any number
//dhcps_msg* pDHCPsMSG;      // Buffer pointer for DHCP processing
int8_t   dhcps_state        = 0;   // DHCP state

#if 1
// 20200604 timer
uint32_t dhcps_tick_1sec;
uint32_t dhcps_option_lease_time;
#endif

#define IP4_ADDR(ipaddr, a,b,c,d) \
        (ipaddr)->addr = htonl(((uint32_t)((a) & 0xff) << 24) | \
                               ((uint32_t)((b) & 0xff) << 16) | \
                               ((uint32_t)((c) & 0xff) << 8) | \
                                (uint32_t)((d) & 0xff))

#define ip4_addr1(ipaddr) ((uint16_t)(ntohl((ipaddr)->addr) >> 24) & 0xff)
#define ip4_addr2(ipaddr) ((uint16_t)(ntohl((ipaddr)->addr) >> 16) & 0xff)
#define ip4_addr3(ipaddr) ((uint16_t)(ntohl((ipaddr)->addr) >> 8) & 0xff)
#define ip4_addr4(ipaddr) ((uint16_t)(ntohl((ipaddr)->addr)) & 0xff)

uint16_t
htons(uint16_t n)
{
  return ((n & 0xff) << 8) | ((n & 0xff00) >> 8);
}


uint32_t
htonl(uint32_t n)
{
  return ((n & 0xff) << 24) |
    ((n & 0xff00) << 8) |
    ((n & 0xff0000UL) >> 8) |
    ((n & 0xff000000UL) >> 24);
}

uint32_t
ntohl(uint32_t n)
{
  return htonl(n);
}

#if 1
// 20200605
static uint8_t ismarked_ip_in_table(uint8_t d)
{
//#define DEBUG_ISMARKED_IP_IN_TABLE
  
	if(d != 0)
	{
		#ifdef DEBUG_ISMARKED_IP_IN_TABLE
		printf("marked? ip .%d\r\n", d);
		#endif

		if (0 < d && d <= 32)
		{
			#ifdef DEBUG_ISMARKED_IP_IN_TABLE
			printf("\r\n ip_table.ip_range[0] = 0x%x\r\n",ip_table.ip_range[0]);
			#endif  

			if(1<<(d-1) & ip_table.ip_range[0])
			{
				#ifdef DEBUG_ISMARKED_IP_IN_TABLE
				printf("\r\n ip .%d marked ip_table.ip_range[0] = 0x%x\r\n",d, ip_table.ip_range[0]);
				#endif

				return 1;
			}
			return 0;
		}
		else if (32 < d && d <= 64)
		{
			#ifdef DEBUG_ISMARKED_IP_IN_TABLE
			printf("\r\n ip_table.ip_range[1] = 0x%x\r\n",ip_table.ip_range[1]);
			#endif  

			if(1<<(d-1-32) & ip_table.ip_range[1])
			{
				#ifdef DEBUG_ISMARKED_IP_IN_TABLE
				printf("\r\n ip .%d marked ip_table.ip_range[1] = 0x%x\r\n",d, ip_table.ip_range[1]);
				#endif

				return 1;
			}
			return 0;
		}
		else if (64 < d && d <= 96)
		{
			#ifdef DEBUG_ISMARKED_IP_IN_TABLE
			printf("\r\n ip_table.ip_range[2] = 0x%x\r\n",ip_table.ip_range[2]);
			#endif  

			if(1<<(d-1-64) & ip_table.ip_range[2])
			{
				#ifdef DEBUG_ISMARKED_IP_IN_TABLE
				printf("\r\n ip .%d marked ip_table.ip_range[2] = 0x%x\r\n",d, ip_table.ip_range[2]);
				#endif

				return 1;
			}
			return 0;
		}
		else if (96 < d && d <= 128)
		{
			#ifdef DEBUG_ISMARKED_IP_IN_TABLE
			printf("\r\n ip_table.ip_range[3] = 0x%x\r\n",ip_table.ip_range[3]);
			#endif  

			if(1<<(d-1-96) & ip_table.ip_range[3])
			{
				#ifdef DEBUG_ISMARKED_IP_IN_TABLE
				printf("\r\n ip .%d marked ip_table.ip_range[3] = 0x%x\r\n",d, ip_table.ip_range[3]);
				#endif

				return 1;
			}
			return 0;
		}
	}
	return 0;
}
#endif

#if 1
// 20200605
static void unmark_ip_in_table(uint8_t d)
{
//#define DEBUG_UNMARK_IP_IN_TABLE
  if(d != 0)
  {
    #ifdef DEBUG_UNMARK_IP_IN_TABLE
    printf("unmark ip .%d\r\n", d);
    #endif
    if (0 < d && d <= 32)
    {
      #ifdef DEBUG_UNMARK_IP_IN_TABLE
  		printf("\r\n ip_table.ip_range[0] = 0x%x\r\n",ip_table.ip_range[0]);
      #endif  

		  ip_table.ip_range[0] = UNMARK_RANGE1_IP_BIT(ip_table, d);	
      
      #ifdef DEBUG_UNMARK_IP_IN_TABLE
  		printf("\r\n ip_table.ip_range[0] = 0x%x\r\n",ip_table.ip_range[0]);
      #endif	
  	}
    else if (32 < d && d <= 64)
    {
      #ifdef DEBUG_UNMARK_IP_IN_TABLE
  		printf("\r\n ip_table.ip_range[1] = 0x%x\r\n",ip_table.ip_range[1]);
      #endif	
      
    	ip_table.ip_range[1] = UNMARK_RANGE2_IP_BIT(ip_table, (d - 32));
      
      #ifdef DEBUG_UNMARK_IP_IN_TABLE
  		printf("\r\n ip_table.ip_range[1] = 0x%x\r\n",ip_table.ip_range[1]);
      #endif	
  	}
    else if (64 < d && d <= 96)
    {
      #ifdef DEBUG_UNMARK_IP_IN_TABLE
  		printf("\r\n ip_table.ip_range[2] = 0x%x\r\n",ip_table.ip_range[2]);
      #endif	
      
  		ip_table.ip_range[2] = UNMARK_RANGE3_IP_BIT(ip_table, (d - 64));
      
      #ifdef DEBUG_UNMARK_IP_IN_TABLE
  		printf("\r\n ip_table.ip_range[2] = 0x%x\r\n",ip_table.ip_range[2]);
      #endif	
  	}
    else if (96 < d && d <= 128)
    {
      #ifdef DEBUG_UNMARK_IP_IN_TABLE
  		printf("\r\n ip_table.ip_range[3] = 0x%x\r\n",ip_table.ip_range[3]);
      #endif	
      
  		ip_table.ip_range[3] = UNMARK_RANGE4_IP_BIT(ip_table, (d - 96));
      
      #ifdef DEBUG_UNMARK_IP_IN_TABLE
  		printf("\r\n ip_table.ip_range[3] = 0x%x\r\n",ip_table.ip_range[3]);
      #endif	
  	}
  }
}
#else
static void unmark_ip_in_table()
{
//#define DEBUG_UNMARK_IP_IN_TABLE
  uint8_t d;
  d = search_mac();
  if(d != 0)
  {
    #ifdef DEBUG_UNMARK_IP_IN_TABLE
    printf("unmark ip %d\r\n", d);
    #endif
    if (0 < d && d <= 32)
    {
      #ifdef DEBUG_UNMARK_IP_IN_TABLE
  		printf("\r\n ip_table.ip_range[0] = 0x%x\r\n",ip_table.ip_range[0]);
      #endif  

		  ip_table.ip_range[0] = UNMARK_RANGE1_IP_BIT(ip_table, d);	
      
      #ifdef DEBUG_UNMARK_IP_IN_TABLE
  		printf("\r\n ip_table.ip_range[0] = 0x%x\r\n",ip_table.ip_range[0]);
      #endif	
  	}
    else if (32 < d && d <= 64)
    {
      #ifdef DEBUG_UNMARK_IP_IN_TABLE
  		printf("\r\n ip_table.ip_range[1] = 0x%x\r\n",ip_table.ip_range[1]);
      #endif	
      
    	ip_table.ip_range[1] = UNMARK_RANGE2_IP_BIT(ip_table, (d - 32));
      
      #ifdef DEBUG_UNMARK_IP_IN_TABLE
  		printf("\r\n ip_table.ip_range[1] = 0x%x\r\n",ip_table.ip_range[1]);
      #endif	
  	}
    else if (64 < d && d <= 96)
    {
      #ifdef DEBUG_UNMARK_IP_IN_TABLE
  		printf("\r\n ip_table.ip_range[2] = 0x%x\r\n",ip_table.ip_range[2]);
      #endif	
      
  		ip_table.ip_range[2] = UNMARK_RANGE3_IP_BIT(ip_table, (d - 64));
      
      #ifdef DEBUG_UNMARK_IP_IN_TABLE
  		printf("\r\n ip_table.ip_range[2] = 0x%x\r\n",ip_table.ip_range[2]);
      #endif	
  	}
    else if (96 < d && d <= 128)
    {
      #ifdef DEBUG_UNMARK_IP_IN_TABLE
  		printf("\r\n ip_table.ip_range[3] = 0x%x\r\n",ip_table.ip_range[3]);
      #endif	
      
  		ip_table.ip_range[3] = UNMARK_RANGE4_IP_BIT(ip_table, (d - 96));
      
      #ifdef DEBUG_UNMARK_IP_IN_TABLE
  		printf("\r\n ip_table.ip_range[3] = 0x%x\r\n",ip_table.ip_range[3]);
      #endif	
  	}
  }
}
#endif

#if 1
// 20200611
static void mark_conflict_in_table(uint8_t d)
{
  #if (debug_dhcps)   
    printf("\r\n mark conflict ip .%d\r\n",d);
  #endif

  memset(ip_table.chaddr[d-1], 0, sizeof(ip_table.chaddr[d-1]));
}

#endif

#if 1
// 20200605
static void mark_ip_in_table(uint8_t d)
{
//#define DEBUG_MARK_IP_IN_TABLE

  #ifdef DEBUG_MARK_IP_IN_TABLE   
	printf("\r\n mark ip .%d\r\n",d);
	#endif	

	if(1 == ismarked_ip_in_table(d))
	{
		#ifdef DEBUG_MARK_IP_IN_TABLE   
		printf("\r\n already mark ip .%d\r\n",d);
		#endif
	}
	else
	{
		if (0 < d && d <= 32) {
			ip_table.ip_range[0] = MARK_RANGE1_IP_BIT(ip_table, d);	
			#ifdef DEBUG_MARK_IP_IN_TABLE		
			printf("\r\n ip_table.ip_range[0] = 0x%x\r\n",ip_table.ip_range[0]);
			#endif	
		} else if (32 < d && d <= 64) {
			ip_table.ip_range[1] = MARK_RANGE2_IP_BIT(ip_table, (d - 32));
			#ifdef DEBUG_MARK_IP_IN_TABLE	
			printf("\r\n ip_table.ip_range[1] = 0x%x\r\n",ip_table.ip_range[1]);
			#endif	
		} else if (64 < d && d <= 96) {
			ip_table.ip_range[2] = MARK_RANGE3_IP_BIT(ip_table, (d - 64));
			#ifdef DEBUG_MARK_IP_IN_TABLE	
			printf("\r\n ip_table.ip_range[2] = 0x%x\r\n",ip_table.ip_range[2]);
			#endif	
		} else if (96 < d && d <= 128) {
			ip_table.ip_range[3] = MARK_RANGE4_IP_BIT(ip_table, (d - 96));
			#ifdef DEBUG_MARK_IP_IN_TABLE	
			printf("\r\n ip_table.ip_range[3] = 0x%x\r\n",ip_table.ip_range[3]);
			#endif	
		} else {
			printf("\r\n Request ip over the range(1-128) \r\n");
		}
	}

  #if 1
  // 20200611
  memcpy(ip_table.chaddr[d-1], dhcp_client_ethernet_address, sizeof(ip_table.chaddr[d-1]));
  #else
	memcpy(ip_table.chaddr[d], dhcp_client_ethernet_address, sizeof(ip_table.chaddr[d]));
  #endif

	#ifdef DEBUG_MARK_IP_IN_TABLE
    #if 1
    // 20201028 taylor
    printf("\r\nMark %d.%d.%d.%d\r\n", ip4_addr1(&dhcps_local_address), ip4_addr2(&dhcps_local_address), ip4_addr3(&dhcps_local_address), d);
    #else
	printf("\r\nMark %d.%d.%d.%d\r\n", ip4_addr1(&dhcps_local_address), ip4_addr2(&dhcps_local_address), ip4_addr3(&dhcps_local_address), ip4_addr4(&dhcps_local_address));
    #endif
	for(int i=0; i<16; i++)
	{
		printf("%x:", ip_table.chaddr[d-1][i]);
	}
	printf("\r\n");
	#endif

	memcpy(ip_table.cltime[d-1], (uint8_t*)&dhcps_tick_1sec, sizeof(ip_table.cltime[d-1]));
	#ifdef DEBUG_MARK_IP_IN_TABLE
	printf("ip .%d Mark lease time : ", d);
	printf("%#x\r\n", dhcps_tick_1sec);
	#endif
}
#else
static void mark_ip_in_table(uint8_t d)
{
#if (debug_dhcps)   
  	printf("\r\n mark ip %d\r\n",d);
#endif	
	if (0 < d && d <= 32) {
		ip_table.ip_range[0] = MARK_RANGE1_IP_BIT(ip_table, d);	
#if (debug_dhcps)		
		printf("\r\n ip_table.ip_range[0] = 0x%x\r\n",ip_table.ip_range[0]);
#endif	
	} else if (32 < d && d <= 64) {
	  	ip_table.ip_range[1] = MARK_RANGE2_IP_BIT(ip_table, (d - 32));
#if (debug_dhcps)	
		printf("\r\n ip_table.ip_range[1] = 0x%x\r\n",ip_table.ip_range[1]);
#endif	
	} else if (64 < d && d <= 96) {
		ip_table.ip_range[2] = MARK_RANGE3_IP_BIT(ip_table, (d - 64));
#if (debug_dhcps)	
		printf("\r\n ip_table.ip_range[2] = 0x%x\r\n",ip_table.ip_range[2]);
#endif	
	} else if (96 < d && d <= 128) {
		ip_table.ip_range[3] = MARK_RANGE4_IP_BIT(ip_table, (d - 96));
#if (debug_dhcps)	
		printf("\r\n ip_table.ip_range[3] = 0x%x\r\n",ip_table.ip_range[3]);
#endif	
	} else {
	  	printf("\r\n Request ip over the range(1-128) \r\n");
	}

  memcpy(ip_table.chaddr[d], dhcp_client_ethernet_address, sizeof(ip_table.chaddr[d]));
  
#if (debug_dhcps)
  printf("\r\nMark %d.%d.%d.%d\r\n", ip4_addr1(&dhcps_local_address), ip4_addr2(&dhcps_local_address), ip4_addr3(&dhcps_local_address), ip4_addr4(&dhcps_local_address));
  for(int i=0; i<16; i++)
  {
    printf("%x:", ip_table.chaddr[d][i]);
  }
  printf("\r\n");
#endif
}
#endif

void dhcps_set_addr_pool(int addr_pool_set, ip_addr * addr_pool_start, ip_addr *addr_pool_end)
{

	if(addr_pool_set){
		dhcps_addr_pool_set = 1;

		memcpy(&dhcps_addr_pool_start, addr_pool_start,
							sizeof(ip_addr));

		memcpy(&dhcps_addr_pool_end, addr_pool_end,
							sizeof(ip_addr));
	}else{
		dhcps_addr_pool_set = 0;
	}

}


/** 
  * @brief  Initialize dhcp server.
  * @param  None.
  * @retval None.
  * Note, for now,we assume the server latch ip 192.168.1.1 and support dynamic 
  *       or fixed IP allocation. 
  */
void dhcps_init(uint8_t s, uint8_t * buf)
{
#if 1
    printf("dhcps_init\r\n");
#endif
//	printf("dhcps_init,wlan:%c\n\r",pnetif->name[1]);

  getSHAR(DHCPs_CHADDR);
  if((DHCPs_CHADDR[0] | DHCPs_CHADDR[1]  | DHCPs_CHADDR[2] | DHCPs_CHADDR[3] | DHCPs_CHADDR[4] | DHCPs_CHADDR[5]) == 0x00)
  {
    // assign temporary mac address, you should be set SHAR before call this function.
    DHCPs_CHADDR[0] = 0x00;
    DHCPs_CHADDR[1] = 0x08;
    DHCPs_CHADDR[2] = 0xdc;      
    DHCPs_CHADDR[3] = 0x00;
    DHCPs_CHADDR[4] = 0x00;
    DHCPs_CHADDR[5] = 0x00; 
    setSHAR(DHCPs_CHADDR);     
  }

  DHCPs_SOCKET = s; // SOCK_DHCP
  dhcp_message_repository = (dhcps_msg*)buf;
#ifdef USE_READ_SYSRAM
  printf("dhcp_message_repository = %#x\r\n", dhcp_message_repository);
#endif
  DHCPs_XID = 0x12345678;

  dhcps_state = 0;

  IP4_ADDR(&dhcps_send_broadcast_address, 255, 255, 255, 255);
	/* get net info from net interface */

  memcpy(&dhcps_local_address, gWIZNETINFO.ip, sizeof(gWIZNETINFO.ip));
	memcpy(&dhcps_local_mask, gWIZNETINFO.sn, sizeof(gWIZNETINFO.sn));
	memcpy(&dhcps_local_gateway, gWIZNETINFO.gw, sizeof(gWIZNETINFO.gw));

	/* calculate the usable network ip range */
	dhcps_network_id.addr = (dhcps_local_address.addr & (dhcps_local_mask.addr));
	
	dhcps_subnet_broadcast.addr = ((dhcps_network_id.addr | ~(dhcps_local_mask.addr)));
#if 0
	dhcps_owned_first_ip.addr = htonl((ntohl(dhcps_network_id.addr) + 1));
	dhcps_owned_last_ip.addr = htonl(ntohl(dhcps_subnet_broadcast.addr) - 1);
	dhcps_num_of_available_ips = ((ntohl(dhcps_owned_last_ip.addr) 
				- ntohl(dhcps_owned_first_ip.addr)) + 1); 
#endif

#if IS_USE_FIXED_IP
  IP4_ADDR(&dhcps_allocated_client_address, ip4_addr1(&dhcps_local_address),
    ip4_addr2(&dhcps_local_address), ip4_addr3(&dhcps_local_address),
    (ip4_addr4(&dhcps_local_address)) + 1 );
#else

	//dhcps_ip_table = (struct ip_table *)(pvPortMalloc(sizeof(struct ip_table)));
	memset(&ip_table, 0, sizeof(struct table));
	memset(&dhcps_allocated_client_address, 0, sizeof(ip_addr));
	memset(bound_client_ethernet_address, 0, sizeof(bound_client_ethernet_address));
	mark_ip_in_table((uint8_t)ip4_addr4(&dhcps_local_address));
	mark_ip_in_table((uint8_t)ip4_addr4(&dhcps_local_gateway));
#if 0
	for (i = 1; i < ip4_addr4(&dhcps_local_address); i++) {
		mark_ip_in_table(i);
	}
#endif	
#endif

#if 1
// 20200608
	dhcps_tick_1sec = 0;
	dhcps_option_lease_time = (dhcp_option_lease_time_one_day[0]<<24 | dhcp_option_lease_time_one_day[1]<<16 | dhcp_option_lease_time_one_day[2]<<8 | dhcp_option_lease_time_one_day[3]);
#endif

}

uint8_t dhcps_handle_state_machine_change(uint8_t option_message_type)
{
//#define DEBUG_DHCPS_HANDLE_STATE_MACHINE_CHANGE

	switch (option_message_type) {
	case DHCP_MESSAGE_TYPE_DECLINE:
		#if 1
		// 20200611
		#ifdef DEBUG_DHCPS_HANDLE_STATE_MACHINE_CHANGE
		printf("DECLINE IP address is %d\r\n", ip4_addr4(&client_request_ip));
		#endif
		mark_conflict_in_table(ip4_addr4(&client_request_ip));
		dhcp_server_state_machine = DHCP_SERVER_STATE_DECLINE;
		#else
		dhcp_server_state_machine = DHCP_SERVER_STATE_IDLE;
		#endif
		break;
	case DHCP_MESSAGE_TYPE_DISCOVER:
		if (dhcp_server_state_machine == DHCP_SERVER_STATE_IDLE) {
			dhcp_server_state_machine = DHCP_SERVER_STATE_OFFER;
		}
		break;
	case DHCP_MESSAGE_TYPE_REQUEST:

#if (!IS_USE_FIXED_IP) 	
		if (dhcp_server_state_machine == DHCP_SERVER_STATE_OFFER) {
			if (ip4_addr4(&dhcps_allocated_client_address) != 0) { 
				if (memcmp((void *)&dhcps_allocated_client_address, (void *)&client_request_ip, 4) == 0) {  	
					dhcp_server_state_machine = DHCP_SERVER_STATE_ACK;
			  	} else {
				  	dhcp_server_state_machine = DHCP_SERVER_STATE_NAK;
			  	}
			} else {
			  	dhcp_server_state_machine = DHCP_SERVER_STATE_NAK;
			}
		} else if (dhcp_server_state_machine == DHCP_SERVER_STATE_IDLE) {
		#if 1
      #ifdef DEBUG_DHCPS_HANDLE_STATE_MACHINE_CHANGE
      printf("%d RENEWAL\r\n", __LINE__);
      #endif

      #ifdef DEBUG_DHCPS_HANDLE_STATE_MACHINE_CHANGE
      // Client IP Address
      printf("Client IP %d.%d.%d.%d\r\n",
        dhcp_message_repository->ciaddr[0], dhcp_message_repository->ciaddr[1],
        dhcp_message_repository->ciaddr[2], dhcp_message_repository->ciaddr[3]
        );
      printf("Client Hardware %#x:%#x:%#x:%#x:%#x:%#x:%#x:%#x:%#x:%#x:%#x:%#x:%#x:%#x:%#x:%#x\r\n",
        dhcp_message_repository->chaddr[0], dhcp_message_repository->chaddr[1],
        dhcp_message_repository->chaddr[2], dhcp_message_repository->chaddr[3],
        dhcp_message_repository->chaddr[4], dhcp_message_repository->chaddr[5],
        dhcp_message_repository->chaddr[6], dhcp_message_repository->chaddr[7],
        dhcp_message_repository->chaddr[8], dhcp_message_repository->chaddr[9],
        dhcp_message_repository->chaddr[10], dhcp_message_repository->chaddr[11],
        dhcp_message_repository->chaddr[12], dhcp_message_repository->chaddr[13],
        dhcp_message_repository->chaddr[14], dhcp_message_repository->chaddr[15]
        );
      #endif

      // if client ip = 0.0.0.0
      if(dhcp_message_repository->ciaddr[0] == 0 && dhcp_message_repository->ciaddr[1] == 0 &&
        dhcp_message_repository->ciaddr[2] == 0 && dhcp_message_repository->ciaddr[3] == 0
        )
      {
        #ifdef DEBUG_DHCPS_HANDLE_STATE_MACHINE_CHANGE
        printf("Client IP address is 0\r\n");
        printf("Requested IP address is %d\r\n", ip4_addr4(&client_request_ip));
        #endif
          
        if(ip4_addr1(&client_request_ip) == 0 && ip4_addr2(&client_request_ip) == 0 && 
          ip4_addr3(&client_request_ip) == 0 && ip4_addr4(&client_request_ip) == 0
          )
        {
          dhcp_server_state_machine = DHCP_SERVER_STATE_NAK;
          break;
        }
        else
        {
          IP4_ADDR(&dhcps_renewal_client_address, ip4_addr1(&client_request_ip),
          ip4_addr2(&client_request_ip), ip4_addr3(&client_request_ip), ip4_addr4(&client_request_ip));
        }
      }
      else
      {
        IP4_ADDR(&dhcps_renewal_client_address, dhcp_message_repository->ciaddr[0],
          dhcp_message_repository->ciaddr[1], dhcp_message_repository->ciaddr[2], dhcp_message_repository->ciaddr[3]);
      }

      if(0 == ismarked_ip_in_table(ip4_addr4(&dhcps_renewal_client_address)))
      {
        #ifdef DEBUG_DHCPS_HANDLE_STATE_MACHINE_CHANGE
        printf("Not in a pool\r\n");
        #endif
        dhcp_server_state_machine = DHCP_SERVER_STATE_NAK;
        break;
      }
      #ifdef DEBUG_DHCPS_HANDLE_STATE_MACHINE_CHANGE
      printf("In a pool\r\n");
      #endif
      dhcp_server_state_machine = DHCP_SERVER_STATE_RENEWAL;
    #else
			if ((ip4_addr4(&dhcps_allocated_client_address) != 0) &&
				(memcmp((void *)&dhcps_allocated_client_address, (void *)&client_request_ip, 4) == 0) &&
				(memcmp((void *)&bound_client_ethernet_address, (void *)&dhcp_client_ethernet_address, 16) == 0)) {
				dhcp_server_state_machine = DHCP_SERVER_STATE_ACK;
			} else if ((ip4_addr1(&client_request_ip) == ip4_addr1(&dhcps_network_id)) &&
				(ip4_addr2(&client_request_ip) == ip4_addr2(&dhcps_network_id)) &&
				(ip4_addr3(&client_request_ip) == ip4_addr3(&dhcps_network_id))) {
				uint8_t request_ip4 = (uint8_t) ip4_addr4(&client_request_ip);
				if ((request_ip4 != 0) && (((request_ip4 - 1) / 32) >= 0) && (((request_ip4 - 1) / 32) <= 3) &&
					(((ip_table.ip_range[(request_ip4 - 1) / 32] >> ((request_ip4 - 1) % 32)) & 0x01) == 0)) {
					IP4_ADDR(&dhcps_allocated_client_address, (ip4_addr1(&dhcps_network_id)),
						ip4_addr2(&dhcps_network_id), ip4_addr3(&dhcps_network_id), request_ip4);
					memcpy(bound_client_ethernet_address, dhcp_client_ethernet_address, sizeof(bound_client_ethernet_address));
					dhcp_server_state_machine = DHCP_SERVER_STATE_ACK;
				} else {
					dhcp_server_state_machine = DHCP_SERVER_STATE_NAK;
				}
			} else {
				dhcp_server_state_machine = DHCP_SERVER_STATE_NAK;
			}
    #endif
		} else {
			dhcp_server_state_machine = DHCP_SERVER_STATE_NAK;
		}
#else		
		if (!(dhcp_server_state_machine == DHCP_SERVER_STATE_ACK ||
			dhcp_server_state_machine == DHCP_SERVER_STATE_NAK)) {
		        dhcp_server_state_machine = DHCP_SERVER_STATE_NAK;
		}
#endif
		break;
	case DHCP_MESSAGE_TYPE_RELEASE:
		dhcp_server_state_machine = DHCP_SERVER_STATE_RELEASE;
		break;
	}

	return dhcp_server_state_machine;
}


/**
  * @brief  parse the dhcp message option part.
  * @param  optptr: the addr of the first option field. 
  *         len: the total length of all option fields.          
  * @retval dhcp server state.
  */
static uint8_t dhcps_handle_msg_options(uint8_t *option_start, int16_t total_option_length)
{
       
	int16_t option_message_type = 0;
  uint8_t *option_end = option_start + total_option_length;
  //dhcp_server_state_machine = DHCP_SERVER_STATE_IDLE;

  /* begin process the dhcp option info */
  while (option_start < option_end)
  { 
    switch ((uint8_t)*option_start)
    {
      case DHCP_OPTION_CODE_MSG_TYPE: 
        option_message_type = *(option_start + 2); // 2 => code(1)+lenth(1)
      break;
      
      case DHCP_OPTION_CODE_REQUEST_IP_ADDRESS : 
        #if IS_USE_FIXED_IP
        if (memcmp((char *)&dhcps_allocated_client_address,
          (char *)option_start + 2, 4) == 0)
          dhcp_server_state_machine = DHCP_SERVER_STATE_ACK;
        else 
          dhcp_server_state_machine = DHCP_SERVER_STATE_NAK;
        #else                   		
        memcpy((char *)&client_request_ip, (char *)option_start + 2, 4);	
        #endif
      break;
    } 
    // calculate the options offset to get next option's base addr
    option_start += option_start[1] + 2; // optptr[1]: length value + (code(1)+ Len(1))
  }
	return dhcps_handle_state_machine_change(option_message_type);        
}

/**
  * @brief  get message from buffer then check whether it is dhcp related or not.
  *         if yes , parse it more to undersatnd the client's request.
  * @param  same as recv callback function definition
  * @retval if message is dhcp related then return dhcp server state,
  *	    otherwise return 0
  */
static uint8_t dhcps_check_msg_and_handle_options(uint16_t len)
{
	int dhcp_message_option_offset;
//#define DEBUG_DHCPS_CHECK_MSG_AND_HANDLE_OPTIONS

#ifdef DEBUG_DHCPS_CHECK_MSG_AND_HANDLE_OPTIONS  
  int i;
#endif
	memcpy(dhcp_client_ethernet_address, dhcp_message_repository->chaddr, sizeof(dhcp_client_ethernet_address));
#ifdef DEBUG_DHCPS_CHECK_MSG_AND_HANDLE_OPTIONS
  for(i=0;i<16;i++)
  {
    printf("dhcp_client_ethernet_address[%d] = %x\r\n", i, dhcp_client_ethernet_address[i]);
  }
#endif

	dhcp_message_option_offset = ((int)dhcp_message_repository->options - (int)dhcp_message_repository);
#ifdef DEBUG_DHCPS_CHECK_MSG_AND_HANDLE_OPTIONS
  printf("(int)dhcp_message_repository->options = %x(%d)\r\n", (int)dhcp_message_repository->options, (int)dhcp_message_repository->options);
  printf("(int)dhcp_message_repository->op = %x(%d)\r\n", (int)dhcp_message_repository->op, (int)dhcp_message_repository->op);

  printf("dhcp_message_option_offset = %x(%d)\r\n", dhcp_message_option_offset, dhcp_message_option_offset);
#endif

	dhcp_message_total_options_lenth = (len - dhcp_message_option_offset);
#ifdef DEBUG_DHCPS_CHECK_MSG_AND_HANDLE_OPTIONS
  printf("dhcp_message_total_options_lenth = %x(%d)\r\n", dhcp_message_total_options_lenth, dhcp_message_total_options_lenth);
#endif

	/* check the magic number,if correct parse the content of options */
	if (memcmp((char *)dhcp_message_repository->options,
		(char *)dhcp_magic_cookie, sizeof(dhcp_magic_cookie)) == 0) {
  	return dhcps_handle_msg_options(&dhcp_message_repository->options[4], (dhcp_message_total_options_lenth - 4));
	}
        
	return 0;
}

static uint8_t search_mac()
{
//#define DEBUG_SEARCH_MAC

  uint8_t i;
  for(i=0; i<128; i++)
  {
    if(0 == memcmp(ip_table.chaddr[i], dhcp_client_ethernet_address, sizeof(dhcp_client_ethernet_address)))
    {
      #ifdef DEBUG_SEARCH_MAC
      printf("Matched %d\r\n", i);
      #endif
      #if 1
      // 20200611
      return i+1;
      #else
      return i;
      #endif
    }
  }

  return 0;
}


/**
  * @brief  get one usable ip from the ip table of dhcp server. 
  * @param: None 
  * @retval the usable index which represent the ip4_addr(ip) of allocated ip addr.
  */
#if (!IS_USE_FIXED_IP)
static uint8_t search_next_ip(void)
{       
	uint8_t range_count, offset_count;
	uint8_t start, end;
	if(dhcps_addr_pool_set){
		start = (uint8_t)ip4_addr4(&dhcps_addr_pool_start);
		end = (uint8_t)ip4_addr4(&dhcps_addr_pool_end);
	}else{
		start = 0;
		end = 255;
	}
	for (range_count = 0; range_count < 4; range_count++) {
		for (offset_count = 0;offset_count < 32; offset_count++) {
			if ((((ip_table.ip_range[range_count] >> offset_count) & 0x01) == 0) 
				&&(((range_count * 32) + (offset_count + 1)) >= start)
				&&(((range_count * 32) + (offset_count + 1)) <= end)) {
				return ((range_count * 32) + (offset_count + 1));
			}
		}
	}
	return 0;
}
#endif

/**
  * @brief  fill in the option field with message type of a dhcp message. 
  * @param  msg_option_base_addr: the addr be filled start.
  *	    message_type: the type code you want to fill in 
  * @retval the start addr of the next dhcp option.
  */
static uint8_t *add_msg_type(uint8_t *msg_option_base_addr, uint8_t message_type)
{
	uint8_t *option_start;
	msg_option_base_addr[0] = DHCP_OPTION_CODE_MSG_TYPE;
	msg_option_base_addr[1] = DHCP_OPTION_LENGTH_ONE;
	msg_option_base_addr[2] = message_type;
	option_start = msg_option_base_addr + 3;
	if (DHCP_MESSAGE_TYPE_NAK == message_type)
		*option_start++ = DHCP_OPTION_CODE_END;		
	return option_start;
}


static uint8_t *fill_one_option_content(uint8_t *option_base_addr,
	uint8_t option_code, uint8_t option_length, void *copy_info)
{
	uint8_t *option_data_base_address;
	uint8_t *next_option_start_address = NULL;
	option_base_addr[0] = option_code;
	option_base_addr[1] = option_length;
	option_data_base_address = option_base_addr + 2;
	switch (option_length) {
	case DHCP_OPTION_LENGTH_FOUR:
		memcpy(option_data_base_address, copy_info, DHCP_OPTION_LENGTH_FOUR);
		next_option_start_address = option_data_base_address + 4;
		break;
	case DHCP_OPTION_LENGTH_TWO:
		memcpy(option_data_base_address, copy_info, DHCP_OPTION_LENGTH_TWO);
		next_option_start_address = option_data_base_address + 2;
		break;
	case DHCP_OPTION_LENGTH_ONE:
        #if 1
		// 20201028 taylor
        *option_data_base_address = 0;
        #else
		memcpy(option_data_base_address, copy_info, DHCP_OPTION_LENGTH_ONE);
        #endif
		next_option_start_address = option_data_base_address + 1;
		break;
	}

	return next_option_start_address;
}


/**
  * @brief  fill in the needed content of the dhcp offer message. 
  * @param  optptr  the addr which the tail of dhcp magic field. 
  * @retval the addr represent to add the end of option.
  */
static void add_offer_options(uint8_t *option_start_address)
{
	uint8_t *temp_option_addr;
	/* add DHCP options 1. 
	The subnet mask option specifies the client's subnet mask */
	temp_option_addr = fill_one_option_content(option_start_address,
			DHCP_OPTION_CODE_SUBNET_MASK, DHCP_OPTION_LENGTH_FOUR,
					(void *)&dhcps_local_mask);
	
  /* add DHCP options 3 (i.e router(gateway)). The time server option 
  specifies a list of RFC 868 [6] time servers available to the client. */
	temp_option_addr = fill_one_option_content(temp_option_addr,
			DHCP_OPTION_CODE_ROUTER, DHCP_OPTION_LENGTH_FOUR,
					(void *)&dhcps_local_address);

	/* add DHCP options 6 (i.e DNS). 
        The option specifies a list of DNS servers available to the client. */
	temp_option_addr = fill_one_option_content(temp_option_addr,
			DHCP_OPTION_CODE_DNS_SERVER, DHCP_OPTION_LENGTH_FOUR,
					(void *)&dhcps_local_address);
  
	/* add DHCP options 51.
	This option is used to request a lease time for the IP address. */
	temp_option_addr = fill_one_option_content(temp_option_addr,
			DHCP_OPTION_CODE_LEASE_TIME, DHCP_OPTION_LENGTH_FOUR,
					(void *)&dhcp_option_lease_time_one_day);
  
	/* add DHCP options 54. 
	The identifier is the IP address of the selected server. */
	temp_option_addr = fill_one_option_content(temp_option_addr,
			DHCP_OPTION_CODE_SERVER_ID, DHCP_OPTION_LENGTH_FOUR,
				(void *)&dhcps_local_address);
  
	/* add DHCP options 28. 
	This option specifies the broadcast address in use on client's subnet.*/
	temp_option_addr = fill_one_option_content(temp_option_addr,
		DHCP_OPTION_CODE_BROADCAST_ADDRESS, DHCP_OPTION_LENGTH_FOUR,
				(void *)&dhcps_subnet_broadcast);
  
	/* add DHCP options 26. 
	This option specifies the Maximum transmission unit to use */
	temp_option_addr = fill_one_option_content(temp_option_addr,
		DHCP_OPTION_CODE_INTERFACE_MTU, DHCP_OPTION_LENGTH_TWO,
					(void *) &dhcp_option_interface_mtu_576);
  
	/* add DHCP options 31.
	This option specifies whether or not the client should solicit routers */
	temp_option_addr = fill_one_option_content(temp_option_addr,
		DHCP_OPTION_CODE_PERFORM_ROUTER_DISCOVERY, DHCP_OPTION_LENGTH_ONE,
								NULL);
  
	*temp_option_addr++ = DHCP_OPTION_CODE_END;

}


/**
  * @brief  fill in common content of a dhcp message.  
  * @param  m the pointer which point to the dhcp message store in.
  * @retval None.
  */
static void dhcps_initialize_message(dhcps_msg *dhcp_message_repository, ip_addr yiaddr)
{     
  dhcp_message_repository->op = DHCP_MESSAGE_OP_REPLY;
  dhcp_message_repository->htype = DHCP_MESSAGE_HTYPE;
  dhcp_message_repository->hlen = DHCP_MESSAGE_HLEN; 
  dhcp_message_repository->hops = 0;		
  memcpy((char *)dhcp_recorded_xid, (char *) dhcp_message_repository->xid,
  	sizeof(dhcp_message_repository->xid));
  dhcp_message_repository->secs = 0;
  dhcp_message_repository->flags = htons(BOOTP_BROADCAST);         

  memcpy((char *)dhcp_message_repository->yiaddr,
  (char *)&yiaddr,
  sizeof(dhcp_message_repository->yiaddr));

  memset((char *)dhcp_message_repository->ciaddr, 0,
  	sizeof(dhcp_message_repository->ciaddr));
  memset((char *)dhcp_message_repository->siaddr, 0,
  	sizeof(dhcp_message_repository->siaddr));
  memset((char *)dhcp_message_repository->giaddr, 0,
  	sizeof(dhcp_message_repository->giaddr));
  memcpy((char *)dhcp_message_repository->chaddr, &dhcp_client_ethernet_address,
  	sizeof(dhcp_message_repository->chaddr));
  memset((char *)dhcp_message_repository->sname,  0,
  	sizeof(dhcp_message_repository->sname));
  memset((char *)dhcp_message_repository->file,   0,
  	sizeof(dhcp_message_repository->file));
  memset((char *)dhcp_message_repository->options, 0,
  	dhcp_message_total_options_lenth);
  memcpy((char *)dhcp_message_repository->options, (char *)dhcp_magic_cookie,
  	sizeof(dhcp_magic_cookie));
}

static void dhcps_send_nak();

/**
  * @brief  init and fill in  the needed content of dhcp offer message.  
  * @param  packet_buffer packet buffer for UDP.
  * @retval None.
  */
static void dhcps_send_offer()
{
	uint8_t temp_ip = 0;
  uint8_t search_ip = 0;

#if (!IS_USE_FIXED_IP)
	if ((ip4_addr4(&dhcps_allocated_client_address) != 0) &&
		(memcmp((void *)&dhcps_allocated_client_address, (void *)&client_request_ip, 4) == 0) &&
		(memcmp((void *)&bound_client_ethernet_address, (void *)&dhcp_client_ethernet_address, 16) == 0)) {
		temp_ip = (uint8_t) ip4_addr4(&client_request_ip);
	} else if ((ip4_addr1(&client_request_ip) == ip4_addr1(&dhcps_network_id)) &&
		(ip4_addr2(&client_request_ip) == ip4_addr2(&dhcps_network_id)) &&
		(ip4_addr3(&client_request_ip) == ip4_addr3(&dhcps_network_id))) {
		uint8_t request_ip4 = (uint8_t) ip4_addr4(&client_request_ip);
		if ((request_ip4 != 0) && (((request_ip4 - 1) / 32) >= 0) && (((request_ip4 - 1) / 32) <= 3) &&
			(((ip_table.ip_range[(request_ip4 - 1) / 32] >> ((request_ip4 - 1) % 32)) & 0x01) == 0)) {
			temp_ip = request_ip4;
		}
	}

	if (temp_ip == 0)
	{
		// The client Requested IP is 0.
		// Search MAC Address in a table
		search_ip = search_mac();
		if (search_ip != 0)
		{
			// This MAC Address has been marked.
			// So, it will offer the same IP which it marked before.
			temp_ip = search_ip;
		}
		else
		{
			// create new client ip.
			temp_ip = search_next_ip();
		}
	}
  else
  {
    if(1 == ismarked_ip_in_table(temp_ip))
    {
      // conflict ?
      if(ip_table.chaddr[temp_ip-1][0] == 0 && ip_table.chaddr[temp_ip-1][1] == 0 &&
        ip_table.chaddr[temp_ip-1][2] == 0 && ip_table.chaddr[temp_ip-1][3] == 0 &&
        ip_table.chaddr[temp_ip-1][4] == 0 && ip_table.chaddr[temp_ip-1][5] == 0 &&
        ip_table.chaddr[temp_ip-1][6] == 0 && ip_table.chaddr[temp_ip-1][7] == 0 &&
        ip_table.chaddr[temp_ip-1][8] == 0 && ip_table.chaddr[temp_ip-1][9] == 0 &&
        ip_table.chaddr[temp_ip-1][10] == 0 && ip_table.chaddr[temp_ip-1][11] == 0 &&
        ip_table.chaddr[temp_ip-1][12] == 0 && ip_table.chaddr[temp_ip-1][13] == 0 &&
        ip_table.chaddr[temp_ip-1][14] == 0 && ip_table.chaddr[temp_ip-1][15] == 0)
      {
        #if (debug_dhcps)
        printf("%d conflict temp_ip .%d\r\n", __LINE__, temp_ip);
        #endif
        temp_ip = search_next_ip();
        #if (debug_dhcps)
        printf("%d new temp_ip .%d\r\n", __LINE__, temp_ip);
        #endif
      }
    }
  }
  
  #if (debug_dhcps)
	printf("\r\n Offer .%d\r\n", temp_ip);
  #endif

	if (temp_ip == 0) {
#if 0	
	  	memset(&ip_table, 0, sizeof(struct table));
		mark_ip_in_table((uint8_t)ip4_addr4(&dhcps_local_address));
		printf("\r\n reset ip table!!\r\n");	
#endif	
		printf("\r\n No useable ip!!!!\r\n");
    #if 1
    dhcps_send_nak();
    return;
    #endif
	}
	
	IP4_ADDR(&dhcps_allocated_client_address, (ip4_addr1(&dhcps_network_id)),
		ip4_addr2(&dhcps_network_id), ip4_addr3(&dhcps_network_id), temp_ip);
	memcpy(bound_client_ethernet_address, dhcp_client_ethernet_address, sizeof(bound_client_ethernet_address));
#endif   
  dhcps_initialize_message(dhcp_message_repository, dhcps_allocated_client_address);
  add_offer_options(add_msg_type(&dhcp_message_repository->options[4], DHCP_MESSAGE_TYPE_OFFER));
  
	sock_sendto(DHCPs_SOCKET, (uint8_t *)dhcp_message_repository, sizeof(dhcps_msg), (uint8_t *)&dhcps_send_broadcast_address.addr, DHCP_CLIENT_PORT);
}

/**
  * @brief  init and fill in  the needed content of dhcp nak message.  
  * @param  packet buffer packet buffer for UDP.
  * @retval None.
  */
static void dhcps_send_nak()
{
	ip_addr zero_address;
	IP4_ADDR(&zero_address, 0, 0, 0, 0);

  dhcps_initialize_message(dhcp_message_repository, zero_address);
  add_msg_type(&dhcp_message_repository->options[4], DHCP_MESSAGE_TYPE_NAK);

  sock_sendto(DHCPs_SOCKET, (uint8_t *)dhcp_message_repository, sizeof(dhcps_msg), (uint8_t *)&dhcps_send_broadcast_address.addr, DHCP_CLIENT_PORT);
}

/**
  * @brief  init and fill in  the needed content of dhcp ack message.  
  * @param  packet buffer packet buffer for UDP.
  * @retval None.
  */
static void dhcps_send_reneal_ack()
{
  dhcps_initialize_message(dhcp_message_repository, dhcps_renewal_client_address);
  add_offer_options(add_msg_type(&dhcp_message_repository->options[4], DHCP_MESSAGE_TYPE_ACK));
  
  sock_sendto(DHCPs_SOCKET, (uint8_t *)dhcp_message_repository, sizeof(dhcps_msg), (uint8_t *)&dhcps_send_broadcast_address.addr, DHCP_CLIENT_PORT);
}


/**
  * @brief  init and fill in  the needed content of dhcp ack message.  
  * @param  packet buffer packet buffer for UDP.
  * @retval None.
  */
static void dhcps_send_ack()
{
  dhcps_initialize_message(dhcp_message_repository, dhcps_allocated_client_address);
  add_offer_options(add_msg_type(&dhcp_message_repository->options[4], DHCP_MESSAGE_TYPE_ACK));
  
  sock_sendto(DHCPs_SOCKET, (uint8_t *)dhcp_message_repository, sizeof(dhcps_msg), (uint8_t *)&dhcps_send_broadcast_address.addr, DHCP_CLIENT_PORT);
}

#if 1
// 20200608
void check_leased_time_over()
{
	// 20200605
	#if 1
	// 20200605
	uint8_t i, j;
	uint8_t mark_ip;
	uint32_t mark_time;
	uint32_t leased_time;
	#endif
    #if 1
    // 20201028 taylor
    uint32_t dhcps_tick_1sec_temp;
    #endif

	for(i=0; i<4; i++)
	{
	    dhcps_tick_1sec_temp = dhcps_tick_1sec;
		for(j=0; j<32; j++)
		{
			if(1<<j & ip_table.ip_range[i])
			{
				mark_ip = i*32 + (j+1);

				if(mark_ip != 1)
				{
					memcpy((uint8_t*)&mark_time, ip_table.cltime[mark_ip-1], sizeof(ip_table.cltime[mark_ip-1]));
                    #if 1
                    // 20201028 taylor
                    if(dhcps_tick_1sec_temp >= mark_time)
                    {
                        leased_time = dhcps_tick_1sec_temp - mark_time;
                        #if (debug_dhcps)
                        printf("dhcps_tick_1sec_temp >= mark_time : %#x(%d) = %#x(%d) - %#x(%d)\r\n", leased_time, leased_time, dhcps_tick_1sec_temp, dhcps_tick_1sec_temp, mark_time, mark_time);
                        #endif
                    }
                    else
                    {
                        leased_time = dhcps_tick_1sec_temp + ((uint32_t)0xffffffff - mark_time) + 1;
                        #if (debug_dhcps)
                        printf("dhcps_tick_1sec_temp < mark_time : %#x(%d) = %#x(%d) + %#x(%d) +1\r\n", leased_time, leased_time, dhcps_tick_1sec_temp, dhcps_tick_1sec_temp, ((uint32_t)0xffffffff - mark_time), ((uint32_t)0xffffffff - mark_time));
                        #endif
                    }
                    #else
					leased_time = dhcps_tick_1sec - mark_time;
                    #endif

					if(leased_time >= dhcps_option_lease_time)
					{
						printf("ip .%d leased time over %#x(%d) / %#x(%d)\r\n", mark_ip, leased_time, leased_time, dhcps_option_lease_time, dhcps_option_lease_time);
						unmark_ip_in_table(mark_ip);
						printf("unmarked\r\n");
					}
				}
			}
		}
	}
}
#endif

#ifndef W5500_WITH_A7
void dhcps_time_handler(void)
{
    dhcps_tick_1sec++;
#if (debug_dhcps)
    printf("dhcps_tick_1sec = %d\r\n", dhcps_tick_1sec);
#endif
    check_leased_time_over();
}
#endif

uint8_t dhcps_run(void)
{
	uint8_t client_addr[6];
	uint16_t client_port;
	uint16_t len;

	uint8_t * p;
	uint8_t * e;
	uint8_t type;
	uint8_t opt_len;

//	printf("%s(%d)\r\n", __FILE__, __LINE__);
	if(getSn_SR(DHCPs_SOCKET) != SOCK_UDP)
	   wiz_socket(DHCPs_SOCKET, Sn_MR_UDP, DHCP_SERVER_PORT, 0x00);

  if((len = getSn_RX_RSR(DHCPs_SOCKET)) > 0)
  {
    len = sock_recvfrom(DHCPs_SOCKET, (uint8_t *)dhcp_message_repository, len, client_addr, &client_port);
    printf("=========================================\r\n");
    printf("DHCP message : %d.%d.%d.%d(%d) %d received. \r\n", client_addr[0], client_addr[1], client_addr[2], client_addr[3], client_port, len);
  }
  else
  {
	#if 0
	// 20200605
	#if 1
	// 20200605
	uint8_t i, j;
	uint8_t mark_ip;
	uint32_t mark_time;
	uint32_t leased_time;
	#endif

	for(i=0; i<4; i++)
	{
		for(j=0; j<32; j++)
		{
			if(1<<j & ip_table.ip_range[i])
			{
				mark_ip = i*32 + (j+1);

				if(mark_ip != 1)
				{
					memcpy((uint8_t*)&mark_time, ip_table.cltime[mark_ip-1], sizeof(ip_table.cltime[mark_ip-1]));
					leased_time = dhcps_tick_1sec - mark_time;

					if(leased_time >= dhcps_option_lease_time)
					{
						printf("ip .%d leased time over %#x(%d) / %#x(%d)\r\n", mark_ip, leased_time, leased_time, dhcps_option_lease_time, dhcps_option_lease_time);
						// unmark_ip_in_table(mark_ip);
						printf("unmarked\r\n");
					}
				}
			}
		}
	}
	#endif
    return 0;
  }

  if(client_port == DHCP_CLIENT_PORT)
  {
#if 1
    // Reply type
    #if (debug_dhcps)
    printf("dhcp_message_repository->flags = %#x\r\n", dhcp_message_repository->flags);
    #endif

    if(dhcp_message_repository->flags & 1<<7)
    {
      // Broadcast
      #if (debug_dhcps)
      printf("DHCP Server Reply to Broadcast\r\n");
      #endif
      //IP4_ADDR(&dhcps_send_broadcast_address, 255, 255, 255, 255);
    }
    else
    {
      // Unicast
      #if (debug_dhcps)
      printf("DHCP Server Reply to Unicast %d.%d.%d.%d\r\n", dhcp_message_repository->ciaddr[0], dhcp_message_repository->ciaddr[1], dhcp_message_repository->ciaddr[2], dhcp_message_repository->ciaddr[3]);
      #endif
	    if(dhcp_message_repository->ciaddr[0] != 0
        && dhcp_message_repository->ciaddr[1] != 0
        && dhcp_message_repository->ciaddr[2] != 0
        && dhcp_message_repository->ciaddr[3] != 0
        )
      { 
        memcpy(&dhcps_send_broadcast_address, &dhcp_message_repository->ciaddr, sizeof(dhcps_send_broadcast_address));
      }
    }        
#endif
    switch (dhcps_check_msg_and_handle_options(len))
    {
  		case  DHCP_SERVER_STATE_OFFER:
			printf("DHCP_SERVER_STATE_OFFER\r\n");
  			dhcps_send_offer();
  			break;
  		case DHCP_SERVER_STATE_ACK:
			printf("DHCP_SERVER_STATE_ACK\r\n");
  			dhcps_send_ack();
#if (!IS_USE_FIXED_IP)
  			mark_ip_in_table((uint8_t)ip4_addr4(&dhcps_allocated_client_address)); 			
#endif
  			dhcp_server_state_machine = DHCP_SERVER_STATE_IDLE;
  			break;
  		case DHCP_SERVER_STATE_NAK:
			printf("DHCP_SERVER_STATE_NAK\r\n");
  			dhcps_send_nak();
  			dhcp_server_state_machine = DHCP_SERVER_STATE_IDLE;
  			break;
  		case DHCP_OPTION_CODE_END:
			printf("DHCP_OPTION_CODE_END\r\n");
  			break;
		case DHCP_SERVER_STATE_RELEASE:
      printf("DHCP_SERVER_STATE_RELEASE\r\n");
	  		#if 1
			// 20200605
			unmark_ip_in_table(search_mac());
	  		#else
			unmark_ip_in_table();
			#endif
			dhcp_server_state_machine = DHCP_SERVER_STATE_IDLE;
        break;
		case DHCP_SERVER_STATE_RENEWAL:
			printf("DHCP_SERVER_STATE_RENEWAL\r\n");
      dhcps_send_reneal_ack();
#if (!IS_USE_FIXED_IP)
      mark_ip_in_table((uint8_t)ip4_addr4(&dhcps_renewal_client_address));
#endif
      dhcp_server_state_machine = DHCP_SERVER_STATE_IDLE;
        break;
    case DHCP_SERVER_STATE_DECLINE:
      printf("DHCP_SERVER_STATE_DECLINE\r\n");
      dhcp_server_state_machine = DHCP_SERVER_STATE_IDLE;
        break;
		}
    IP4_ADDR(&dhcps_send_broadcast_address, 255, 255, 255, 255);
    printf("=========================================\r\n\r\n");
  }

  return 0;

}


