/**************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation 
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met: 
 *
 * - Redistributions of source code must retain the above copyright notice, 
 * this list of conditions and the following disclaimer. 
 * - Redistributions in binary form must reproduce the above copyright notice, 
 * this list of conditions and the following disclaimer in the documentation 
 * and/or other materials provided with the distribution. 
 * - Neither name of Intel Corporation nor the names of its contributors 
 * may be used to endorse or promote products derived from this software 
 * without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR 
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY 
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 **************************************************************************/

#ifndef SSDPLIB_H
#define SSDPLIB_H 

#include "httpparser.h"
#include "httpreadwrite.h"
#include "miniserver.h"
#include "UpnpInet.h"


#include <sys/types.h>
#include <signal.h>
#include <setjmp.h>
#include <fcntl.h>
#include <errno.h>


#ifdef WIN32
#else /* WIN32 */
	#include <syslog.h>
	#include <sys/socket.h>
	#ifndef __APPLE__
		#include <netinet/in_systm.h>
		#include <netinet/ip.h>
		#include <netinet/ip_icmp.h>
	#endif /* __APPLE__ */
	#include <sys/time.h>
	#include <arpa/inet.h>
#endif /* WIN32 */


/* Enumeration to define all different types of ssdp searches */
typedef enum SsdpSearchType{
	SSDP_SERROR=-1,
	SSDP_ALL,SSDP_ROOTDEVICE,
	SSDP_DEVICEUDN,
	SSDP_DEVICETYPE,
	SSDP_SERVICE
} SType;


/* Enumeration to define all different type of ssdp messages */
typedef enum SsdpCmdType{
	SSDP_ERROR=-1,
	SSDP_OK,
	SSDP_ALIVE,
	SSDP_BYEBYE,
	SSDP_SEARCH,
	SSDP_NOTIFY,
	SSDP_TIMEOUT
} Cmd;



/* Constant */
#define BUFSIZE   2500
#define SSDP_IP   "239.255.255.250"
#define SSDP_IPV6_LINKLOCAL "FF02::C"
#define SSDP_IPV6_SITELOCAL "FF05::C"
#define SSDP_PORT 1900
#define NUM_TRY 3
#define THREAD_LIMIT 50
#define COMMAND_LEN  300

/* can be overwritten by configure CFLAGS argument */
#ifndef X_USER_AGENT
/** @name X_USER_AGENT
 *  The {\tt X_USER_AGENT} constant specifies the value of the X-User-Agent:
 *  HTTP header. The value "redsonic" is needed for the DSM-320. See
 *  https://sourceforge.net/forum/message.php?msg_id=3166856 for more
 * information
 */ 
 #define X_USER_AGENT "redsonic"
#endif

/* Error code */
#define NO_ERROR_FOUND    0
#define E_REQUEST_INVALID  	-3
#define E_RES_EXPIRED		-4
#define E_MEM_ALLOC		-5
#define E_HTTP_SYNTEX		-6
#define E_SOCKET 		-7
#define RQST_TIMEOUT    20

/*! Structure to store the SSDP information */
typedef struct SsdpEventStruct {
  enum SsdpCmdType Cmd;
  enum SsdpSearchType RequestType;
  int  ErrCode;
  int  MaxAge;
  int  Mx;
  char UDN[LINE_SIZE];
  char DeviceType[LINE_SIZE];
  /* NT or ST */
  char ServiceType[LINE_SIZE];
  char Location[LINE_SIZE];
  char HostAddr[LINE_SIZE];
  char Os[LINE_SIZE];
  char Ext[LINE_SIZE];
  char Date[LINE_SIZE];
  struct sockaddr *DestAddr;
  void * Cookie;
} Event;

typedef void (* SsdpFunPtr)(Event *);

typedef Event SsdpEvent ;

/*! Structure to contain Discovery response. */
typedef struct resultData
{
   struct Upnp_Discovery param;
   void *cookie;
   Upnp_FunPtr ctrlpt_callback;
} ResultData;


typedef struct TData
{
   int Mx;
   void * Cookie;
   char * Data;
   struct sockaddr_storage DestAddr;
   
} ThreadData;

typedef struct ssdpsearchreply
{
  int MaxAge;
  UpnpDevice_Handle handle;
  struct sockaddr_storage dest_addr;
  SsdpEvent event;
  
} SsdpSearchReply;

typedef struct ssdpsearcharg
{
  int timeoutEventId;
  char * searchTarget;
  void *cookie;
  enum SsdpSearchType requestType;
} SsdpSearchArg;


typedef struct 
{
  http_parser_t parser;
  struct sockaddr_storage dest_addr;
} ssdp_thread_data;


/* globals */

#ifdef INCLUDE_CLIENT_APIS
	extern SOCKET gSsdpReqSocket4;
	#ifdef UPNP_ENABLE_IPV6
		extern SOCKET gSsdpReqSocket6;
	#endif /* UPNP_ENABLE_IPV6 */
#endif /* INCLUDE_CLIENT_APIS */
typedef int (*ParserFun)(char *, Event *);


/************************************************************************
* Function : Make_Socket_NoBlocking
*
* Parameters:
*	IN int sock: socket
*
* Description:
*	This function to make ssdp socket non-blocking.
*
* Returns: int
*	0 if successful else -1 
***************************************************************************/
int Make_Socket_NoBlocking (int sock);

/************************************************************************
* Function : ssdp_handle_device_request
*
* Parameters:
*		IN void *data:
*
* Description:
*	This function handles the search request. It do the sanity checks of
*	the request and then schedules a thread to send a random time reply (
*	random within maximum time given by the control point to reply).
*
* Returns: void *
*	1 if successful else appropriate error
***************************************************************************/
#ifdef INCLUDE_DEVICE_APIS
void ssdp_handle_device_request(
	IN http_message_t *hmsg, 
	IN struct sockaddr *dest_addr);
#else
static inline void ssdp_handle_device_request(
	IN http_message_t *hmsg, 
	IN struct sockaddr* dest_addr) {}
#endif

/************************************************************************
* Function : ssdp_handle_ctrlpt_msg
*
* Parameters:
*	IN http_message_t* hmsg: SSDP message from the device
*	IN struct sockaddr* dest_addr: Address of the device
*	IN int timeout: timeout kept by the control point while sending 
*		search message
*	IN void* cookie: Cookie stored by the control point application. 
*		This cookie will be returned to the control point
*		in the callback
*
* Description:
*	This function handles the ssdp messages from the devices. These 
*	messages includes the search replies, advertisement of device coming 
*	alive and bye byes.
*
* Returns: void
*
***************************************************************************/
void ssdp_handle_ctrlpt_msg(
	IN http_message_t *hmsg, 
	IN struct sockaddr *dest_addr,
	IN int timeout,
	IN void *cookie);

/************************************************************************
* Function : unique_service_name
*
* Parameters:
*	IN char *cmd: Service Name string
*	OUT SsdpEvent *Evt: The SSDP event structure partially filled
*		by all the function.
*
* Description:
*	This function fills the fields of the event structure like DeviceType,
*	Device UDN and Service Type
*
* Returns: int
*	0 if successful else -1 
***************************************************************************/
int unique_service_name(char * cmd, SsdpEvent * Evt);


/************************************************************************
* Function : get_ssdp_sockets
*
* Parameters:
*	OUT MiniServerSockArray *out: Arrays of SSDP sockets
*
* Description:
*	This function creates the ssdp sockets. It set their option to listen 
*	for multicast traffic.
*
* Returns: int
*	return UPNP_E_SUCCESS if successful else returns appropriate error
***************************************************************************/
int get_ssdp_sockets(MiniServerSockArray *out);


/************************************************************************
* Function : readFromSSDPSocket	
*
* Parameters:
*	IN SOCKET socket: SSDP socket
*
* Description:
*	This function reads the data from the ssdp socket.
*
* Returns: void
*	
***************************************************************************/
void readFromSSDPSocket(SOCKET socket);


/************************************************************************
* Function : ssdp_request_type1
*
* Parameters:
*	IN char *cmd: command came in the ssdp request
*
* Description:
*	This function figures out the type of the SSDP search in the
*	in the request.
*
* Returns: enum SsdpSearchType
*	return appropriate search type else returns SSDP_ERROR
***************************************************************************/
enum SsdpSearchType ssdp_request_type1(IN char *cmd);


/************************************************************************
* Function : ssdp_request_type
*
* Parameters:
*	IN char *cmd: command came in the ssdp request
*	OUT SsdpEvent *Evt: The event structure partially filled by
*		 this function.
*
* Description:
*	This function starts filling the SSDP event structure based upon the 
*	request received. 
*
* Returns: int
*	0 on success; -1 on error
***************************************************************************/
int ssdp_request_type(IN char * cmd, OUT SsdpEvent * Evt);


/************************************************************************
* Function : SearchByTarget
*
* Parameters:
*	IN int Mx:Number of seconds to wait, to collect all the	responses.
*	char *St: Search target.
*	void *Cookie: cookie provided by control point application. This
*		cokie will be returned to application in the callback.
*
* Description:
*	This function creates and send the search request for a specific URL.
*
* Returns: int
*	1 if successful else appropriate error
***************************************************************************/
int SearchByTarget(IN int Mx, IN char *St, IN void *Cookie);

/************************************************************************
* Function : DeviceAdvertisement
*
* Parameters:
*	IN char *DevType : type of the device
*	IN int RootDev   : flag to indicate if the device is root device
*	IN char *Udn     :
*	IN char *Location: Location URL.
*	IN int Duration  : Service duration in sec.
*	IN int AddressFamily: Device address family.
*
* Description:
*	This function creates the device advertisement request based on
*	the input parameter, and send it to the multicast channel.
*
* Returns: int
*	UPNP_E_SUCCESS if successful else appropriate error
***************************************************************************/
int DeviceAdvertisement(
	IN char *DevType,
	IN int RootDev,
	IN char *Udn, 
	IN char *Location,
	IN int Duration,
	IN int AddressFamily);


/************************************************************************
* Function : DeviceShutdown
*
* Parameters:	
*	IN char *DevType: Device Type.
*	IN int RootDev:1 means root device.
*	IN char *Udn: Device UDN
*	IN char *_Server:
*	IN char *Location: Location URL
*	IN int Duration :Device duration in sec.
*	IN int AddressFamily: Device address family.
*
* Description:
*	This function creates a HTTP device shutdown request packet 
*	and sent it to the multicast channel through RequestHandler.
*
* Returns: int
*	UPNP_E_SUCCESS if successful else appropriate error
***************************************************************************/
int DeviceShutdown(
	IN char *DevType, 
	IN int RootDev,
	IN char *Udn, 
	IN char *_Server, 
	IN char *Location, 
	IN int Duration,
	IN int AddressFamily);

/************************************************************************
* Function : DeviceReply
*
* Parameters:	
*	IN struct sockaddr *DestAddr: destination IP address.
*	IN char *DevType: Device type
*	IN int RootDev: 1 means root device 0 means embedded device.
*	IN char *Udn: Device UDN
*	IN char *Location: Location of Device description document.
*	IN int Duration :Life time of this device.
*
* Description:
*	This function creates the reply packet based on the input parameter, 
*	and send it to the client address given in its input parameter DestAddr.
*
* Returns: int
*	UPNP_E_SUCCESS if successful else appropriate error
***************************************************************************/
int DeviceReply(
	IN struct sockaddr *DestAddr, 
	IN char *DevType, 
	IN int RootDev, 
	IN char *Udn, 
	IN char *Location, 
	IN int  Duration);

/************************************************************************
* Function : SendReply
*
* Parameters:	
*	IN struct sockaddr *DestAddr: destination IP address.
*	IN char *DevType: Device type
*	IN int RootDev: 1 means root device 0 means embedded device.
*	IN char * Udn: Device UDN
*	IN char *_Server:
*	IN char *Location: Location of Device description document.
*	IN int Duration :Life time of this device.
*	IN int ByType:
*
* Description:
*	This function creates the reply packet based on the input parameter, 
*	and send it to the client addesss given in its input parameter DestAddr.
*
* Returns: int
*	UPNP_E_SUCCESS if successful else appropriate error
***************************************************************************/
int SendReply(
	IN struct sockaddr *DestAddr, 
	IN char *DevType, 
	IN int RootDev, 
	IN char *Udn, 
	IN char *Location, 
	IN int Duration, 
	IN int ByType );

/************************************************************************
* Function : ServiceAdvertisement
*
* Parameters:	
*	IN char * Udn: Device UDN
*	IN char *ServType: Service Type.
*	IN char * Location: Location of Device description document.
*	IN int Duration: Life time of this device.
*	IN int AddressFamily: Device address family
*
* Description:
*	This function creates the advertisement packet based 
*	on the input parameter, and send it to the multicast channel.
*
* Returns: int
*	UPNP_E_SUCCESS if successful else appropriate error
***************************************************************************/
int ServiceAdvertisement(
	IN char *Udn, 
	IN char *ServType,
	IN char *Location,
	IN int Duration,
	IN int AddressFamily);

/************************************************************************
* Function : ServiceReply
*
* Parameters:	
*	IN struct sockaddr *DestAddr:
*	IN char *Udn: Device UDN
*	IN char *ServType: Service Type.
*	IN char *Server: Not used
*	IN char *Location: Location of Device description document.
*	IN int Duration :Life time of this device.
*
* Description:
*	This function creates the advertisement packet based 
*	on the input parameter, and send it to the multicast channel.
*
* Returns: int
*	UPNP_E_SUCCESS if successful else appropriate error
***************************************************************************/
int ServiceReply(
	IN struct sockaddr *DestAddr,  
	IN char *ServType, 
	IN char *Udn, 
	IN char *Location,
	IN int Duration);

/************************************************************************
* Function : ServiceShutdown
*
* Parameters:
*	IN char *Udn: Device UDN
*	IN char *ServType: Service Type.
*	IN char *Location: Location of Device description document.
*	IN int Duration :Service duration in sec.
*	IN int AddressFamily: Device address family
*
* Description:
*	This function creates a HTTP service shutdown request packet 
*	and sent it to the multicast channel through RequestHandler.
*
* Returns: int
*	UPNP_E_SUCCESS if successful else appropriate error
***************************************************************************/
int ServiceShutdown(
	IN char *Udn,
	IN char *ServType,
	IN char *Location,
	IN int Duration,
	IN int AddressFamily);


/************************************************************************
* Function : advertiseAndReplyThread
*
* Parameters:
*	IN void *data: Structure containing the search request
*
* Description:
*	This function is a wrapper function to reply the search request 
*	coming from the control point.
*
* Returns: void *
*	always return NULL
***************************************************************************/
void *advertiseAndReplyThread(IN void * data);

/************************************************************************
* Function : AdvertiseAndReply
*
* Parameters:
*	IN int AdFlag: -1 = Send shutdown,
*			0 = send reply, 
*			1 = Send Advertisement
*	IN UpnpDevice_Handle Hnd: Device handle
*	IN enum SsdpSearchType SearchType:Search type for sending replies
*	IN struct sockaddr *DestAddr:Destination address
*	IN char *DeviceType:Device type
*	IN char *DeviceUDN:Device UDN
*	IN char *ServiceType:Service type
*	IN int Exp:Advertisement age
*
* Description:
*	This function to send SSDP advertisements, replies and shutdown messages.
*
* Returns: int
*	UPNP_E_SUCCESS if successful else appropriate error
***************************************************************************/
int AdvertiseAndReply(
	IN int AdFlag, 
	IN UpnpDevice_Handle Hnd, 
	IN enum SsdpSearchType SearchType, 
	IN struct sockaddr *DestAddr,
	IN char *DeviceType, 
	IN char *DeviceUDN, 
	IN char *ServiceType, int Exp);

#endif /* SSDPLIB_H */

