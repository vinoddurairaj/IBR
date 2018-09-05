; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CDiskUserDlg
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "diskuser.h"
LastPage=0

ClassCount=3
Class1=CDiskUserApp
Class2=CDiskUserDlg
Class3=CDiskUserDlgAutoProxy

ResourceCount=1
Resource1=IDD_DISKUSER_DIALOG (English (U.S.))

[CLS:CDiskUserApp]
Type=0
BaseClass=CWinApp
HeaderFile=DiskUser.h
ImplementationFile=DiskUser.cpp
LastObject=CDiskUserApp

[CLS:CDiskUserDlg]
Type=0
BaseClass=CDialog
HeaderFile=DiskUserDlg.h
ImplementationFile=DiskUserDlg.cpp
LastObject=CDiskUserDlg
Filter=D
VirtualFilter=dWC

[CLS:CDiskUserDlgAutoProxy]
Type=0
BaseClass=CCmdTarget
HeaderFile=DlgProxy.h
ImplementationFile=DlgProxy.cpp

[DLG:IDD_DISKUSER_DIALOG]
Type=1
Class=CDiskUserDlg

[DLG:IDD_DISKUSER_DIALOG (English (U.S.))]
Type=1
Class=CDiskUserDlg
ControlCount=24
Control1=IDSTART,button,1342242817
Control2=IDSTOP,button,1342242816
Control3=IDC_STATIC,static,1342308352
Control4=IDC_STATIC,button,1342177287
Control5=IDC_STATIC,button,1342177287
Control6=IDC_STATIC,static,1342308352
Control7=IDC_STATIC,static,1342308352
Control8=IDC_STATIC,static,1342308352
Control9=IDC_STATIC,static,1342308352
Control10=IDC_STATIC,static,1342308352
Control11=IDC_STATIC,static,1342308352
Control12=IDC_EDIT_NumThreads,edit,1350639745
Control13=IDC_EDIT_DiskSize,edit,1350639745
Control14=IDC_STATIC_TestStatus,static,1342308353
Control15=IDC_LISTDRIVENAMES,combobox,1344339971
Control16=IDC_LISTPRIORITIES,combobox,1344339971
Control17=IDC_EDIT_NumThreadsExecuting,edit,1350641665
Control18=IDC_EDIT_Kb_DiskSizePerThread,edit,1350641793
Control19=IDC_STATIC,static,1342308352
Control20=IDC_STATIC,static,1342308352
Control21=IDC_ReadWriteRatio,static,1342308353
Control22=IDC_SLIDER_ReadWriteRatio,msctls_trackbar32,1342242840
Control23=IDC_EDIT_SleepRange,edit,1350639745
Control24=IDC_SleepRange,static,1342308353

