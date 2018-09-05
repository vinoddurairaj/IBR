#include "ntddk.h"

//
// FOR IN_FCT/OUT_FCT definitions
//
#include "ftd_def.h"

PVOID
MapMemMapTheMemory(
    IN PHYSICAL_ADDRESS physicalAddress,
    IN ULONG            length
    )
/*++

Routine Description:

    Given a physical address, maps this address into a user mode process's
    address space

Arguments:


Return Value:

    STATUS_SUCCESS if sucessful, otherwise
    STATUS_UNSUCCESSFUL,
    STATUS_INSUFFICIENT_RESOURCES,
    (other STATUS_* as returned by kernel APIs)

--*/
{
    INTERFACE_TYPE     interfaceType;
    ULONG              busNumber;

    ULONG              inIoSpace;
    PHYSICAL_ADDRESS   physicalAddressBase;
    BOOLEAN            translateBaseAddress;
    PVOID              virtualAddress = NULL;

    IN_FCT(MapMemMapTheMemory)

    interfaceType   = 0; // internal
    busNumber       = 0; // what else
    inIoSpace       = 0; // memory

    //
    // Translate the physical addresses.
    //

    translateBaseAddress =
        HalTranslateBusAddress (interfaceType,
                                busNumber,
                                physicalAddress,
                                &inIoSpace,
                                &physicalAddressBase);

    if ( !translateBaseAddress)
    {
        OUT_FCT(MapMemMapTheMemory)
        return virtualAddress;
    }

    virtualAddress = MmMapIoSpace(
        physicalAddressBase,
        length,
        FALSE);

    OUT_FCT(MapMemMapTheMemory)
    return virtualAddress;
}

VOID
MapMemUnMapTheMemory(
    IN PVOID BaseAddress, 
    IN ULONG  NumberOfBytes
    )
{
    IN_FCT(MapMemUnMapTheMemory)
    MmUnmapIoSpace(BaseAddress, NumberOfBytes);
    OUT_FCT(MapMemUnMapTheMemory)
}
