#ifndef __SMPLETCP_H__
#define __SMPLETCP_H__

#pragma pack(push,1)      // x86, MS compiler; MIPS, MIPS compiler

//////////////////////////////////////////////////////////////////////
// PCAUSA Notes
// ------------
// This header is an extended version of the SMPLETCP.H header provided
// in the Windows NT DDK. It includes the information from the original
// header plus selected extracts from other downloadable headers that
// Microsoft prvides on the internet for developers.
//
//   smpletcp.h - Windows NT 4.0 DDK
//   ipexport.h - Windows 95 WSHELPER download
//   ipinfo.h   - Windows 95 WSHELPER download
//   llinfo.h   - Windows 95 WSHELPER download
//

/********************************************************************/
/**               Copyright(c) Microsoft Corp., 1990-1996          **/
/********************************************************************/
//
//  This file contains public definitions exported to transport layer and
//  application software for TCP/IP.
//

//
// IP type definitions.
//
typedef unsigned long   IPAddr;     // An IP address.
typedef unsigned long   IPMask;     // An IP subnet mask.
typedef unsigned long   IP_STATUS;  // Status code returned from IP APIs.

//
// The ip_option_information structure describes the options to be
// included in the header of an IP packet. The TTL, TOS, and Flags
// values are carried in specific fields in the header. The OptionsData
// bytes are carried in the options area following the standard IP header.
// With the exception of source route options, this data must be in the
// format to be transmitted on the wire as specified in RFC 791. A source
// route option should contain the full route - first hop thru final
// destination - in the route data. The first hop will be pulled out of the
// data and the option will be reformatted accordingly. Otherwise, the route
// option should be formatted as specified in RFC 791.
//

struct ip_option_information {
    unsigned char      Ttl;             // Time To Live
    unsigned char      Tos;             // Type Of Service
    unsigned char      Flags;           // IP header flags
    unsigned char      OptionsSize;     // Size in bytes of options data
    unsigned char      *OptionsData;     // Pointer to options data
}; /* ip_option_information */


#define MAX_OPT_SIZE    40         // Maximum length of IP options in bytes

#define TCP_SOCKET_NODELAY      1
#define TCP_SOCKET_KEEPALIVE    2
#define TCP_SOCKET_OOBINLINE    3
#define TCP_SOCKET_BSDURGENT    4
#define TCP_SOCKET_ATMARK       5
#define TCP_SOCKET_WINDOW       6

//
// Address object identifiers.  All but AO_OPTION_MCASTIF take single boolean
// character value.  That one expects a pointer to an IP address.
//
#define AO_OPTION_TTL              1
#define AO_OPTION_MCASTTTL         2
#define AO_OPTION_MCASTIF          3
#define AO_OPTION_XSUM             4
#define AO_OPTION_IPOPTIONS        5
#define AO_OPTION_ADD_MCAST        6
#define AO_OPTION_DEL_MCAST        7
#define AO_OPTION_TOS              8
#define AO_OPTION_IP_DONTFRAGMENT  9

// See "New" IP Helper MIB_IPSTATS For Description
typedef struct IPSNMPInfo {
	ULONG		ipsi_forwarding;
	ULONG		ipsi_defaultttl;
	ULONG		ipsi_inreceives;
	ULONG		ipsi_inhdrerrors;
	ULONG		ipsi_inaddrerrors;
	ULONG		ipsi_forwdatagrams;
	ULONG		ipsi_inunknownprotos;
	ULONG		ipsi_indiscards;
	ULONG		ipsi_indelivers;												
	ULONG		ipsi_outrequests;
	ULONG		ipsi_routingdiscards;
	ULONG		ipsi_outdiscards;
	ULONG		ipsi_outnoroutes;
	ULONG		ipsi_reasmtimeout;
	ULONG		ipsi_reasmreqds;
	ULONG		ipsi_reasmoks;
	ULONG		ipsi_reasmfails;
	ULONG		ipsi_fragoks;
	ULONG		ipsi_fragfails;
	ULONG		ipsi_fragcreates;
	ULONG		ipsi_numif;
	ULONG		ipsi_numaddr;
	ULONG		ipsi_numroutes;
} IPSNMPInfo;

// See "New" IP Helper MIB_IPADDROW For Description
typedef struct IPAddrEntry {
	ULONG		iae_addr;
	ULONG		iae_index;
	ULONG		iae_mask;
	ULONG		iae_bcastaddr;
	ULONG		iae_reasmsize;
	USHORT		iae_context;
	USHORT		iae_pad;
} IPAddrEntry;

typedef struct IPRouteEntry {
	ULONG		ire_dest;
	ULONG		ire_index;
	ULONG		ire_metric1;
	ULONG		ire_metric2;
	ULONG		ire_metric3;
	ULONG		ire_metric4;
	ULONG		ire_nexthop;
	ULONG		ire_type;
	ULONG		ire_proto;
	ULONG		ire_age;
	ULONG		ire_mask;
	ULONG		ire_metric5;
   ULONG    ire_unk8;   //??   // FOR_NT
} IPRouteEntry;

#define	IRE_TYPE_OTHER		1
#define	IRE_TYPE_INVALID	2
#define	IRE_TYPE_DIRECT		3
#define	IRE_TYPE_INDIRECT	4

#define	IRE_PROTO_OTHER		1
#define	IRE_PROTO_LOCAL		2
#define	IRE_PROTO_NETMGMT	3
#define	IRE_PROTO_ICMP		4
#define	IRE_PROTO_EGP		5
#define	IRE_PROTO_GGP		6
#define	IRE_PROTO_HELLO		7
#define	IRE_PROTO_RIP		8
#define	IRE_PROTO_IS_IS		9
#define	IRE_PROTO_ES_IS		10
#define	IRE_PROTO_CISCO		11
#define	IRE_PROTO_BBN		12
#define	IRE_PROTO_OSPF		13
#define	IRE_PROTO_BGP		14

#define	IRE_METRIC_UNUSED	0xffffffff

#define	IP_MIB_STATS_ID					1
#define	ICMP_MIB_STATS_ID				1

#define  AT_ADDRXLAT_INFO_ID           1
#define	AT_MIB_ADDRXLAT_ENTRY_ID      0x101

#define	IP_MIB_RTTABLE_ENTRY_ID			0x101
#define	IP_MIB_ADDRTABLE_ENTRY_ID		0x102

#define IP_INTFC_FLAG_P2P   1

typedef struct IPInterfaceInfo {
    ULONG       iii_flags;
    ULONG       iii_mtu;
    ULONG       iii_speed;
    ULONG       iii_addrlength;
    UCHAR       iii_addr[1];
} IPInterfaceInfo;

#define IP_INTFC_INFO_ID                0x103

#define	IF_MIB_STATS_ID		1

#define	MAX_PHYSADDR_SIZE       8     // MAX_ADAPTER_ADDRESS_LENGTH
#define	MAX_IFDESCR_LEN			256   // MAX_ADAPTER_DESCRIPTION_LENGTH

#define	IFE_FIXED_SIZE	offsetof(struct IFEntry, if_descr)

typedef struct IFEntry {
	ULONG			if_index;
	ULONG			if_type;
	ULONG			if_mtu;
	ULONG			if_speed;
	ULONG			if_physaddrlen;
	UCHAR			if_physaddr[MAX_PHYSADDR_SIZE];
	ULONG			if_adminstatus;
	ULONG			if_operstatus;
	ULONG			if_lastchange;
	ULONG			if_inoctets;
	ULONG			if_inucastpkts;
	ULONG			if_innucastpkts;
	ULONG			if_indiscards;
	ULONG			if_inerrors;
	ULONG			if_inunknownprotos;
	ULONG			if_outoctets;
	ULONG			if_outucastpkts;
	ULONG			if_outnucastpkts;
	ULONG			if_outdiscards;
	ULONG			if_outerrors;
	ULONG			if_outqlen;
	ULONG			if_descrlen;
	UCHAR			if_descr[1];
} IFEntry;

// 1992 Windows 95 LLINFO.H         // 1999 IPIFCONS.H
#define	IF_TYPE_OTHER	  	1        // MIB_IF_TYPE_OTHER
#define	IF_TYPE_ETHERNET	6        // MIB_IF_TYPE_ETHERNET
#define	IF_TYPE_TOKENRING	9        // MIB_IF_TYPE_TOKENRING
#define	IF_TYPE_FDDI		15       // MIB_IF_TYPE_FDDI
#define	IF_TYPE_PPP			23       // MIB_IF_TYPE_PPP
#define	IF_TYPE_LOOPBACK	24       // MIB_IF_TYPE_LOOPBACK
#define	IF_TYPE_SLIP		28       // MIB_IF_TYPE_SLIP

#define	IF_STATUS_UP		1        // MIB_IF_ADMIN_STATUS_UP
#define	IF_STATUS_DOWN		2        // MIB_IF_ADMIN_STATUS_DOWN
#define	IF_STATUS_TESTING	3        // MIB_IF_ADMIN_STATUS_TESTING

//
// Device Name - this string is the name of the device.  It is the name
// that should be passed to CreateFile when accessing the device.
//
#define DD_TCP_DEVICE_NAME      L"\\Device\\Tcp"
#define DD_UDP_DEVICE_NAME      L"\\Device\\Udp"
#define DD_RAW_IP_DEVICE_NAME   L"\\Device\\RawIp"

#define FSCTL_TCP_BASE     FILE_DEVICE_NETWORK

#define _TCP_CTL_CODE(function, method, access) \
            CTL_CODE(FSCTL_TCP_BASE, function, method, access)

#define IOCTL_TCP_QUERY_INFORMATION_EX  \
            _TCP_CTL_CODE(0, METHOD_NEITHER, FILE_ANY_ACCESS)

#define IOCTL_TCP_SET_INFORMATION_EX  \
            _TCP_CTL_CODE(1, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#pragma pack(pop)                  // x86, MS compiler; MIPS, MIPS compiler

#endif // __SMPLETCP_H__

