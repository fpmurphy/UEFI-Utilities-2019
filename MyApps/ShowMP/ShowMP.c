//
//  Copyright (c) 2019   Finnbarr P. Murphy.   All rights reserved.
//
//  Show UEFI multi-processor status 
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

#include <Pi/PiDxeCis.h>

#include <Protocol/LoadedImage.h>
#include <Protocol/MpService.h>

#define UTILITY_VERSION L"20190608"
#undef DEBUG



VOID
Usage( BOOLEAN ErrorMsg )
{
    if ( ErrorMsg ) {
        Print(L"ERROR: Unknown option.\n");
    }
    Print(L"Usage: ShowMP [-V | --version]\n");
}


INTN
EFIAPI
ShellAppMain( UINTN Argc, 
              CHAR16 **Argv )
{
    EFI_MP_SERVICES_PROTOCOL  *MpServiceProtocol = NULL;
    EFI_PROCESSOR_INFORMATION ProcessorInfo;
    EFI_STATUS                Status = EFI_SUCCESS;
    EFI_GUID                  gEfiMpServiceProtocolGuid = EFI_MP_SERVICES_PROTOCOL_GUID;
    UINTN                     Proc;
    UINTN                     NumProc;
    UINTN                     NumEnabledProc; 

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

    // Get MP services protocol
    Status = gBS->LocateProtocol( &gEfiMpServiceProtocolGuid,
                                  NULL,
                                  (VOID **)&MpServiceProtocol );
    if (EFI_ERROR(Status)) {
        Print(L"ERROR: Cannot locate MpService procotol: %d\n", Status);
        return Status;
    }

    // Processor counts
    Status = MpServiceProtocol->GetNumberOfProcessors( MpServiceProtocol, 
                                                       &NumProc, 
                                                       &NumEnabledProc );
    if (EFI_ERROR(Status)) {
        Print(L"ERROR: Cannot get processor count: %d\n", Status);
        return Status;
    }

    Print(L"\n");
    Print(L"  Number of processors: %d\n", NumProc);
    Print(L"  Number of enabled processors: %d\n", NumEnabledProc);
    Print(L"\n");
    Print(L"  ProcID   Enabled   Type   Healthy   Pkg  Core  Thread\n");
    Print(L"  -----------------------------------------------------\n");

    for (Proc=0; Proc < NumProc; Proc++) {
        Status = MpServiceProtocol->GetProcessorInfo( MpServiceProtocol, 
                                                      Proc, 
                                                      &ProcessorInfo );
        if (EFI_ERROR(Status)) {
            Print(L"ERROR: Cannot get information for processor: %d [%d]\n", Proc, Status);
            return Status;
        }
		
        Print(L"    %04x      %s      %3s       %s    %4d    %2d      %d\n",
            ProcessorInfo.ProcessorId,
            (ProcessorInfo.StatusFlag & PROCESSOR_ENABLED_BIT) ? L"Y"   : L"N",
            (ProcessorInfo.StatusFlag & PROCESSOR_AS_BSP_BIT)  ? L"BSP" : L" AP",
            (ProcessorInfo.StatusFlag & PROCESSOR_HEALTH_STATUS_BIT) ? L"Y"   : L"N",
            ProcessorInfo.Location.Package,
            ProcessorInfo.Location.Core,
            ProcessorInfo.Location.Thread );
    }
    Print(L"\n");

    return Status;
}
