//
//  Copyright (c) 2017-2020   Finnbarr P. Murphy.   All rights reserved.
//
//  ShowHIIPkg - Show basic information on available HII packages
//
//  License: UDK2017 license applies to code from UDK2017 sources,
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
#include <Protocol/HiiDatabase.h>

#define UTILITY_VERSION L"20200620"
#undef DEBUG


typedef struct {
    UINT8  PackageType;
    CHAR16 *PackageTypeString;
} HII_PACKAGE_TYPE_STRING;

HII_PACKAGE_TYPE_STRING HiiPackageTypeStringTable[] = {
    {EFI_HII_PACKAGE_TYPE_ALL,          L"HII_PACKAGE_TYPE_ALL"},
    {EFI_HII_PACKAGE_TYPE_GUID,         L"HII_PACKAGE_TYPE_GUID"},
    {EFI_HII_PACKAGE_FORMS,             L"HII_PACKAGE_FORMS"},
    {EFI_HII_PACKAGE_STRINGS,           L"HII_PACKAGE_STRINGS"},
    {EFI_HII_PACKAGE_FONTS,             L"HII_PACKAGE_FONTS"},
    {EFI_HII_PACKAGE_IMAGES,            L"HII_PACKAGE_IMAGES"},
    {EFI_HII_PACKAGE_SIMPLE_FONTS,      L"HII_PACKAGE_SIMPLE_FONTS"},
    {EFI_HII_PACKAGE_DEVICE_PATH,       L"HII_PACKAGE_DEVICE_PATH"},
    {EFI_HII_PACKAGE_KEYBOARD_LAYOUT,   L"HII_PACKAGE_KEYBOARD_LAYOUT"},
    {EFI_HII_PACKAGE_ANIMATIONS,        L"HII_PACKAGE_ANIMATIONS"},
    {EFI_HII_PACKAGE_END,               L"HII_PACKAGE_END"},
    {EFI_HII_PACKAGE_TYPE_SYSTEM_BEGIN, L"HII_PACKAGE_TYPE_SYSTEM_BEGIN"},
    {EFI_HII_PACKAGE_TYPE_SYSTEM_END,   L"HII_PACKAGE_TYPE_SYSTEM_END"},
};


CHAR16 *
HiiPackageTypeToString( UINT8 PackageType )
{
    UINTN MaxIndex = ARRAY_SIZE(HiiPackageTypeStringTable);

    for (UINTN Index = 0; Index < MaxIndex; Index++) {
        if (HiiPackageTypeStringTable[Index].PackageType == PackageType) {
            return HiiPackageTypeStringTable[Index].PackageTypeString;
        }
    }

    return L"Unknown Package Type";
}


VOID
DumpHiiPackage( VOID *HiiPackage )
{
    EFI_HII_PACKAGE_HEADER *HiiPackageHeader = (EFI_HII_PACKAGE_HEADER *) HiiPackage;

    Print(L"      Type: 0x%02x (%s)   Length: 0x%06x (%d)\n", HiiPackageHeader->Type,
                                                              HiiPackageTypeToString(HiiPackageHeader->Type),  
                                                              HiiPackageHeader->Length,
                                                              HiiPackageHeader->Length);
}


VOID
DumpHiiDatabase( VOID    *HiiDatabase,
                 UINTN   HiiDatabaseSize,
                 BOOLEAN Terse )
{
    EFI_HII_PACKAGE_LIST_HEADER *HiiPackageListHeader;
    EFI_HII_PACKAGE_HEADER      *HiiPackageHeader;
    CHAR16                      GuidStr[100];
    UINT16                      HiiPackageCount = 0;

    if (Terse)
        Print(L"\n");
    else
        Print(L"\nHII Database Size: 0x%x (%d)\n\n", HiiDatabaseSize, HiiDatabaseSize);

    HiiPackageListHeader = (EFI_HII_PACKAGE_LIST_HEADER *) HiiDatabase;

    while ((UINTN) HiiPackageListHeader < ((UINTN) HiiDatabase + HiiDatabaseSize)) {
        UINTN HiiPackageSize = HiiPackageListHeader->PackageLength;
        if (HiiPackageSize == 0)
            break;
        HiiPackageCount++;
        UnicodeSPrint(GuidStr, sizeof(GuidStr), L"%g", &(HiiPackageListHeader->PackageListGuid));
        Print(L"%02d  GUID: %s   Length: 0x%06x (%d)\n", HiiPackageCount, GuidStr, HiiPackageSize, HiiPackageSize);
        HiiPackageHeader = (EFI_HII_PACKAGE_HEADER *)(HiiPackageListHeader + 1);
        if (!Terse) {
            while ((UINTN) HiiPackageHeader < (UINTN) (HiiPackageListHeader + HiiPackageListHeader->PackageLength)) {
                DumpHiiPackage( HiiPackageHeader );
                if (HiiPackageHeader->Type == EFI_HII_PACKAGE_END)
                   break;
                HiiPackageHeader = (EFI_HII_PACKAGE_HEADER *) ((UINTN) HiiPackageHeader + HiiPackageHeader->Length);
            }
        }
        HiiPackageListHeader = (EFI_HII_PACKAGE_LIST_HEADER *) ((UINTN) HiiPackageListHeader + HiiPackageSize);
    }
}


VOID
Usage( CHAR16 *Str,
       BOOLEAN ErrorMsg )
{
    if ( ErrorMsg ) {
        Print(L"ERROR: Unknown option.\n");
    }
    Print(L"Usage: %s [-V | --version]\n", Str);
    Print(L"       %s [-t | --terse]\n", Str);
}


INTN
EFIAPI
ShellAppMain( UINTN Argc,
              CHAR16 **Argv )
{
    EFI_HII_PACKAGE_LIST_HEADER *PackageList = NULL;
    EFI_HII_DATABASE_PROTOCOL   *HiiDbProtocol;
    EFI_STATUS                  Status = EFI_SUCCESS;
    BOOLEAN                     Terse = FALSE;
    UINTN                       PackageListSize = 0;

    if (Argc == 2) {
        if (!StrCmp(Argv[1], L"--version") ||
            !StrCmp(Argv[1], L"-V")) {
            Print(L"Version: %s\n", UTILITY_VERSION);
            return Status;
        } else if (!StrCmp(Argv[1], L"--help") ||
            !StrCmp(Argv[1], L"-h")) {
            Usage(Argv[0], FALSE);
            return Status;
        } else if (!StrCmp(Argv[1], L"--terse") ||
            !StrCmp(Argv[1], L"-t")) {
            Terse = TRUE;
        } else {
            Usage(Argv[0], TRUE);
            return Status;
        }
    }
    if (Argc > 2) {
        Usage(Argv[0], TRUE);
        return Status;
    }


    Status = gBS->LocateProtocol( &gEfiHiiDatabaseProtocolGuid, 
                                  NULL, 
                                  (VOID **) &HiiDbProtocol );
    if (EFI_ERROR(Status)) {
        Print(L"ERROR: Could not find HII Database protocol\n");
        return Status;
    }

    // Get HII packages
    Status = HiiDbProtocol->ExportPackageLists( HiiDbProtocol,
                                                NULL,
                                                &PackageListSize,
                                                PackageList );
    if (Status != EFI_BUFFER_TOO_SMALL) {
        Print(L"ERROR: Could not obtain package list size\n");
        return Status;
    }

    Status = gBS->AllocatePool( EfiBootServicesData, 
                                PackageListSize, 
                                (VOID **) &PackageList );
    if (EFI_ERROR(Status)) {
        Print(L"ERROR: Could not allocate sufficient memory for package list\n");
        return Status;
    }

    Status = HiiDbProtocol->ExportPackageLists( HiiDbProtocol,
                                                NULL,
                                                &PackageListSize,
                                                PackageList );
    if (EFI_ERROR(Status)) {
        Print(L"ERROR: Could not retrieve the package list\n");
        FreePool(PackageList);
        return Status;
    }

    DumpHiiDatabase( PackageList, PackageListSize, Terse );

    FreePool(PackageList);

    Print(L"\n");

    return Status;
}


