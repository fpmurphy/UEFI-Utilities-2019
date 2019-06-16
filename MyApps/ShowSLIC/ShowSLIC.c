//
//  Copyright (c) 2018-2019  Finnbarr P. Murphy.   All rights reserved.
//
//  Display ACPI SLIC (System Licensed Internal Code) table 
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

#define UTILITY_VERSION L"20190615"
#undef DEBUG

#pragma pack(1)
// OEM Public Key
typedef struct {
    UINT32    Type;
    UINT32    Length;
    UINT8     KeyType;
    UINT8     Version;
    UINT16    Reserved;
    UINT32    Algorithm;
    CHAR8     Magic[4];
    UINT32    BitLength;
    UINT32    Exponent;
    UINT8     Modulus[128];
} OEM_PUBLIC_KEY;

// Windows Marker
typedef struct {
    UINT32    Type;
    UINT32    Length;
    UINT32    Version;   
    CHAR8     OemId[6];
    CHAR8     OemTableId[8];
    CHAR8     Product[8];
    UINT16    MinorVersion;
    UINT16    MajorVersion;
    UINT8     Signature[144];
} WINDOWS_MARKER;
 
// Software Licensing
typedef struct {
    EFI_ACPI_SDT_HEADER Header;
    OEM_PUBLIC_KEY      PubKey;
    WINDOWS_MARKER      Marker;
} EFI_ACPI_SLIC;
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
    Print(L"    OEM Revision      : 0x%08x (%d)\n", Ptr->OemRevision,
                                                    Ptr->OemRevision);
    AsciiToUnicodeSize((CHAR8 *)&(Ptr->CreatorId), 4, Buffer, TRUE);
    Print(L"    Creator ID        : %s\n", Buffer);
    Print(L"    Creator Revision  : 0x%08x (%d)\n", Ptr->CreatorRevision, 
                                                    Ptr->CreatorRevision);
    Print(L"\n");
}


VOID
PrintOemPublicKey( OEM_PUBLIC_KEY *Ptr,
                   BOOLEAN Verbose )
{
    CHAR16 Buffer[50];

    Print(L"  OEM Public Key\n");
    Print(L"    Type              : 0x%08x (%d)\n", Ptr->Type, Ptr->Type);
    Print(L"    Length            : 0x%08x (%d)\n", Ptr->Length, Ptr->Length);
    Print(L"    KeyType           : 0x%02x (%d)\n", Ptr->KeyType, Ptr->KeyType);
    Print(L"    Version           : 0x%02x (%d)\n", Ptr->Version, Ptr->Version);
    Print(L"    Reserved          : 0x%04x (%d)\n", Ptr->Reserved, Ptr->Reserved);
    Print(L"    Algorithm         : 0x%08x (%d)\n", Ptr->Algorithm, Ptr->Algorithm);
    AsciiToUnicodeSize((CHAR8 *)&(Ptr->Magic), 4, Buffer, TRUE);
    Print(L"    Magic             : %s\n", Buffer);
    Print(L"    Bit Length        : 0x%08x (%d)\n", Ptr->BitLength, Ptr->BitLength);
    Print(L"    Exponent          : 0x%08x (%d)\n", Ptr->Exponent, Ptr->Exponent);
    if (Verbose) {
        Print(L"    Modulus:\n");
        PrintHexTable( (UINT8 *)&(Ptr->Modulus), (int)(Ptr->BitLength)/8, TRUE );
    }
    Print(L"\n");
}


VOID
PrintWindowsMarker( WINDOWS_MARKER *Ptr, 
                    BOOLEAN Verbose )
{
    CHAR16 Buffer[50];

    Print(L"  Windows Marker\n");
    Print(L"    Type              : 0x%08x (%d)\n", Ptr->Type, Ptr->Type);
    Print(L"    Length            : 0x%08x (%d)\n", Ptr->Length, Ptr->Length);
    Print(L"    Version           : 0x%02x (%d)\n", Ptr->Version, Ptr->Version);
    AsciiToUnicodeSize((CHAR8 *)(Ptr->OemId), 6, Buffer, TRUE);
    Print(L"    OEM ID            : %s\n", Buffer);
    AsciiToUnicodeSize((CHAR8 *)(Ptr->OemTableId), 8, Buffer, TRUE);
    Print(L"    OEM Table ID      : %s\n", Buffer);
    AsciiToUnicodeSize((CHAR8 *)(Ptr->Product), 8, Buffer, TRUE);
    Print(L"    Windows Flag      : %s\n", Buffer);
    Print(L"    SLIC Version      : 0x%04x%04x (%d.%d)\n", Ptr->MajorVersion, Ptr->MinorVersion,
                                                           Ptr->MajorVersion, Ptr->MinorVersion);
    if (Verbose) {
        Print(L"    Signature:\n");
        PrintHexTable( (UINT8 *)&(Ptr->Signature), 144, TRUE ); 
    }
    Print(L"\n");
}


VOID 
PrintSLIC( EFI_ACPI_SLIC *Slic, 
           BOOLEAN Verbose,
           BOOLEAN Hexdump )
{
    Print(L"\n");
    if (Hexdump) {
        PrintHexTable( (UINT8 *)Slic, sizeof(EFI_ACPI_SLIC), FALSE );
        Print(L"\n");
    } else {
        PrintAcpiHeader( (EFI_ACPI_SDT_HEADER *)&(Slic->Header) );
        PrintOemPublicKey( (OEM_PUBLIC_KEY *)&(Slic->PubKey), Verbose );
        PrintWindowsMarker( (WINDOWS_MARKER *)&(Slic->Marker), Verbose );
    }
}


VOID
ParseRSDP( EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER *Rsdp, 
           CHAR16* GuidStr, 
           BOOLEAN Verbose,
           BOOLEAN Hexdump )
{
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

    EntryPtr = (UINT64 *)(Xsdt + 1);
    for (int Index = 0; Index < EntryCount; Index++, EntryPtr++) {
        Entry = (EFI_ACPI_SDT_HEADER *)((UINTN)(*EntryPtr));
        if (Entry->Signature == SIGNATURE_32 ('S', 'L', 'I', 'C')) {
            PrintSLIC((EFI_ACPI_SLIC *)((UINTN)(*EntryPtr)), Verbose, Hexdump);
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
    Print(L"Usage: ShowSLIC [-v | --verbose]\n");
    Print(L"       ShowSLIC [-d | --dump]\n");
    Print(L"       ShowSLIC [-V | --version]\n");
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
        if (!StrCmp(Argv[1], L"--dump") ||
            !StrCmp(Argv[1], L"-d")) {
            Hexdump = TRUE;
        } else if (!StrCmp(Argv[1], L"--verbose") ||
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
