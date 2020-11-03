//
//  Copyright (c) 2012-2020  Finnbarr P. Murphy.   All rights reserved.
//
//  Show BootXXXX variables
//
//  License: EDKII license applies to code from EDKII source,
//           BSD 2 clause license applies to all other code.
//

#include <Uefi.h>
#include <Uefi/UefiSpec.h>

#include <Library/UefiLib.h>
#include <Library/ShellCEntryLib.h>
#include <Library/ShellLib.h>
#include <Library/PrintLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Protocol/LoadedImage.h>

#include <Guid/SystemResourceTable.h>
#include <Guid/GlobalVariable.h>

#define UTILITY_VERSION L"20201002"
#undef DEBUG

// for option setting
typedef enum {
    Default = 0,
    All 
} MODE;


EFI_STATUS
GetNvramVariable( CHAR16   *VariableName,
                  EFI_GUID *VariableOwnerGuid, 
                  VOID     **Buffer,
                  UINTN    *BufferSize)
{
    EFI_STATUS Status;
    UINTN Size = 0;

    *BufferSize = 0;

    Status = gRT->GetVariable( VariableName, VariableOwnerGuid, NULL, &Size, NULL );
    if (Status != EFI_BUFFER_TOO_SMALL)
        return Status;

    *Buffer = AllocateZeroPool( Size );
    if (!Buffer)
        return EFI_OUT_OF_RESOURCES;

    Status = gRT->GetVariable( VariableName, VariableOwnerGuid, NULL, &Size, *Buffer );
    if (Status != EFI_SUCCESS) {
        FreePool( *Buffer );
        *Buffer = NULL;
    } else 
        *BufferSize = Size;

    return Status;
}


EFI_STATUS
OutputBootVariable( CHAR16   *VariableNane,
                    EFI_GUID VariableOwnerGuid,
                    MODE     Mode )
{
    EFI_LOAD_OPTION *LoadOption;
    EFI_STATUS      Status = EFI_SUCCESS;
    UINT8           *Buffer;
    UINTN           BufferSize;
    CHAR16          *Description = (CHAR16 *)NULL;
    UINTN           DescriptionSize;
    CHAR16          *DevPathString = (CHAR16 *)NULL;
    VOID            *FilePathList;

    Status = GetNvramVariable( VariableNane, &VariableOwnerGuid, (VOID *)&Buffer, &BufferSize );
    if (Status == EFI_SUCCESS) {
        LoadOption      = (EFI_LOAD_OPTION *)Buffer;
        Description     = (CHAR16*)(Buffer + sizeof (EFI_LOAD_OPTION));
        DescriptionSize = StrSize( Description );
          
        if (Mode == Default ) {
            if ((LoadOption->Attributes & LOAD_OPTION_HIDDEN) != 0) {
                return Status;
            }
        }
        if (LoadOption->FilePathListLength != 0) {
            FilePathList = (UINT8 *)Description + DescriptionSize;
            DevPathString = ConvertDevicePathToText(FilePathList, TRUE, FALSE);
        }

        Print(L"[%0x] %s: %s %s\n", LoadOption->Attributes, VariableNane, Description, DevPathString ); 
        FreePool( Buffer );
    } else if (Status == EFI_NOT_FOUND) {
        Print(L"Variable %s not found.\n", VariableNane);
    } else
        Print(L"ERROR: Failed to get variable %s. Status Code: %d\n", VariableNane, Status);

    return Status;
}


EFI_STATUS
GetBootOrderVariable( VOID **Buffer, 
                      UINTN *BufferSize )
{
   EFI_STATUS Status = EFI_SUCCESS;
   EFI_GUID   Guid = EFI_GLOBAL_VARIABLE;

   Status = GetNvramVariable( L"BootOrder", &Guid, Buffer, BufferSize );

   return Status;
}


VOID
Usage( BOOLEAN ErrorMsg )
{
    if ( ErrorMsg ) {
        Print(L"ERROR: Unknown option.\n");
    }
    Print(L"Usage: ShowBoot [-a | --all]\n");
    Print(L"       ShowBoot [-V | --version]\n");
}


INTN
EFIAPI
ShellAppMain( UINTN Argc, 
              CHAR16 **Argv )
{
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_GUID   Guid = EFI_GLOBAL_VARIABLE;
    UINT16     *BootOrderList = (UINT16 *)NULL;
    UINT16     BootString[10];
    UINTN      BootOrderListSize = 0;
    UINTN      Index;
    MODE       Mode = Default;
    
    if (Argc == 2) {
        if (!StrCmp(Argv[1], L"--all") ||
            !StrCmp(Argv[1], L"-a")) {
            Mode = All;
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

    Status = GetBootOrderVariable( (VOID **) &BootOrderList, &BootOrderListSize );
    if (Status != EFI_SUCCESS) {
        FreePool( BootOrderList );
        Print(L"ERROR: Failed to find BootOrder variable [%d]\n", Status );
        return Status;
    }

    for ( Index = 0; Index < BootOrderListSize / sizeof(UINT16); Index++ ) {
        UnicodeSPrint( BootString, sizeof (BootString), L"Boot%04x", BootOrderList[Index] );
#ifdef DEBUG
        Print(L"%d: %s\n", Index, BootString );
#else
        Status = OutputBootVariable( BootString, Guid, Mode );
#endif
    }

    FreePool( BootOrderList );

    return Status;
}
