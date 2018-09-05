//tdimain.cpp

// Mike Pollett
//#include	"..\tdmf.inc"

#include "tdiutil.h"

PDRIVER_OBJECT g_pTheDriverObject = NULL;


NTSTATUS DriverEntry(
PDRIVER_OBJECT      pDriverObject,       // created by the I/O sub-system
PUNICODE_STRING     RegistryPath)       // path to the registry key
{
   PDEVICE_EXTENSION pDeviceExtension = NULL;
   NTSTATUS          LoadStatus, Status = STATUS_SUCCESS;
   ULONG             DevicesCreated = 0;

   //
   // Initialize Global Data
   //
   KdPrint(("TDITTCP: DriverEntry\n") );
   KdPrint(("  RegistryPath: %ws\n", RegistryPath->Buffer) );

   g_pTheDriverObject = pDriverObject;

   LoadStatus = SftkDeviceLoadC(
                  pDriverObject,
                  RegistryPath
                  );

   if( NT_SUCCESS( LoadStatus ) )
   {
      ++DevicesCreated;
   }

   LoadStatus = SftkDeviceLoadS(
                  pDriverObject,
                  RegistryPath
                  );

   if( NT_SUCCESS( LoadStatus ) )
   {
      ++DevicesCreated;
   }

   if( !DevicesCreated )
   {
      return STATUS_UNSUCCESSFUL;
   }


   return STATUS_SUCCESS;

}