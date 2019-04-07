//
//  Copyright (c) 2017-2019   Finnbarr P. Murphy.   All rights reserved.
//
//  Show PCI devices
//
//  License: UDK2015 license applies to code from UDK2015 source,
//           BSD 2 clause license applies to all other code.
//

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/ShellCEntryLib.h>
#include <Library/ShellLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PrintLib.h>

#include <Protocol/LoadedImage.h>
#include <Protocol/PciEnumerationComplete.h>
#include <Protocol/PciRootBridgeIo.h>

#include <IndustryStandard/Pci.h>
 
#define CALC_EFI_PCI_ADDRESS(Bus, Dev, Func, Reg) \
    ((UINT64) ((((UINTN) Bus) << 24) + (((UINTN) Dev) << 16) + (((UINTN) Func) << 8) + ((UINTN) Reg)))


#pragma pack(1)
typedef union {
   PCI_DEVICE_HEADER_TYPE_REGION  Device;
   PCI_CARDBUS_CONTROL_REGISTER   CardBus;
} NON_COMMON_UNION;

typedef struct {
   PCI_DEVICE_INDEPENDENT_REGION  Common;
   NON_COMMON_UNION               NonCommon;
   UINT32                         Data[48];
} PCI_CONFIG_SPACE;
#pragma pack()

#define UTILITY_VERSION L"20190329"
#undef DEBUG


//
// Copyed from UDK2015 Source. UDK2015 license applies.
//
EFI_STATUS
PciGetNextBusRange( EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  **Descriptors,
                    UINT16 *MinBus,
                    UINT16 *MaxBus,
                    BOOLEAN *IsEnd )
{
    *IsEnd = FALSE;

    if ((*Descriptors) == NULL) {
        *MinBus = 0;
        *MaxBus = PCI_MAX_BUS;
        return EFI_SUCCESS;
    }

    while ((*Descriptors)->Desc != ACPI_END_TAG_DESCRIPTOR) {
        if ((*Descriptors)->ResType == ACPI_ADDRESS_SPACE_TYPE_BUS) {
            *MinBus = (UINT16) (*Descriptors)->AddrRangeMin;
            *MaxBus = (UINT16) (*Descriptors)->AddrRangeMax;
            (*Descriptors)++;
            return (EFI_SUCCESS);
        }

        (*Descriptors)++;
    }

    if ((*Descriptors)->Desc == ACPI_END_TAG_DESCRIPTOR) {
        *IsEnd = TRUE;
    }

    return EFI_SUCCESS;
}


//
// Copyed from UDK2015 Source. UDK2015 license applies.
//
EFI_STATUS
PciGetProtocolAndResource( EFI_HANDLE Handle,
                           EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL **IoDev,
                           EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR **Descriptors )
{
    EFI_STATUS Status;

    // Get inferface from protocol
    Status = gBS->HandleProtocol( Handle,
                                  &gEfiPciRootBridgeIoProtocolGuid,
                                  (VOID**)IoDev );
    if (EFI_ERROR (Status)) {
        return Status;
    }

    Status = (*IoDev)->Configuration( *IoDev, (VOID**)Descriptors );
    if (Status == EFI_UNSUPPORTED) {
        *Descriptors = NULL;
        return EFI_SUCCESS;
    }

    return Status;
}


VOID
Usage( CHAR16 *Str, 
       BOOLEAN ErrorMsg )
{
    if ( ErrorMsg ) {
        Print(L"ERROR: Unknown option.\n");
    }
    Print(L"Usage: %s [-V | --version]\n", Str);
}


INTN
EFIAPI
ShellAppMain( UINTN Argc, 
              CHAR16 **Argv )
{
    EFI_GUID gEfiPciEnumerationCompleteProtocolGuid = EFI_PCI_ENUMERATION_COMPLETE_GUID;  
    EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *IoDev;
    EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Descriptors;
    PCI_DEVICE_INDEPENDENT_REGION PciHeader;
    PCI_CONFIG_SPACE ConfigSpace;
    PCI_DEVICE_HEADER_TYPE_REGION *DeviceHeader;
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_HANDLE *HandleBuf;
    UINTN HandleBufSize;
    UINTN HandleCount;
    UINT64 Address;
    UINT16 MinBus;
    UINT16 MaxBus;
    BOOLEAN IsEnd; 
    VOID *Interface;

    if (Argc == 2) {
        if (!StrCmp(Argv[1], L"--version") ||
            !StrCmp(Argv[1], L"-V")) {
            Print(L"Version: %s\n", UTILITY_VERSION);
            return Status;
        } else if (!StrCmp(Argv[1], L"--help") ||
            !StrCmp(Argv[1], L"-h")) {
            Usage(Argv[0], FALSE);
            return Status;
        } else {
            Usage(Argv[0], TRUE);
            return Status;
        }
    }
    if (Argc > 2) {
        Usage(Argv[0], TRUE);
        return Status;
    }
 

    Status = gBS->LocateProtocol( &gEfiPciEnumerationCompleteProtocolGuid,
                                  NULL,
                                  &Interface );
    if (EFI_ERROR(Status)) {
        Print(L"ERROR: Could not find PCI enumeration protocol\n");
        return Status;
    }

    HandleBufSize = sizeof(EFI_HANDLE);
    HandleBuf = (EFI_HANDLE *) AllocateZeroPool( HandleBufSize);
    if (HandleBuf == NULL) {
        Print(L"ERROR: Out of memory resources\n");
        return EFI_OUT_OF_RESOURCES;
    }

    Status = gBS->LocateHandle( ByProtocol,
                                &gEfiPciRootBridgeIoProtocolGuid,
                                NULL,
                                &HandleBufSize,
                                HandleBuf);

    if (Status == EFI_BUFFER_TOO_SMALL) {
        HandleBuf = ReallocatePool (sizeof (EFI_HANDLE), HandleBufSize, HandleBuf);
        if (HandleBuf == NULL) {
            Print(L"ERROR: Out of memory resources\n");
            Status = EFI_OUT_OF_RESOURCES;
            goto Done;
        }

        Status = gBS->LocateHandle( ByProtocol,
                                    &gEfiPciRootBridgeIoProtocolGuid,
                                    NULL,
                                    &HandleBufSize,
                                    HandleBuf);
    }

    if (EFI_ERROR (Status)) {
        Print(L"ERROR: Failed to find any PCI handles\n");
        goto Done;
    }

    HandleCount = HandleBufSize / sizeof (EFI_HANDLE);

    for (UINT16 Index = 0; Index < HandleCount; Index++) {
        Status = PciGetProtocolAndResource( HandleBuf[Index],
                                            &IoDev,
                                            &Descriptors);
        if (EFI_ERROR(Status)) {
            Print(L"ERROR: PciGetProtocolAndResource [%d]\n, Status");
            goto Done;
        }
  
        while(TRUE) {
            Status = PciGetNextBusRange( &Descriptors, &MinBus, &MaxBus, &IsEnd);
            if (EFI_ERROR(Status)) {
                Print(L"ERROR: Retrieving PCI bus range [%d]\n", Status);
                goto Done;
            }

            if (IsEnd) {
                break;
            }

            Print(L"\n");
            Print(L"  Bus     Vendor    Device   Subvendor SubvendorDevice\n");
            Print(L"  ----------------------------------------------------\n");

            for (UINT16 Bus = MinBus; Bus <= MaxBus; Bus++) {
                for (UINT16 Device = 0; Device <= PCI_MAX_DEVICE; Device++) {
                    for (UINT16 Func = 0; Func <= PCI_MAX_FUNC; Func++) {
                         Address = CALC_EFI_PCI_ADDRESS (Bus, Device, Func, 0);

                         Status = IoDev->Pci.Read( IoDev,
                                                   EfiPciWidthUint8,
                                                   Address,
                                                   sizeof(ConfigSpace),
                                                   &ConfigSpace);

                         DeviceHeader = (PCI_DEVICE_HEADER_TYPE_REGION *) &(ConfigSpace.NonCommon.Device);

                         IoDev->Pci.Read( IoDev,
                                          EfiPciWidthUint16,
                                          Address,
                                          1,
                                          &PciHeader.VendorId );

                         if (PciHeader.VendorId == 0xffff && Func == 0) {
                             break;
                         }

                         if (PciHeader.VendorId != 0xffff) {
                             IoDev->Pci.Read( IoDev,
                                              EfiPciWidthUint32,
                                              Address,
                                              sizeof (PciHeader) / sizeof (UINT32),
                                              &PciHeader );

                             Print(L"   %02d      %04x      %04x       %04x       %04x\n", 
                                   Bus, PciHeader.VendorId, PciHeader.DeviceId, 
                                   DeviceHeader->SubsystemVendorID, DeviceHeader->SubsystemID);

                             if (Func == 0 && 
                                ((PciHeader.HeaderType & HEADER_TYPE_MULTI_FUNCTION) == 0x00)) {
                                break;
                             }
                         }
                     }
                 }
             }

            if (Descriptors == NULL) {
                break;
            }
        }
    }

    Print(L"\n");

Done:
    if (HandleBuf != NULL) {
        FreePool( HandleBuf );
    }

    return Status;
}
