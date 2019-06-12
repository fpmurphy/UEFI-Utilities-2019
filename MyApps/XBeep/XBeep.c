//
//  Copyright (c) 2018 - 2019   Finnbarr P. Murphy.   All rights reserved.
//
//  Beep 1 or more times. Options to vary frequency, duration, interval, and more. 
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

// Piezo speaker or PIT related
#define EFI_SPEAKER_CONTROL_PORT       0x61
#define EFI_SPEAKER_OFF_MASK           0xFC
#define EFI_TIMER_CONTROL_PORT         0x43
#define EFI_TIMER_CONTROL2_PORT        0x42

#define EFI_DEFAULT_BEEP_NUMBER        1 
#define EFI_DEFAULT_BEEP_ON_TIME       0x50000
#define EFI_DEFAULT_BEEP_OFF_TIME      0x50000
#define EFI_DEFAULT_BEEP_FREQUENCY     0x500
#define EFI_DEFAULT_BEEP_ALTFREQUENCY  0x1500

#define I8254_CTRL_SEL_CTR(x)          ((x) << 6)
#define I8254_CTRL_RW(x)               (((x) & 0x3) << 4)
#define I8254_CTRL_LATCH               I8254_CTRL_RW(0)
#define I8254_CTRL_REG                 0x03
#define I8254_CTRL_LSB_MSB             I8254_CTRL_RW(3)

#define UTILITY_VERSION L"20190612"
#undef DEBUG

// global
UINT16 CountdownValue;


BOOLEAN
IsHexNumber( CHAR16* str )
{
    CHAR16 *s = str;

    // allow negative
    if (*s == L'-')
       s++;

    if (*s && (*s == L'0')) {
        s++;
        if (*s && (*s == L'x' || *s == L'X')) {
            s++;
        } else {
            return FALSE;
        }
    } else {
        return FALSE;
    }

    while (*s) {
        if (! ((*s >= L'0') && (*s <= L'9')) || 
              ((*s >= L'A') && (*s <= L'F')) ||
              ((*s >= L'a') && (*s <= L'f'))) { 
            return FALSE;
        }
        s++;
    }

    return TRUE;
}


BOOLEAN
IsDecNumber( CHAR16* str )
{
    CHAR16 *s = str;

    // allow negative
    if (*s == L'-')
       s++;

    if (*s == CHAR_NULL) {
        return FALSE;
    }

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


#ifdef DEBUG
// FPM - WARNING: Probably wrong. Inconsident values returned. Not sure why yet!
UINT16
ReadCounter(UINT32 Counter)
{
    UINT16 Val = 0;
    UINT8  Data;

    if (Counter > 2)
       return 0;

    Data = I8254_CTRL_SEL_CTR(Counter) | I8254_CTRL_LATCH; 
    IoWrite8(EFI_TIMER_CONTROL_PORT, Data);                    // 43

    Data = I8254_CTRL_SEL_CTR(Counter) | I8254_CTRL_RW(Counter) | I8254_CTRL_LSB_MSB ; 
    // Data = I8254_CTRL_SEL_CTR(Counter) | I8254_CTRL_READBACK_STATUS ; 
    IoWrite8(EFI_TIMER_CONTROL_PORT, Data);                    // 43

    Val =  IoRead8( EFI_TIMER_CONTROL2_PORT );                 // 42 LSB
    // Val |= (IoRead8( EFI_TIMER_CONTROL2_PORT ) << 8);          // 42 MSB
    Val += (IoRead8( EFI_TIMER_CONTROL2_PORT ) << 8);          // 42 MSB

    return Val;
}
#endif


EFI_STATUS
ChangeFrequency( UINT16 Frequency )
{
    UINT8 Data;

    Data = 0xB6;
    IoWrite8(EFI_TIMER_CONTROL_PORT, Data);

    Data = (UINT8)(Frequency & 0x00FF);
    IoWrite8( EFI_TIMER_CONTROL2_PORT, Data );
    Data = (UINT8)((Frequency & 0xFF00) >> 8);
    IoWrite8( EFI_TIMER_CONTROL2_PORT, Data );
    
    return EFI_SUCCESS;
}


EFI_STATUS
ResetFrequency( VOID )
{
    EFI_STATUS Status;

    Status = ChangeFrequency( EFI_DEFAULT_BEEP_FREQUENCY );
    
    return Status;
}


VOID
SimpleBeep( BOOLEAN TwoTone ) 
{
    TurnOnSpeaker();
    gBS->Stall( EFI_DEFAULT_BEEP_ON_TIME );
    TurnOffSpeaker();
    gBS->Stall( EFI_DEFAULT_BEEP_OFF_TIME );
    if (TwoTone) {
        // CountdownValue = ReadCounter(2);
        ChangeFrequency( EFI_DEFAULT_BEEP_ALTFREQUENCY );
        TurnOnSpeaker();
        gBS->Stall( EFI_DEFAULT_BEEP_ON_TIME );
        TurnOffSpeaker();
        // gBS->Stall( EFI_DEFAULT_BEEP_OFF_TIME );
        ResetFrequency();
    }  
}


EFI_STATUS
ComplexBeep( UINTN NumBeeps,
             UINTN Interval,
             UINTN Duration,
             UINTN Frequency,
             UINTN AltFreq,
             BOOLEAN TwoTone )
{
#ifdef DEBUG
    CountdownValue = ReadCounter(2);
#endif

    ChangeFrequency( Frequency );
    for ( UINTN Num = 0; Num < NumBeeps; Num++ ) {
        TurnOnSpeaker();
        gBS->Stall( Duration );
        TurnOffSpeaker();
        gBS->Stall( Interval );
        if (TwoTone) {
            ChangeFrequency( AltFreq );
            TurnOnSpeaker();
            gBS->Stall( Duration );
            TurnOffSpeaker();
            if (Num < NumBeeps) {
               gBS->Stall( Interval );
               ChangeFrequency( Frequency );
            }
        }  
    }
    ResetFrequency();

    return EFI_SUCCESS;
}


VOID
PrintDefaults( VOID )
{
    Print(L"\n");
    Print(L"                   Default beep count: %d\n",   EFI_DEFAULT_BEEP_NUMBER ); 
    Print(L"            Default beep enabled time: 0x%x\n", EFI_DEFAULT_BEEP_ON_TIME ); 
    Print(L"           Default beep disabled time: 0x%x\n", EFI_DEFAULT_BEEP_OFF_TIME );
    Print(L"               Default beep frequency: 0x%x\n", EFI_DEFAULT_BEEP_FREQUENCY );
    Print(L"   Default beep alternative frequency: 0x%x\n", EFI_DEFAULT_BEEP_ALTFREQUENCY );
    Print(L"\n");
}


VOID
Usage( BOOLEAN ErrorMsg )
{
    if ( ErrorMsg ) {
        Print(L"ERROR: Unknown option(s).\n");
    }
    Print(L"Usage: XBeep [-T] [-n number ] [-d hexduration] [-i hexinteravl] [-f hexfrequency] [-a hexaltfreq]\n");
    Print(L"       XBeep -D | --defaults\n");
    Print(L"       XBeep -T | --twotone\n");
    Print(L"       XBeep -V | --version\n");
}


INTN
EFIAPI
ShellAppMain( UINTN Argc, 
              CHAR16 **Argv )
{
    EFI_STATUS Status = EFI_SUCCESS;
    UINTN NumBeeps  = EFI_DEFAULT_BEEP_NUMBER;
    UINTN Duration  = EFI_DEFAULT_BEEP_ON_TIME;
    UINTN Interval  = EFI_DEFAULT_BEEP_OFF_TIME;
    UINTN Frequency = EFI_DEFAULT_BEEP_FREQUENCY;
    UINTN AltFreq   = EFI_DEFAULT_BEEP_ALTFREQUENCY;
    BOOLEAN TwoTone = FALSE;
#ifdef DEBUG
    UINT32 Val = 0;
#endif

    if (Argc == 1) {
       SimpleBeep(FALSE);
       return Status;
    } else if (Argc == 2) {
        if (!StrCmp(Argv[1], L"--version") ||
            !StrCmp(Argv[1], L"-V")) {
            Print(L"Version: %s\n", UTILITY_VERSION);
        } else if (!StrCmp(Argv[1], L"--help") ||
            !StrCmp(Argv[1], L"-h")) {
            Usage(FALSE);
        } else if (!StrCmp(Argv[1], L"--defaults") ||
            !StrCmp(Argv[1], L"-D")) {
            PrintDefaults();
            return Status;
        } else if (!StrCmp(Argv[1], L"--twotone") ||
            !StrCmp(Argv[1], L"-T")) {
            SimpleBeep(TRUE);
            return Status;
#ifdef DEBUG
        } else if (!StrCmp(Argv[1], L"-R")) {      // undocumented experimental option
            Val = ReadCounter(2);
            Print(L"Timer 3 count value: %d %x\n", Val, Val);
            return Status;
#endif
        } else {
            Usage(TRUE);
        }
        return Status;
    }

    for (int i = 1; i < Argc; i++) {
        if (!StrCmp(Argv[i], L"-n")) {
            if (i < Argc && IsDecNumber(Argv[i+1])) {
               i++;
               NumBeeps = (UINTN) StrDecimalToUint64( Argv[i] );            
               if (NumBeeps < 1) {
                   Print(L"ERROR: Invalid number of beeps entered.\n");               
                   Usage(FALSE);
                   return Status;
               }
            } else {
                Print(L"ERROR: Invalid or missing argument to option.\n");               
                Usage(FALSE);
                return Status;
            }
        } else if (!StrCmp(Argv[i], L"-d")) {
            if (i < Argc && IsHexNumber(Argv[i+1])) {
               i++;
               Duration = (UINTN) StrHexToUint64( Argv[i] );            
               if (Duration < 1) {
                   Print(L"ERROR: Invalid duration entered.\n");               
                   Usage(FALSE);
                   return Status;
               }
            } else {
                Print(L"ERROR: Invalid or missing argument to option.\n");               
                Usage(FALSE);
                return Status;
            }
        } else if (!StrCmp(Argv[i], L"-i")) {
            if (i < Argc && IsHexNumber(Argv[i+1])) {
               i++;
               Interval = (UINTN) StrHexToUint64( Argv[i] );            
               if (Interval < 1) {
                   Print(L"ERROR: Invalid interval entered.\n");               
                   Usage(FALSE);
                   return Status;
               }
            } else {
                Print(L"ERROR: Invalid or missing argument to option.\n");               
                Usage(FALSE);
                return Status;
            }
        } else if (!StrCmp(Argv[i], L"-f")) {
            if (i < Argc && IsHexNumber(Argv[i+1])) {
               i++;
               Frequency = (UINTN) StrHexToUint64( Argv[i] );            
               if (Frequency < 1) {
                   Print(L"ERROR: Invalid frequency entered.\n");               
                   Usage(FALSE);
                   return Status;
               }
            } else {
                Print(L"ERROR: Invalid or missing argument to option.\n");               
                Usage(FALSE);
                return Status;
            }
        } else if (!StrCmp(Argv[i], L"-a")) {
            if (i < Argc && IsHexNumber(Argv[i+1])) {
               i++;
               AltFreq = (UINTN) StrHexToUint64( Argv[i] );            
               if (AltFreq < 1) {
                   Print(L"ERROR: Invalid alternative frequency entered.\n");               
                   Usage(FALSE);
                   return Status;
               }
            } else {
                Print(L"ERROR: Invalid or missing argument to option.\n");               
                Usage(FALSE);
                return Status;
            }
        } else if (!StrCmp(Argv[i], L"-T")) {
             TwoTone = TRUE; 
        } else {
            Usage(TRUE);
            return Status;
        }
    }


    Status = ComplexBeep( NumBeeps, Interval, Duration, Frequency, AltFreq, TwoTone );

    return Status;
}
