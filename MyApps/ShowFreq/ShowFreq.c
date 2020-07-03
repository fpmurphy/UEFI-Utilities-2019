//
//  Copyright (c) 2018-2020  Finnbarr P. Murphy.   All rights reserved.
//
//  Show CPU frequency via Intel RDTSC instruction
//
//  License: BSD 2 clause License
//
//  Portions Copyright (c) 2018, Intel Corporation. All rights reserved.
//           See relevant code in EDK11 for exact details
//

#include <Uefi.h>
#include <Uefi/UefiSpec.h>

#include <Library/UefiLib.h>
#include <Library/ShellCEntryLib.h>
#include <Library/ShellLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PrintLib.h>
 
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/LoadedImage.h>

#define UTILITY_VERSION L"20200627"
#undef DEBUG


//
// Based on code found at http://code.google.com/p/my-itoa/
//
int 
Integer2AsciiString( int val, 
                     char* buf )
{
    const unsigned int radix = 10;

    char* p = buf;
    unsigned int a; 
    int len;
    char* b;
    char temp;
    unsigned int u;

    if (val < 0) {
        *p++ = '-';
        val = 0 - val;
    }
    u = (unsigned int)val;
    b = p;

    do {
        a = u % radix;
        u /= radix;
        *p++ = a + '0';
    } while (u > 0);

    len = (int)(p - buf);
    *p-- = 0;

    // swap 
    do {
        temp = *p; *p = *b; *b = temp;
        --p; ++b;
    } while (b < p);

    return len;
}


//
// Based on code found on the Internet (author unknown)
// Search for ftoa implementations
//
int 
Float2AsciiString( float f,
                   char *buffer, 
                   int numdecimals )
{
    int status = 0;
    char *s = buffer;
    long mantissa, int_part, frac_part;
    short exp2;
    char m;

    typedef union {
        long L;
        float F;
    } LF_t;
    LF_t x;

    if (f == 0.0) {           // return 0.00
        *s++ = '0'; *s++ = '.'; *s++ = '0'; *s++ = '0'; 
        *s = 0;
       return status;
    }

    x.F = f;

    exp2 = (unsigned char)(x.L >> 23) - 127;
    mantissa = (x.L & 0xFFFFFF) | 0x800000;
    frac_part = 0;
    int_part = 0;

    if (exp2 >= 31 || exp2 < -23) {
        *s = 0;
        return 1;
    } 

    if (exp2 >= 0) {
        int_part = mantissa >> (23 - exp2);
        frac_part = (mantissa << (exp2 + 1)) & 0xFFFFFF;
    } else {
        frac_part = (mantissa & 0xFFFFFF) >> -(exp2 + 1);
    }

    if (int_part == 0)
       *s++ = '0';
    else {
        Integer2AsciiString(int_part, s);
        while (*s) s++;
    }
    *s++ = '.';

    if (frac_part == 0)
        *s++ = '0';
    else {
        for (m = 0; m < numdecimals; m++) {                       // print BCD
            frac_part = (frac_part << 3) + (frac_part << 1);      // frac_part *= 10
            *s++ = (frac_part >> 24) + '0';
            frac_part &= 0xFFFFFF;
        }
    }
    *s = 0;

    return status;
}


VOID
Ascii2UnicodeString( CHAR8 *String,
                     CHAR16 *UniString )
{
    while (*String != '\0') {
        *(UniString++) = (CHAR16) *(String++);
    }
    *UniString = '\0';
}


CHAR16 *
FreqString( UINT64 Val )
{
    char Str[8];
    static CHAR16 Wstr[8];

    float g1 = (float) Val/1000000000;

    Float2AsciiString(g1, Str, 2);
    Ascii2UnicodeString(Str, Wstr);

    return Wstr;
}


UINT64
Rdtsc(VOID)
{
    UINT32 Hi = 0, Lo = 0;

#if defined(__clang__)
    __asm__ __volatile__ ("rdtsc" : "=a"(Lo), "=d"(Hi));
#elif defined(__GNUC__)
    __asm__ __volatile__ ("rdtsc" : "=a"(Lo), "=d"(Hi));
#else
#error No inline assembly code found.
#endif

    return( ((UINT64)Lo) | (((UINT64)Hi)<<32) );
}
 

VOID
Usage( BOOLEAN ErrorMsg )
{
    if ( ErrorMsg ) {
        Print(L"ERROR: Unknown option.\n");
    }
    Print(L"ShowFreq [-V | --version]\n");
}
 

INTN
EFIAPI
ShellAppMain( UINTN Argc,
              CHAR16 **Argv )
{
    EFI_STATUS Status = EFI_SUCCESS;
    UINT64     Second = 0;
    UINT64     First  = 0;

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

    First = Rdtsc();
    gBS->Stall(1000000);  
    Second = Rdtsc();

#if DEBUG
    Print(L"CPU Frequency: %ld Hz\n", (Second - First));
#else
    Print(L"CPU Frequency: %s GHz\n", FreqString( (Second - First))); 
#endif

    return Status;
}
