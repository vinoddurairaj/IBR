md InstallStruc
cd InstallStruc
md Gui
md Install
md Collector
md UninstallIcon
md ReplicationServer
md ReplicationServer\nt4
md ReplicationServer\win2000


REM COPY GUI

copy ..\W2K\PerformanceMonitor.ocx Gui
copy ..\W2K\TdmfCommonGui.exe Gui
copy ..\W2K\ContextMenus.reg Gui
copy ..\W2K\TDMFObjects.dll Gui

REM COPY COLLECTOR

copy ..\W2k\tdmfCollector.exe Collector

REM COPY NT4 FILES

copy ..\NT4\tdmfhostinfo.exe ReplicationServer\nt4               
copy ..\NT4\tdmfkillbackfresh.exe ReplicationServer\nt4
copy ..\NT4\tdmfinfo.exe ReplicationServer\nt4
copy ..\NT4\tdmfkillpmd.exe ReplicationServer\nt4
copy ..\NT4\tdmfdebugcapture.exe ReplicationServer\nt4
copy ..\NT4\tdmfLogMsg.dll ReplicationServer\nt4
copy ..\NT4\tdmfinit.exe ReplicationServer\nt4
copy ..\NT4\tdmfset.exe ReplicationServer\nt4
copy ..\NT4\tdmfstop.exe ReplicationServer\nt4
copy ..\NT4\tdmflaunchpmd.exe ReplicationServer\nt4
copy ..\NT4\tdmfreco.exe ReplicationServer\nt4
copy ..\NT4\tdmfstart.exe ReplicationServer\nt4
copy ..\NT4\tdmfkillrmd.exe ReplicationServer\nt4
copy ..\NT4\tdmf_ReplServer.exe ReplicationServer\nt4
copy ..\NT4\tdmftrace.exe ReplicationServer\nt4
copy ..\NT4\tdmflaunchrefresh.exe ReplicationServer\nt4
copy ..\NT4\tdmflaunchbackfresh.exe ReplicationServer\nt4
copy ..\NT4\tdmflicinfo.exe ReplicationServer\nt4
copy ..\NT4\tdmfcheckpoint.exe ReplicationServer\nt4
copy ..\NT4\TDMFblock.sys ReplicationServer\nt4
copy ..\NT4\tdmfkillrefresh.exe ReplicationServer\nt4
copy ..\NT4\tdmfPerf.dll ReplicationServer\nt4
copy ..\NT4\tdmfoverride.exe ReplicationServer\nt4      
copy ..\NT4\ContextMenus.reg ReplicationServer\nt4
copy ..\..\cp_post_on_000.bat ReplicationServer\nt4
copy ..\..\cp_pre_on_000.bat ReplicationServer\nt4
copy ..\..\cp_pre_off_000.bat ReplicationServer\nt4
copy ..\..\lib\libftd\errors.MSG ReplicationServer\nt4
REM copy ..\tdmfboot.bat ReplicationServer\nt4
REM copy ..\tdmf_ReplServer.reg ReplicationServer\nt4

REM COPY WIN2000 FILES

copy ..\W2K\tdmfhostinfo.exe ReplicationServer\win2000               
copy ..\W2K\tdmfkillbackfresh.exe ReplicationServer\win2000 
copy ..\W2K\tdmfinfo.exe ReplicationServer\win2000
copy ..\W2K\tdmfkillpmd.exe ReplicationServer\win2000
copy ..\W2K\tdmfdebugcapture.exe ReplicationServer\win2000
copy ..\W2K\tdmfLogMsg.dll ReplicationServer\win2000
copy ..\W2K\tdmfinit.exe ReplicationServer\win2000
copy ..\W2K\tdmfset.exe ReplicationServer\win2000
copy ..\W2K\tdmfstop.exe ReplicationServer\win2000
copy ..\W2K\tdmflaunchpmd.exe ReplicationServer\win2000
copy ..\W2K\tdmfreco.exe ReplicationServer\win2000
copy ..\W2K\tdmfstart.exe ReplicationServer\win2000
copy ..\W2K\tdmfkillrmd.exe ReplicationServer\win2000
copy ..\W2K\tdmf_ReplServer.exe ReplicationServer\win2000
copy ..\W2K\tdmftrace.exe ReplicationServer\win2000
copy ..\W2K\tdmflaunchrefresh.exe ReplicationServer\win2000
copy ..\W2K\tdmflaunchbackfresh.exe ReplicationServer\win2000
copy ..\W2K\tdmflicinfo.exe ReplicationServer\win2000
copy ..\W2K\tdmfcheckpoint.exe ReplicationServer\win2000
copy ..\W2K\TDMFblock.sys ReplicationServer\win2000
copy ..\W2K\tdmfkillrefresh.exe ReplicationServer\win2000
copy ..\W2K\tdmfPerf.dll ReplicationServer\win2000
copy ..\W2K\tdmfoverride.exe ReplicationServer\win2000      
copy ..\W2K\ContextMenus.reg ReplicationServer\win2000
copy ..\..\cp_post_on_000.bat ReplicationServer\win2000
copy ..\..\cp_pre_on_000.bat ReplicationServer\win2000
copy ..\..\cp_pre_off_000.bat ReplicationServer\win2000
copy ..\..\lib\libftd\errors.MSG ReplicationServer\win2000
REM copy ..\tdmfboot.bat ReplicationServer\win2000
REM copy ..\tdmf_ReplServer.reg ReplicationServer\win2000

REM COPY INSTALL AND OTHER FILES

copy ..\..\gui\TdmfCommonGui\res\sftk.ico UninstallIcon
copy ..\W2K\TdmfDbUpgrade.dll Install
copy ..\W2K\TDMFInstall.dll Install