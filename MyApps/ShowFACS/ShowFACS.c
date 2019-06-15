//
//  Copyright (c) 2018-2019  Finnbarr P. Murphy.  All rights reserved.
//
//  Display ACPI FACS (Firmware ACPI Control Structure)
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
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PrintLib.h>

#include <Protocol/LoadedImage.h>
#include <Protocol/AcpiSystemDescriptionTable.h>

#include <Guid/Acpi.h>

#define UTILITY_VERSION L"20190612"
#undef DEBUG


#if 0
typedef struct {
   UINT32  Signature;
   UINT32  Length;
   UINT32  HardwareSignature;
   UINT32  FirmwareWakingVector;
   UINT32  GlobalLock;
   UINT32  Flags;
   UINT64  XFirmwareWakingVector;
   UINT8   Version;
   UINT8   Reserved[31];
} EFI_ACPI_2_0_FIRMWARE_ACPI_CONTROL_STRUCTURE;
#endif


VOID AsciiToUnicodeSize(CHAR8 *, UINT8, CHAR16 *, BOOLEAN);

VOID
AsciiToUnicodeSize( CHAR8 *String,
                    UINT8 length,
                    CHAR16 *UniString,
                    BOOLEAN Quote )
{
    int len = length;

    if (Quote)
        *(UniString++) = L'"';
    while (*String != '\0' && len > 0) {
        *(UniString++) = (CHAR16) *(String++);
        len--;
    }
    if (Quote)
        *(UniString++) = L'"';
    *UniString = '\0';
}


VOID
PrintHexTable( UINT8   *ptr,
               int     Count,
               BOOLEAN ExtraSpace )
{
    if (ExtraSpace) { 
        Print(L"    ");
    } else {
        Print(L"  ");
    }

    for (int i = 0; i < Count; i++ ) {
        if ( i > 0 && i%16 == 0) {
            if (ExtraSpace) { 
                Print(L"\n    ");
            } else {
                Print(L"\n  ");
            }
        }
        Print(L"0x%02x ", 0xff & *ptr++);
    }
    Print(L"\n");
}


VOID 
PrintFACS( EFI_ACPI_2_0_FIRMWARE_ACPI_CONTROL_STRUCTURE *Facs,
           BOOLEAN Hexdump )
{
    CHAR16 Buffer[50];

    Print(L"\n");
    if (Hexdump) {
        PrintHexTable( (UINT8 *)Facs, (int)(Facs->Length), FALSE );
    } else {
        Print(L"  FACS Table Details\n"); 
        AsciiToUnicodeSize((CHAR8 *)&(Facs->Signature), 4, Buffer, TRUE);
        Print(L"    Signature             : %s\n", Buffer);
        Print(L"    Length                : 0x%08x (%d)\n", Facs->Length, Facs->Length);
        Print(L"    Hardware Signature    : 0x%08x (%d)\n", Facs->HardwareSignature, 
                                                            Facs->HardwareSignature);
        Print(L"    FirmwareWakingVector  : 0x%08x (%d)\n", Facs->FirmwareWakingVector, 
                                                            Facs->FirmwareWakingVector);
        Print(L"    GlobalLock            : 0x%08x (%d)\n", Facs->GlobalLock, Facs->GlobalLock);
        Print(L"    Flags                 : 0x%08x (%d)\n", Facs->Flags, Facs->Flags);
        Print(L"    XFirmwareWakingVector : 0x%016x (%ld)\n", Facs->XFirmwareWakingVector, 
                                                              Facs->XFirmwareWakingVector);
        Print(L"    Version               : 0x%02x (%d)\n", Facs->Version, Facs->Version);
	Print(L"    Reserved:\n");
        PrintHexTable( (UINT8 *)&(Facs->Reserved), 31, TRUE );
    }
    Print(L"\n");
}


VOID
ParseRSDP( EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER *Rsdp, 
           CHAR16* GuidStr, 
           BOOLEAN Hexdump )
{
    EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE *Fadt2Table;
    EFI_ACPI_2_0_FIRMWARE_ACPI_CONTROL_STRUCTURE *Facs2Table;
    EFI_ACPI_SDT_HEADER *Xsdt, *Entry;
    UINT32 EntryCount;
    UINT64 *EntryPtr;
#ifdef DEBUG
    CHAR16 OemStr[20];

    Print(L"\n\nACPI GUID: %s\n", GuidStr);

    AsciiToUnicodeSize((CHAR8 *)(Rsdp->OemId), 6, OemStr, FALSE);
    Print(L"\nFound RSDP. Version: %d  OEM ID: %s\n", (int)(Rsdp->Revision), OemStr);
#endif

    if (Rsdp->Revision >= EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER_REVISION) {
        Xsdt = (EFI_ACPI_SDT_HEADER *)(Rsdp->XsdtAddress);
    } else {
#ifdef DEBUG 
        Print(L"ERROR: No ACPI XSDT table found.\n");
#endif
        return;
    }

    if (Xsdt->Signature != SIGNATURE_32 ('X', 'S', 'D', 'T')) {
#ifdef DEBUG 
        Print(L"ERROR: Invalid ACPI XSDT table found.\n");
#endif
        return;
    }

    EntryCount = (Xsdt->Length - sizeof (EFI_ACPI_SDT_HEADER)) / sizeof(UINT64);
#ifdef DEBUG 
    AsciiToUnicodeSize((CHAR8 *)(Xsdt->OemId), 6, OemStr, FALSE);
    Print(L"Found XSDT. OEM ID: %s  Entry Count: %d\n\n", OemStr, EntryCount);
#endif

    // Locate Fixed ACPI Description Table - "FACP"
    EntryPtr = (UINT64 *)(Xsdt + 1);
    for (int Index = 0; Index < EntryCount; Index++, EntryPtr++) {
        Entry = (EFI_ACPI_SDT_HEADER *)((UINTN)(*EntryPtr));
        if (!AsciiStrnCmp( (CHAR8 *)&(Entry->Signature), "FACP", 4)) {
            Fadt2Table = (EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE *)((UINTN)(*EntryPtr));
            Facs2Table = (EFI_ACPI_2_0_FIRMWARE_ACPI_CONTROL_STRUCTURE *)((UINTN)(Fadt2Table->FirmwareCtrl));
            PrintFACS(Facs2Table, Hexdump);
        }
    }

    return;
}


VOID
Usage( BOOLEAN ErrorMsg )
{
    if ( ErrorMsg ) {
        Print(L"ERROR: Unknown option.\n");
    }
    Print(L"Usage: ShowFACS [-d | --dump]\n");
    Print(L"       ShowFACS [-V | --version]\n");
}


INTN
EFIAPI
ShellAppMain( UINTN Argc, 
              CHAR16 **Argv )
{
    EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER *Rsdp = NULL;
    EFI_CONFIGURATION_TABLE *ect = gST->ConfigurationTable;
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_GUID   gAcpi20TableGuid = EFI_ACPI_20_TABLE_GUID;
    EFI_GUID   gAcpi10TableGuid = ACPI_10_TABLE_GUID;
    BOOLEAN    Hexdump = FALSE;
    CHAR16     GuidStr[100];

    if (Argc == 2) {
        if (!StrCmp(Argv[1], L"--dump") ||
            !StrCmp(Argv[1], L"-d")) {
            Hexdump = TRUE;
        } else if (!StrCmp(Argv[1], L"--version") ||
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

    // Locate Root System Description Pointer Table - "RSDP" 
    for (int i = 0; i < gST->NumberOfTableEntries; i++) {
        if ((CompareGuid (&(gST->ConfigurationTable[i].VendorGuid), &gAcpi20TableGuid)) ||
            (CompareGuid (&(gST->ConfigurationTable[i].VendorGuid), &gAcpi10TableGuid))) {
            if (!AsciiStrnCmp("RSD PTR ", (CHAR8 *)(ect->VendorTable), 8)) {
                UnicodeSPrint(GuidStr, sizeof(GuidStr), L"%g", &(gST->ConfigurationTable[i].VendorGuid));
                Rsdp = (EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER *)ect->VendorTable;
                ParseRSDP(Rsdp, GuidStr, Hexdump); 
            }        
        }
        ect++;
    }

    if (Rsdp == NULL) {
        Print(L"ERROR: Could not find an ACPI RSDP table.\n");
        Status = EFI_NOT_FOUND;
    }

    return Status;
}
