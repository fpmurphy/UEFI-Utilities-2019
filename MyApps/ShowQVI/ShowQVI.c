//
//  Copyright (c) 2012 - 2020  Finnbarr P. Murphy.   All rights reserved.
//
//  Show EFI Variable Store infomation returned by QueryVariableInformation API 
//
//  License: EDKII license applies to code from EDKII source,
//           BSD 2 clause license applies to all other code.
//

#include <Uefi.h>
#include <Uefi/UefiSpec.h>

#include <Uefi/UefiSpec.h>
#include <Guid/GlobalVariable.h>

#include <Library/UefiLib.h>
#include <Library/ShellCEntryLib.h>
#include <Library/ShellLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Protocol/LoadedImage.h>

#define UTILITY_VERSION L"20201103"
#undef DEBUG


VOID
Usage( BOOLEAN ErrorMsg )
{
    if ( ErrorMsg ) {
        Print(L"ERROR: Unknown option.\n");
    }
    Print(L"Usage: ShowQVI [-V | --version]\n");
}


INTN
EFIAPI
ShellAppMain( UINTN Argc, 
              CHAR16 **Argv )
{
    EFI_STATUS Status = EFI_SUCCESS;
    UINT32     Attributes;
    UINT64     MaxStoreSize = 0;
    UINT64     RemainStoreSize = 0;
    UINT64     MaxVariableSize = 0;

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


    Attributes = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS;

    Status = gRT->QueryVariableInfo( Attributes,
                                     &MaxStoreSize, 
                                     &RemainStoreSize,
                                     &MaxVariableSize );
    if (Status != EFI_SUCCESS) {
        Print(L"ERROR: QueryVariableInfo: %d\n", Status);
        return Status;
    }

    Print(L"\n");
    Print(L"    Maximum Variable Storage Size: 0x%08x [%ld]\n", MaxStoreSize, MaxStoreSize);
    Print(L"  Remaining Variable Storage Size: 0x%08x [%ld]\n", RemainStoreSize, RemainStoreSize);
    Print(L"            Maximum Variable Size: 0x%08x [%ld]\n", MaxVariableSize, MaxVariableSize);
    Print(L"\n");

    return Status;
}
