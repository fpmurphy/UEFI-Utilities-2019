//
//  Copyright (c) 2019 - 2020   Finnbarr P. Murphy.   All rights reserved.
//
//  Show date and time or GetTime debugging information 
//
//  License: EDKII license applies to code from EDKII source,
//           BSD 2 clause license applies to all other code.
//

#include <Uefi.h>
#include <Uefi/UefiSpec.h>

#include <Library/UefiLib.h>
#include <Library/ShellCEntryLib.h>
#include <Library/ShellLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/PrintLib.h>

#include <Protocol/LoadedImage.h>

#define UTILITY_VERSION L"20200609"
#undef DEBUG


EFI_STATUS
DebugGetTime( VOID )
{
    EFI_TIME_CAPABILITIES TimeCapabilities;
    EFI_TIME              Time;
    EFI_STATUS            Status = EFI_SUCCESS;

    Status = gRT->GetTime(&Time, &TimeCapabilities);
    if (EFI_ERROR(Status)) {
        Print(L"ERROR: GetTime: %d\n", Status);
        return Status;
    }

    Print(L"Year        [%d]\n", Time.Year);
    Print(L"Month       [%02d]\n", Time.Month);
    Print(L"Day         [%02d]\n", Time.Day);
    Print(L"Hour        [%02d]\n", Time.Hour);
    Print(L"Minute      [%02d]\n", Time.Minute);
    Print(L"Second      [%02d]\n", Time.Second);
    Print(L"Nanosecond  [%d]\n", Time.Nanosecond);
    Print(L"TimeZone    [%d]\n", Time.TimeZone);
    Print(L"Daylight    [%d]\n", Time.Daylight);
    Print(L"\n");
    Print(L"Resolution  [%d]\n", TimeCapabilities.Resolution);
    Print(L"Accuracy    [%d]\n", TimeCapabilities.Accuracy);
    Print(L"SetsToZero  [%d]\n", TimeCapabilities.SetsToZero);
    Print(L"\n");
        
    return Status;
}


VOID
Usage( BOOLEAN ErrorMsg )
{
    if ( ErrorMsg ) {
        Print(L"ERROR: Unknown option.\n");
    }
    Print(L"Usage: DateTime [-V | --version]\n");
    Print(L"       DateTime [-d | --debug]\n");
}


INTN
EFIAPI
ShellAppMain( UINTN Argc, 
              CHAR16 **Argv )
{
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_TIME   Time;

    if (Argc == 2) {
        if (!StrCmp(Argv[1], L"--version") ||
            !StrCmp(Argv[1], L"-V")) {
            Print(L"Version: %s\n", UTILITY_VERSION);
            return Status;
        } else if (!StrCmp(Argv[1], L"--debug") ||
            !StrCmp(Argv[1], L"-d")) {
            Status = DebugGetTime();
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

    Status = gRT->GetTime(&Time, NULL);
    if (EFI_ERROR(Status)) {
        Print(L"ERROR: GetTime: %d\n", Status);
        return Status;
    }

    if (Time.TimeZone == EFI_UNSPECIFIED_TIMEZONE) {
        Print(L"%4d-%02d-%02d %02d:%02d:%02d (LOCAL)\n",
            Time.Year, Time.Month, Time.Day,
            Time.Hour, Time.Minute, Time.Second);
    } else {
        Print(L"%4d-%02d-%02d %02d:%02d:%02d (UTC%s%02d:%02d)\n",
            Time.Year, Time.Month, Time.Day,
            Time.Hour, Time.Minute, Time.Second,
            (Time.TimeZone > 0 ? L"-" : L"+"),
            ((ABS(Time.TimeZone)) / 60),
            ((ABS(Time.TimeZone)) % 60));
    }

    return Status;
}
