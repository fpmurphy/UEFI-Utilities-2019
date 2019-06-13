//
//  Copyright (c) 2015-2019  Finnbarr P. Murphy.   All rights reserved.
//
//  Show ACPI tables
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

#define UTILITY_VERSION L"20190606"
#undef DEBUG


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
PrintListEntry( EFI_ACPI_SDT_HEADER *Ptr, int Index )
{
    CHAR16 Buffer1[20];

    AsciiToUnicodeSize((CHAR8 *)&(Ptr->Signature), 4, Buffer1, FALSE);
    Print(L"  %s", Buffer1);
    
#if 0
    if (!AsciiStrnCmp( (CHAR8 *)&(Ptr->Signature), "FACP", 4)) {
        Print(L"  (inc. FACS, DSDT)");
    }
#endif

    if ((++Index)%8 == 0) {
        Print(L"\n");
    }
}


VOID
PrintTableEntry( EFI_ACPI_SDT_HEADER *Ptr )
{
    CHAR16 Buffer1[20], Buffer2[20];

    AsciiToUnicodeSize((CHAR8 *)&(Ptr->Signature), 4, Buffer1, FALSE);
    AsciiToUnicodeSize((CHAR8 *)&(Ptr->CreatorId), 4, Buffer2, FALSE);

    Print(L"  %s    0x%02x     %s     0x%08x", 
          Buffer1, (int)(Ptr->Revision), Buffer2, (int)(Ptr->CreatorRevision) );
    AsciiToUnicodeSize((CHAR8 *)&(Ptr->OemTableId), 8, Buffer1, TRUE);
    Print(L"   %s", Buffer1);

#if 0
    if (!AsciiStrnCmp( (CHAR8 *)&(Ptr->Signature), "FACP", 4)) {
        Print(L"  (inc. FACS, DSDT)");
    }
#endif
    Print(L"\n");
}


VOID
ParseRSDP( EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER *Rsdp, 
           CHAR16* GuidStr,
           BOOLEAN Verbose )
{
    EFI_ACPI_SDT_HEADER *Xsdt;
    UINT32 EntryCount;
    UINT64 *EntryPtr;
    CHAR16 OemStr[20];

    if (Rsdp->Revision >= EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER_REVISION) {
        if ( Verbose ) {
            AsciiToUnicodeSize((CHAR8 *)(Rsdp->OemId), 6, OemStr, FALSE);
            Print(L"\n  RSDP Revision: %d  OEM ID: %s\n", (int)(Rsdp->Revision), OemStr);
        }
        Xsdt = (EFI_ACPI_SDT_HEADER *)(Rsdp->XsdtAddress);
    } else {
#ifdef DEBUG
        Print(L"ERROR: RSDP table < revision ACPI 2.0 found.\n");
#endif
        return;
    }

    if (Xsdt->Signature != SIGNATURE_32 ('X', 'S', 'D', 'T')) {
        Print(L"ERROR: Invalid ACPI XSDT table found.\n");
        return;
    }

    EntryCount = (Xsdt->Length - sizeof (EFI_ACPI_SDT_HEADER)) / sizeof(UINT64);
    if ( Verbose ) {
        AsciiToUnicodeSize((CHAR8 *)(Xsdt->OemId), 6, OemStr, FALSE);
        Print(L"  XSDT Revision: %d  OEM ID: %s  Entry Count: %d\n\n", (int)(Xsdt->Revision), OemStr, EntryCount);

        Print(L"  Table Revision CreatorID  CreatorRev   OemTableID\n");
        Print(L"  -------------------------------------------------\n");
    } else {
        Print(L"\n");
    }

    EntryPtr = (UINT64 *)(Xsdt + 1);
    for (int Index = 0; Index < EntryCount; Index++, EntryPtr++) {
        if ( Verbose ) {
            PrintTableEntry((EFI_ACPI_SDT_HEADER *)((UINTN)(*EntryPtr)));
        } else {
            PrintListEntry((EFI_ACPI_SDT_HEADER *)((UINTN)(*EntryPtr)), Index);
        }
    }

    if ( !Verbose ) {
        Print(L"\n");
    }
    Print(L"\n");

    return;
}


VOID
Usage( BOOLEAN ErrorMsg )
{
    if ( ErrorMsg ) {
        Print(L"ERROR: Unknown option.\n");
    }
    Print(L"Usage: ShowACPI [-v | --verbose]\n");
    Print(L"       ShowACPI [-V | --version]\n");
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
    BOOLEAN    Verbose = FALSE;
    CHAR16     GuidStr[100];

    if (Argc == 2) {
        if (!StrCmp(Argv[1], L"--verbose") ||
            !StrCmp(Argv[1], L"-v")) {
            Verbose = TRUE;
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


    // locate RSDP (Root System Description Pointer) 
    for (int i = 0; i < gST->NumberOfTableEntries; i++) {
        if ((CompareGuid (&(gST->ConfigurationTable[i].VendorGuid), &gAcpi20TableGuid)) ||
            (CompareGuid (&(gST->ConfigurationTable[i].VendorGuid), &gAcpi10TableGuid))) {
            if (!AsciiStrnCmp("RSD PTR ", (CHAR8 *)(ect->VendorTable), 8)) {
                UnicodeSPrint(GuidStr, sizeof(GuidStr), L"%g", &(gST->ConfigurationTable[i].VendorGuid));
                Rsdp = (EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER *)ect->VendorTable;
                ParseRSDP(Rsdp, GuidStr, Verbose); 
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
