//
//  Copyright (c) 2018 - 2019   Finnbarr P. Murphy.   All rights reserved.
//
//  Beep 1 or more times. 
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
#include <Library/IoLib.h>

#include <Protocol/LoadedImage.h>

// Piezo speaker related
#define EFI_SPEAKER_CONTROL_PORT    0x61
#define EFI_SPEAKER_OFF_MASK        0xFC
#define EFI_BEEP_ON_TIME_INTERVAL   0x50000
#define EFI_BEEP_OFF_TIME_INTERVAL  0x50000

#define UTILITY_VERSION L"20190612"
#undef DEBUG


BOOLEAN
IsNumber( CHAR16* str )
{
    CHAR16 *s = str;

    // allow negative
    if (*s == L'-')
       s++;

    while (*s) {
        if (*s  < L'0' || *s > L'9')
            return FALSE;
        s++;
    }

    return TRUE;
}


VOID
TurnOnSpeaker( VOID )
{
    UINT8 Data;

    Data = IoRead8( EFI_SPEAKER_CONTROL_PORT );
    Data |= 0x03;
    IoWrite8( EFI_SPEAKER_CONTROL_PORT, Data) ;
}


VOID
TurnOffSpeaker( VOID )
{
    UINT8 Data;

    Data = IoRead8( EFI_SPEAKER_CONTROL_PORT );
    Data &= EFI_SPEAKER_OFF_MASK;
    IoWrite8( EFI_SPEAKER_CONTROL_PORT, Data );
}


VOID
GenerateBeep( UINTN NumberBeeps )
{
    for ( UINTN Num=0; Num < NumberBeeps; Num++ ) {
        TurnOnSpeaker();
        gBS->Stall( EFI_BEEP_ON_TIME_INTERVAL );
        TurnOffSpeaker();
        gBS->Stall( EFI_BEEP_OFF_TIME_INTERVAL );
    }
}


VOID
Usage( BOOLEAN ErrorMsg )
{
    if ( ErrorMsg ) {
        Print(L"ERROR: Unknown option(s).\n");
    }
    Print(L"Usage: Beep [NumberOfBeeps]\n");
    Print(L"       Beep [-V | --version]\n");
}


INTN
EFIAPI
ShellAppMain( UINTN Argc, 
              CHAR16 **Argv )
{
    EFI_STATUS Status = EFI_SUCCESS;
    UINTN NumberBeeps = 1;

    if (Argc == 2) {
        if (!StrCmp(Argv[1], L"--version") ||
            !StrCmp(Argv[1], L"-V")) {
            Print(L"Version: %s\n", UTILITY_VERSION);
            return Status;
        } else if (!StrCmp(Argv[1], L"--help") ||
            !StrCmp(Argv[1], L"-h")) {
            Usage(FALSE);
            return Status;
        } else if (IsNumber(Argv[1])) {
            NumberBeeps = (UINTN) StrDecimalToUint64( Argv[1] );            
            if (NumberBeeps < 1) {
                Print(L"ERROR: Invalid number of beeps\n");               
                Usage(FALSE);
                return Status;
            } 
        } else {
            Usage(TRUE);
            return Status;
        }
    }
    if (Argc > 2) {
        Usage(TRUE);
        return Status;
    }

    GenerateBeep( NumberBeeps );

    return Status;
}
