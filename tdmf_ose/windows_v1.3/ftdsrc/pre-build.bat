@Echo Off

if %1 == Debug goto NT_Release
if %1 == Release goto NT_Release
	rem set TOS=W2K

	goto next
:NT_Release
	rem set TOS=NT4
:next

set W2KBASE=%2NETDDK\3790
set prefix=Dtc
rem set QNM=tdmf
rem set CAPQ=TDMF
set MASTERNAME=TdmfReplServer
set PRODUCTNAME=TDMF
