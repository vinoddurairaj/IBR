                           TDI Query (TDIQ) Test Tool
                              Version V1.00.00.01
                               February 17, 2000

             Copyright (c) 2000 Printing Communications Assoc., Inc.
                                  - PCAUSA -

                      Printing Communications Assoc., Inc.
                             4201 Brunswick Court
                             Smyrna, GA 30080 USA

                             TEL: (770) 432-4580
                             FAX: (770) 436-7246
                           E-Mail: tdiq@pcausa.com

TDIQ OVERVIEW
=============
The TDI Query (TDIQ) Test Tool is a test application developed in
support of the PCAUSA TDI Client 2000 samples. It's purpose is to
explore the use of extended TDI queries using IOCTL_TCP_QUERY_INFORMATION_EX
on the Windows NT platform from the safety and comfort of a Win32
application.

An unexpected fallout from this development is an application which
can query IP configuration, route and statistics information.

The information that can be queried by TDIQ is similar to that
provided by the "new" IPHelper library.

However, unlike IPHelper, TDIQ works on ALL current Windows platforms
from the original Windows 95 through Windows 2000. It does this
without the suport of IPHelper functions or WSAIoctl functions, and
does not require Winsock 2 support. (Winsock 1.1 is used primarily
for address-to-string conversion routines...).

TDIQ does NOT support setting of information at this time.

TDIQ is a command-line executable. It can display three categories of
information selected by command-line options:

  IP Configuration Information - Similar to IPCONFIG. Enter:
     C:> tdiq -ipconfig

  IP Route Information - Similar to ROUTE PRINT. Enter
     C:> tdiq -iproute

  IP Statistics Information - Similar to IpHelper IPSTAT. Enter:
     C:> tdiq -ipstat

  All Available Information. Enter:
     C:> tdiq -all

Here is sample output from TDIQ from a Windows NT platform:


TDIQ IP Configuration Information Sample Output
===============================================
C:> tdiq -ipconfig
TDI Query (TDIQ) Test Tool for Windows NT - V1.00.00.01
Copyright (c) 2000 Printing Communications Assoc., Inc (PCAUSA)

IP Configuration:
Ethernet Adapter:

  Description..........: 3Com 3C90x Ethernet Adapter
  Physical Address.....: 00-60-08-A3-29-D0
  IP Address...........: 172.16.1.130
  Subnet Mask..........: 255.255.255.0
  Default Gateway......: 172.16.1.1
  Speed................: 100000000 bits/second
  MTU (Local Nodes)....: 1480 bytes

Ethernet Adapter:

  Description..........:  Intel EtherExpress PRO PCI Adapter
  Physical Address.....: 00-90-27-8C-B4-92
  IP Address...........: 172.16.3.33
  Subnet Mask..........: 255.255.255.0
  Default Gateway......: 172.16.3.33
  Speed................: 10000000 bits/second
  MTU (Local Nodes)....: 1480 bytes

Ethernet Adapter:

  Description..........:  Intel EtherExpress PRO PCI Adapter
  Physical Address.....: 00-90-27-8C-B4-91
  IP Address...........: 172.16.2.33
  Subnet Mask..........: 255.255.255.0
  Default Gateway......: 172.16.2.33
  Speed................: 100000000 bits/second
  MTU (Local Nodes)....: 1480 bytes

Loopback Adapter:

  Description..........:  MS TCP Loopback interface
  Physical Address.....: 
  IP Address...........: 127.0.0.1
  Subnet Mask..........: 255.0.0.0
  Default Gateway......: 127.0.0.1
  Speed................: 10000000 bits/second
  MTU (Local Nodes)....: 1480 bytes


TDIQ IP Route Information Sample Output
=======================================
C:> tdiq -iproute
TDI Query (TDIQ) Test Tool for Windows NT - V1.00.00.01
Copyright (c) 2000 Printing Communications Assoc., Inc (PCAUSA)

==========================================================================
Interface List:
0x1 ...........................  MS TCP Loopback interface
0x2 ...00-90-27-8C-B4-91.......  Intel EtherExpress PRO PCI Adapter
0x3 ...00-90-27-8C-B4-92.......  Intel EtherExpress PRO PCI Adapter
0x4 ...00-60-08-A3-29-D0....... 3Com 3C90x Ethernet Adapter
==========================================================================
==========================================================================
Active Routes:
Network Address          Netmask  Gateway Address        Interface  Metric
    172.16.3.33  255.255.255.255        127.0.0.1        127.0.0.1       1
      224.0.0.0        224.0.0.0      172.16.2.33      172.16.2.33       1
      224.0.0.0        224.0.0.0      172.16.3.33      172.16.3.33       1
      224.0.0.0        224.0.0.0     172.16.1.130     172.16.1.130       1
        0.0.0.0          0.0.0.0       172.16.1.1     172.16.1.130       1
 172.16.255.255  255.255.255.255      172.16.2.33      172.16.2.33       1
255.255.255.255  255.255.255.255      172.16.2.33      172.16.2.33       1
     172.16.2.0    255.255.255.0      172.16.2.33      172.16.2.33       1
     172.16.3.0    255.255.255.0      172.16.3.33      172.16.3.33       1
     172.16.1.0    255.255.255.0     172.16.1.130     172.16.1.130       1
    172.16.2.33  255.255.255.255        127.0.0.1        127.0.0.1       1
   172.16.1.130  255.255.255.255        127.0.0.1        127.0.0.1       1
      127.0.0.0        255.0.0.0        127.0.0.1        127.0.0.1       1
==========================================================================

TDIQ IP Statistics Sample Output
================================
C:> tdiq -ipstat
TDI Query (TDIQ) Test Tool for Windows NT - V1.00.00.01
Copyright (c) 2000 Printing Communications Assoc., Inc (PCAUSA)

IP Statistics:
  dwForwarding       = 2
  dwDefaultTTL       = 128
  dwInReceives       = 6497
  dwInHdrErrors      = 0
  dwInAddrErrors     = 0
  dwForwDatagrams    = 0
  dwInUnknownProtos  = 0
  dwInDiscards       = 0
  dwInDelivers       = 6497
  dwOutRequests      = 3138
  dwRoutingDiscards  = 0
  dwOutDiscards      = 0
  dwOutNoRoutes      = 0
  dwReasmTimeout     = 60
  dwReasmReqds       = 0
  dwReasmOks         = 0
  dwReasmFails       = 0
  dwFragOks          = 0
  dwFragFails        = 0
  dwFragCreates      = 0
  dwNumIf            = 4
  dwNumAddr          = 4
  dwNumRoutes        = 13

TDIQ Source Code
================
Sources to the TDI Query (TDIQ) Test Tool are included in the PCAUSA TDI
Client 2000 samples.


Building TDIQ
=============
TDIQ sources are provided as a Microsoft Visual C++ 5.0 project. This
is easily concerted to a Microsoft Visual C++ 6.0 project when the
.DSP file is opened.

Building TDIQ requires some header files from the Windows NT 4.0 DDK.
You will probably need to modify the Visual C++ Additional include
directory settings for the C/C++ Preprocessor to reflect the location
of your DDK. Be sure to make this change for "All configurations".



