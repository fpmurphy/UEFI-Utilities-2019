//
//  Copyright (c) 2019   Finnbarr P. Murphy.   All rights reserved.
//
//  Show USB devices
//
//  License: EDKII license applies to code from EDKII source,
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
#include <Protocol/UsbIo.h>

#define UTILITY_VERSION L"20190408"
#undef DEBUG


VOID
Usage( BOOLEAN ErrorMsg )
{
    if ( ErrorMsg ) {
        Print(L"ERROR: Unknown option.\n");
    }
    Print(L"Usage: ShowUSB [-V | --version]\n");
}


INTN
EFIAPI
ShellAppMain( UINTN Argc, 
              CHAR16 **Argv )
{
    EFI_USB_DEVICE_DESCRIPTOR DevDesc;
    EFI_USB_INTERFACE_DESCRIPTOR IfDesc;
    EFI_USB_IO_PROTOCOL *UsbIo;
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_HANDLE *HandleBuffer = NULL;
    BOOLEAN    LangFound;
    CHAR16     *Manufacturer;
    CHAR16     *Product;
    CHAR16     *SerialNumber;
    UINT16     *LangIdTable;
    UINT16     TableSize;
    UINTN      HandleCount;

    if (Argc == 2) {
        if (!StrCmp(Argv[1], L"--version") ||
            !StrCmp(Argv[1], L"-V")) {
            Print(L"Version: %s\n", UTILITY_VERSION);
            return Status;
        } else if (!StrCmp(Argv[1], L"--help") ||
            !StrCmp(Argv[1], L"-h")) {
            Usage(FALSE);
            return Status;
        } else {
            Usage(TRUE);
            return Status;
        }
    }
    if (Argc > 2) {
        Usage(TRUE);
        return Status;
    }
 
    Status = gBS->LocateHandleBuffer( ByProtocol,
                                      &gEfiUsbIoProtocolGuid,
                                      NULL,
                                      &HandleCount,
                                      &HandleBuffer );
    if (EFI_ERROR(Status)) {
        Print(L"ERROR: LocateHandleBuffer.\n");
        return Status;
    }  
	 
    Print(L"\n");
    Print(L"  VendorID  ProductID  Manufacturer/Product/SerialNumbaer\n");
    Print(L"  -------------------------------------------------------\n");
    Print(L"\n");

    for ( UINT8 Index = 0; Index < HandleCount; Index++) {
        Status = gBS->HandleProtocol( HandleBuffer[Index],
                                      &gEfiUsbIoProtocolGuid,
                                      (VOID**)&UsbIo );
        if (EFI_ERROR(Status)) {
            Print(L"ERROR: Open UsbIo.\n");
            FreePool( HandleBuffer ); 
            return Status;
         }
	 
         Status = UsbIo->UsbGetDeviceDescriptor( UsbIo, &DevDesc );    
         if (EFI_ERROR(Status)) {
             Print(L"ERROR: UsbGetDeviceDescriptor.\n");
             FreePool( HandleBuffer ); 
             return Status;
         }
	 
         Status = UsbIo->UsbGetInterfaceDescriptor( UsbIo, &IfDesc );
         if (EFI_ERROR (Status)) {
             Print(L"ERROR: UsbGetInterfaceDescriptor.\n");
             FreePool( HandleBuffer ); 
             return Status;
         }
	 
         TableSize = 0;
         LangIdTable = NULL;
         LangFound = TRUE;
         Status = UsbIo->UsbGetSupportedLanguages (UsbIo, &LangIdTable, &TableSize);
#if 0
         if (EFI_ERROR (Status) || (TableSize == 0) || (LangIdTable == NULL)) {
             Print(L"ERROR: UsbGetSupportedLanguages\n");
             LangFound = FALSE;
             FreePool( HandleBuffer ); 
             return Status;
         }
#endif
         FreePool( LangIdTable ); 

         Status = UsbIo->UsbGetStringDescriptor( UsbIo,
	                                         0x0409,  // English
	                                         DevDesc.StrManufacturer,
	                                         &Manufacturer );
         if (EFI_ERROR (Status)) {
             Manufacturer = L"Unknown";
         }
	 
         Status = UsbIo->UsbGetStringDescriptor( UsbIo,
                                                 0x0409,
                                                 DevDesc.StrProduct,
                                                 &Product );
         if (EFI_ERROR (Status)) {
             Product = L"Unknown";  
         }
	                 
         Status = UsbIo->UsbGetStringDescriptor( UsbIo,
                                                 0x0409,
                                                 DevDesc.StrSerialNumber,
                                                 &SerialNumber );
         if (EFI_ERROR (Status)) {
             SerialNumber = L"Unknown";
         }

         Print(L"    %04X      %04X     %s, %s, %s\n",
                      DevDesc.IdVendor,
                      DevDesc.IdProduct,
                      Manufacturer,
                      Product,
                      SerialNumber );     
    }
    Print(L"\n");

    FreePool( HandleBuffer );      

    return Status;
}
