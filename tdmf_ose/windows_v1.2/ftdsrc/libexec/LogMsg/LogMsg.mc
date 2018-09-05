;/*++

;Module Name:

;

;    LogMsg.h (generated by LogMsg.mc)

;

;Abstract:

;

;    This module contains the error strings for the event-logging system

;

;Environment:

;

;    Kernel\User mode

;

;Revision History:

;

;--*/

;

;#ifndef _LOGMSG_H_

;#define _LOGMSG_H_

;

;//

;//  Status values are 32 bit values layed out as follows:

;//

;//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1

;//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0

;//  +---+-+-------------------------+-------------------------------+

;//  |Sev|C|       Facility          |               Code            |

;//  +---+-+-------------------------+-------------------------------+

;//

;//  where

;//

;//      Sev - is the severity code

;//

;//          00 - Success

;//          01 - Informational

;//          10 - Warning

;//          11 - Error

;//

;//      C - is the Customer code flag

;//

;//      Facility - is the facility code

;//

;//      Code - is the facility's status code

;//

;

;//

;//  Values are 32 bit values layed out as follows:

;//

;//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1

;//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0

;//  +---+-+-+-----------------------+-------------------------------+

;//  |Sev|C|R|     Facility          |               Code            |

;//  +---+-+-+-----------------------+-------------------------------+

;//

;//  where

;//

;//      Sev - is the severity code

;//

;//          00 - Success

;//          01 - Informational

;//          10 - Warning

;//          11 - Error

;//

;//      C - is the Customer code flag

;//

;//      R - is a reserved bit

;//

;//      Facility - is the facility code

;//

;//      Code - is the facility's status code

;//



MessageIdTypedef=NTSTATUS



SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS

               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL

               Warning=0x2:STATUS_SEVERITY_WARNING

               Error=0x3:STATUS_SEVERITY_ERROR

              )



FacilityNames=(System=0x0

               RpcRuntime=0x2:FACILITY_RPC_RUNTIME

               RpcStubs=0x3:FACILITY_RPC_STUBS

               Io=0x4:FACILITY_IO_ERROR_CODE

               FTD=0x7:FACILITY_FTD_ERROR_CODE

              )







MessageIdTypedef=LONG
MessageId=0x0001 
Facility=Application 
Severity=Informational 
SymbolicName=FTD_SIMPLE_MESSAGE
Language=English
%1
.

MessageId=
Facility=Application
Severity=Informational 
SymbolicName=FTD_SIMPLE_KERNEL_MESSAGE
Language=English
%2
.

MessageId=
Severity=Error
Facility=Application
SymbolicName=FTD_SERVICE_NOT_RUNNING_ERROR
Language=English
The %1 service is not running.
.

;//
;//     Perfutil messages
;//
MessageId=
Severity=Error
Facility=Application
SymbolicName=FTD_PERF_CANT_READ_DATA
Language=English
Unable to read performance data from %1.
.

MessageId=
Severity=Error
Facility=Application
SymbolicName=FTD_PERF_CANT_READ_FIRST_COUNTER
Language=English
Unable to read the "First Counter" value 
under the %1\Performance key.
Status code returned in data.
.

MessageId=
Severity=Error
Facility=Application
SymbolicName=FTD_PERF_CANT_READ_FIRST_HELP
Language=English
Unable to read the "First Help" value 
under the %1\Performance key. 
Status code returned in data.
.

MessageId=
Severity=Error
Facility=Application
SymbolicName=FTD_PERF_OPEN_FILE_MAPPING_ERROR
Language=English
Unable to open mapped file containing %1 performance data.
.

MessageId=
Severity=Error
Facility=Application
SymbolicName=FTD_PERF_UNABLE_MAP_VIEW_OF_FILE
Language=English
Unable to map to shared memory file containing %1 performance data.
.

MessageId=
Severity=Error
Facility=Application
SymbolicName=FTD_PERF_UNABLE_OPEN_DRIVER_KEY
Language=English
Unable open "Performance" key of %1 Application in registy. 
Status code is returned in data.
.

;#endif /* _LOGMSG_H_ */

;

