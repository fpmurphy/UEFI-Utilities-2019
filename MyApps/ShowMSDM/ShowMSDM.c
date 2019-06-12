//
//  Copyright (c) 2014-2019  Finnbarr P. Murphy.  All rights reserved.
//
//  Display ACPI MSDM and/or Microsoft Windows License Key
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

#define UTILITY_VERSION L"20190611"
#undef DEBUG

 
#pragma pack(1)
typedef struct {
    UINT32   Version;
    UINT32   Reserved;
    UINT32   DataType;
    UINT32   DataReserved;
    UINT32   DataLength;
    CHAR8    Data[30];
} SOFTWARE_LICENSING;

// Microsoft Data Management table structure
typedef struct {
    EFI_ACPI_SDT_HEADER Header;
    SOFTWARE_LICENSING  SoftLic;
} EFI_ACPI_MSDM;
#pragma pack()


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
PrintHexTable( UINT8 *ptr,
               int Count )
{
    int i = 0;

    Print(L"  ");
    for (i = 0; i < Count; i++ ) {
        if ( i > 0 && i%16 == 0)
            Print(L"\n  ");
        Print(L"0x%02x ", 0xff & *ptr++);
    }
    Print(L"\n");
}


VOID
PrintAcpiHeader( EFI_ACPI_SDT_HEADER *Ptr )
{
    CHAR16 Buffer[50];

    Print(L"  ACPI Standard Header\n");
    AsciiToUnicodeSize((CHAR8 *)&(Ptr->Signature), 4, Buffer, TRUE);
    Print(L"    Signature         : %s\n", Buffer);
    Print(L"    Length            : 0x%08x (%d)\n", Ptr->Length, Ptr->Length);
    Print(L"    Revision          : 0x%02x (%d)\n", Ptr->Revision, Ptr->Revision);
    Print(L"    Checksum          : 0x%02x (%d)\n", Ptr->Checksum, Ptr->Checksum);
    AsciiToUnicodeSize((CHAR8 *)&(Ptr->OemId), 6, Buffer, TRUE);
    Print(L"    OEM ID            : %s\n", Buffer);
    AsciiToUnicodeSize((CHAR8 *)&(Ptr->OemTableId), 8, Buffer, TRUE);
    Print(L"    OEM Table ID      : %s\n", Buffer);
    Print(L"    OEM Revision      : 0x%08x (%d)\n", Ptr->OemRevision, Ptr->OemRevision);
    AsciiToUnicodeSize((CHAR8 *)&(Ptr->CreatorId), 4, Buffer, TRUE);
    Print(L"    Creator ID        : %s\n", Buffer);
    Print(L"    Creator Revision  : 0x%08x (%d)\n", Ptr->CreatorRevision, Ptr->CreatorRevision);
    Print(L"\n");
}


VOID
PrintSoftwareLicensing( SOFTWARE_LICENSING *Ptr,
                        BOOLEAN Verbose )
{
    CHAR16 Buffer[50];

    if (Verbose) {
        Print(L"  Software Licensing\n");
        Print(L"    Version       : 0x%08x (%d)\n", Ptr->Version, Ptr->Version);
        Print(L"    Reserved      : 0x%08x (%d)\n", Ptr->Reserved, Ptr->Reserved);
        Print(L"    Data Type     : 0x%08x (%d)\n", Ptr->DataType, Ptr->DataType);
        Print(L"    Data Reserved : 0x%08x (%d)\n", Ptr->DataReserved, Ptr->DataReserved);
        Print(L"    Data Length   : 0x%08x (%d)\n", Ptr->DataLength, Ptr->DataLength);
        AsciiToUnicodeSize((CHAR8 *)(Ptr->Data), 30, Buffer, TRUE);
        Print(L"    Data          : %s\n", Buffer);
    } else {
        AsciiToUnicodeSize((CHAR8 *)(Ptr->Data), 30, Buffer, FALSE);
        Print(L"  %s\n", Buffer);
    }
}


VOID 
PrintMSDM( EFI_ACPI_MSDM *Msdm,
           BOOLEAN Verbose,
           BOOLEAN Hexdump )
{
    Print(L"\n");
    if (Hexdump) {
        PrintHexTable( (UINT8 *)Msdm, (int)(Msdm->Header.Length) );
    } else {
        if (Verbose) {
            PrintAcpiHeader( (EFI_ACPI_SDT_HEADER *)&(Msdm->Header) );
        }
        PrintSoftwareLicensing( (SOFTWARE_LICENSING *)&(Msdm->SoftLic), Verbose);
    }
    Print(L"\n");
}


int
ParseRSDP( EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER *Rsdp, 
           CHAR16* GuidStr, 
           BOOLEAN Verbose,
           BOOLEAN Hexdump )
{
    EFI_ACPI_SDT_HEADER *Xsdt, *Entry;
#ifdef DEBUG 
    CHAR16 OemStr[20];
#endif
    UINT32 EntryCount;
    UINT64 *EntryPtr;

#ifdef DEBUG 
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
        return 1;
    }

    if (Xsdt->Signature != SIGNATURE_32 ('X', 'S', 'D', 'T')) {
#ifdef DEBUG 
        Print(L"ERROR: Invalid ACPI XSDT table found.\n");
#endif
        return 1;
    }

    EntryCount = (Xsdt->Length - sizeof (EFI_ACPI_SDT_HEADER)) / sizeof(UINT64);
#ifdef DEBUG 
    AsciiToUnicodeSize((CHAR8 *)(Xsdt->OemId), 6, OemStr, FALSE);
    Print(L"Found XSDT. OEM ID: %s  Entry Count: %d\n\n", OemStr, EntryCount);
#endif

    EntryPtr = (UINT64 *)(Xsdt + 1);
    for (int Index = 0; Index < EntryCount; Index++, EntryPtr++) {
        Entry = (EFI_ACPI_SDT_HEADER *)((UINTN)(*EntryPtr));
        if (Entry->Signature == SIGNATURE_32 ('M', 'S', 'D', 'M')) {
            PrintMSDM((EFI_ACPI_MSDM *)((UINTN)(*EntryPtr)), Verbose, Hexdump);
        }
    }

    return 0;
}


VOID
Usage( BOOLEAN ErrorMsg )
{
    if ( ErrorMsg ) {
        Print(L"ERROR: Unknown option.\n");
    }

    Print(L"Usage: ShowMSDM [-v | --verbose]\n");
    Print(L"       ShowMSDM [-V | --version]\n");
    Print(L"       ShowMSDM [-d | --dump]\n");
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
    BOOLEAN    Hexdump = FALSE;
    CHAR16     GuidStr[100];

    if (Argc == 2) {
        if (!StrCmp(Argv[1], L"--verbose") ||
            !StrCmp(Argv[1], L"-v")) {
            Verbose = TRUE;
        } else if (!StrCmp(Argv[1], L"--dump") ||
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

    // locate RSDP (Root System Description Pointer) 
    for (int i = 0; i < gST->NumberOfTableEntries; i++) {
        if ((CompareGuid (&(gST->ConfigurationTable[i].VendorGuid), &gAcpi20TableGuid)) ||
            (CompareGuid (&(gST->ConfigurationTable[i].VendorGuid), &gAcpi10TableGuid))) {
            if (!AsciiStrnCmp("RSD PTR ", (CHAR8 *)(ect->VendorTable), 8)) {
                UnicodeSPrint(GuidStr, sizeof(GuidStr), L"%g", &(gST->ConfigurationTable[i].VendorGuid));
                Rsdp = (EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER *)ect->VendorTable;
                ParseRSDP(Rsdp, GuidStr, Verbose, Hexdump); 
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
