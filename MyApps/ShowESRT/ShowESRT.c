//
//  Copyright (c) 2015-2019  Finnbarr P. Murphy.   All rights reserved.
//
//  Display ESRT (EFI System Resource Table)
//
//  License: EDKII license applies to code from EDKII source,
//           BSD 2 clause license applies to all other code.
//

#include <Uefi.h>
#include <Uefi/UefiSpec.h>

#include <Library/UefiLib.h>
#include <Library/ShellCEntryLib.h>
#include <Library/ShellLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/LoadedImage.h>

#include <Guid/SystemResourceTable.h>

#define UTILITY_VERSION L"20190615"
#undef DEBUG

// for option setting
typedef enum {
   Verbose = 1,
   Hexdump,
} MODE;


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
DumpEsrt( VOID *data,
          MODE Mode )
{
    EFI_SYSTEM_RESOURCE_TABLE *Esrt = data;
    EFI_SYSTEM_RESOURCE_ENTRY *EsrtEntry = (EFI_SYSTEM_RESOURCE_ENTRY *)((UINT8 *)data + sizeof (*Esrt));
    BOOLEAN ob;
    UINT16  PrivateFlags;

    Print(L"\n");
    if ( Mode == Hexdump ) {
        PrintHexTable( (UINT8 *)Esrt, (int)(sizeof(EFI_SYSTEM_RESOURCE_TABLE) \
                                      + ((Esrt->FwResourceCount) * sizeof(EFI_SYSTEM_RESOURCE_ENTRY))), 
                       FALSE );
        Print(L"\n");
        return;
    }

    if ( Mode == Verbose ) {
        Print(L"  ESRT found at 0x%08x\n", data);
        Print(L"  Firmware Resource Count: %d\n", Esrt->FwResourceCount);
        Print(L"  Firmware Resource Max Count: %d\n", Esrt->FwResourceCountMax);
        Print(L"  Firmware Resource Version: %ld\n", Esrt->FwResourceVersion);
        Print(L"\n");
    }

    if (Esrt->FwResourceVersion != 1) {
        Print(L"ERROR: Unsupported ESRT version: %d\n", Esrt->FwResourceVersion);
        return;
    }

    for (int i = 0; i < Esrt->FwResourceCount; i++) {
        ob = FALSE;
        Print(L"  Firmware Resource Entry: %d\n", i);
        Print(L"  Firmware Class GUID: %g\n", &EsrtEntry->FwClass);
        Print(L"  Firmware Type: %d ", EsrtEntry->FwType);
        switch (EsrtEntry->FwType) {
            case 0:  Print(L"(Unknown)\n");
                     break;
            case 1:  Print(L"(System)\n");
                     break;
            case 2:  Print(L"(Device)\n");
                     break;
            case 3:  Print(L"(UEFI Driver)\n");
                     break;
            default: Print(L"\n");
        }
        Print(L"  Firmware Version: 0x%08x\n", EsrtEntry->FwVersion);
        Print(L"  Lowest Supported Firmware Version: 0x%08x\n", EsrtEntry->LowestSupportedFwVersion);

        Print(L"  Capsule Flags: 0x%08x", EsrtEntry->CapsuleFlags);
        PrivateFlags = (EsrtEntry->CapsuleFlags) &= 0xffff;
        if ( EsrtEntry->CapsuleFlags & CAPSULE_FLAGS_PERSIST_ACROSS_RESET ) {
            if (!ob) {
                ob = TRUE;
                Print(L" (");
            }
            Print(L"Persist Across Reboot");
        }
        if ( EsrtEntry->CapsuleFlags & CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE ) {
            if (!ob) {
                ob = TRUE;
                Print(L" (");
            } else 
                Print(L", ");
            Print(L"Populate System Table");
        }
        if ( EsrtEntry->CapsuleFlags & CAPSULE_FLAGS_INITIATE_RESET ) {
            if (!ob) {
                ob = TRUE;
                Print(L" (");
            } else 
                Print(L", ");
            Print(L"Initiate Reset");
        }
        if ( PrivateFlags ) {
            if (!ob) {
                ob = TRUE;
                Print(L" (");
            } else 
                Print(L", ");
            Print(L"Private Update Flags: 0x%04x", PrivateFlags);
        }
        if (ob) 
            Print(L")");
        Print(L"\n");
        Print(L"  Last Attempt Version: 0x%08x\n", EsrtEntry->LastAttemptVersion);
        Print(L"  Last Attempt Status: %d ", EsrtEntry->LastAttemptStatus);
        switch(EsrtEntry->LastAttemptStatus) {
            case 0:  Print(L"(Success)\n");
                     break;
            case 1:  Print(L"(Unsuccessful)\n");
                     break;
            case 2:  Print(L"(Insufficient Resources)\n");
                     break;
            case 3:  Print(L"(Incorrect Version)\n");
                     break;
            case 4:  Print(L"(Invalid Image Format)\n");
                     break;
            case 5:  Print(L"(Authentication Error)\n");
                     break;
            case 6:  Print(L"(AC Power Not Connected)\n");
                     break;
            case 7:  Print(L"(Insufficent Battery Power)\n");
                     break;
            default: Print(L"(Unknown)\n");
        }
        Print(L"\n");
        EsrtEntry++;
    }
}


VOID
Usage( BOOLEAN ErrorMsg )
{
    if ( ErrorMsg ) {
        Print(L"ERROR: Unknown option.\n");
    }
    Print(L"Usage: ShowESRT [-v | --verbose]\n");
    Print(L"       ShowESRT [-d | --dump]\n");
    Print(L"       ShowESRT [-V | --version]\n");
}


INTN
EFIAPI
ShellAppMain( UINTN Argc, 
              CHAR16 **Argv )
{
    EFI_CONFIGURATION_TABLE *ect = gST->ConfigurationTable;
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_GUID   EsrtGuid = EFI_SYSTEM_RESOURCE_TABLE_GUID;
    MODE       Mode = 0;

    if (Argc == 2) {
        if (!StrCmp(Argv[1], L"--verbose") ||
            !StrCmp(Argv[1], L"-v")) {
            Mode = Verbose;
        } else if (!StrCmp(Argv[1], L"--dump") ||
            !StrCmp(Argv[1], L"-d")) {
            Mode = Hexdump;
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

    for (int Index = 0; Index < gST->NumberOfTableEntries; Index++) {
        if (!CompareMem(&ect->VendorGuid, &EsrtGuid, sizeof(EsrtGuid))) {
            DumpEsrt( ect->VendorTable, Mode );
            return EFI_SUCCESS;
        }
        ect++;
        continue;
    }

    Print(L"No ESRT found\n");

    return Status;
}

