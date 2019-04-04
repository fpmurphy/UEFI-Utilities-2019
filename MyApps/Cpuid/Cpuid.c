//
//  Copyright (c) 2017 - 2019   Finnbarr P. Murphy.   All rights reserved.
//  Portions Copyright (c) 2016, Intel Corporation.   All rights reserved. 
//
//  Show concise CPUID information about a processsor. 
//
//  License: BSD license applies to code copyrighted by Intel Corporation.
//           BSD 2 clause license applies to all other code.
//
//


#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Register/Cpuid.h>

#define UTILITY_VERSION L"20190403"
#define WIDTH 60
#undef DEBUG


//
// Display Processor Signature
//
VOID
ProcessorSignature( VOID )
{
    UINT32 Eax, Ebx, Ecx, Edx;
    CHAR8  Signature[13];

    AsmCpuid( CPUID_SIGNATURE, &Eax, &Ebx, &Ecx, &Edx );

#ifdef DEBUG
    Print(L"  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", Eax, Ebx, Ecx, Edx);
#endif

    *(UINT32 *)(Signature + 0) = Ebx;
    *(UINT32 *)(Signature + 4) = Edx;
    *(UINT32 *)(Signature + 8) = Ecx;
    Signature[12] = 0;

    Print(L"    Signature: %a\n", Signature);
}


//
// Display Processor Brand String
//
VOID
ProcessorBrandString( VOID )
{
    CPUID_BRAND_STRING_DATA Eax, Ebx, Ecx, Edx;
    UINT32                  BrandString[13];

    AsmCpuid( CPUID_BRAND_STRING1, &Eax.Uint32, &Ebx.Uint32, &Ecx.Uint32, &Edx.Uint32 );
#ifdef DEBUG
    Print(L"  String1:  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", Eax.Uint32, Ebx.Uint32, Ecx.Uint32, Edx.Uint32);
#endif
    BrandString[0] = Eax.Uint32;
    BrandString[1] = Ebx.Uint32;
    BrandString[2] = Ecx.Uint32;
    BrandString[3] = Edx.Uint32;

    AsmCpuid( CPUID_BRAND_STRING2, &Eax.Uint32, &Ebx.Uint32, &Ecx.Uint32, &Edx.Uint32 );
#ifdef DEBUG
    Print(L"  String2:  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", Eax.Uint32, Ebx.Uint32, Ecx.Uint32, Edx.Uint32);
#endif
    BrandString[4] = Eax.Uint32;
    BrandString[5] = Ebx.Uint32;
    BrandString[6] = Ecx.Uint32;
    BrandString[7] = Edx.Uint32;

    AsmCpuid( CPUID_BRAND_STRING3, &Eax.Uint32, &Ebx.Uint32, &Ecx.Uint32, &Edx.Uint32 );
#ifdef DEBUG
    Print(L"  String3:  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", Eax.Uint32, Ebx.Uint32, Ecx.Uint32, Edx.Uint32);
#endif
    BrandString[8]  = Eax.Uint32;
    BrandString[9]  = Ebx.Uint32;
    BrandString[10] = Ecx.Uint32;
    BrandString[11] = Edx.Uint32;

    BrandString[12] = 0;

    Print (L"   CPU String: %a\n", (CHAR8 *)BrandString);
}


//
// Display Processor Version Information
//
VOID 
ProcessorVersionInfo( VOID )
{
    CPUID_VERSION_INFO_EAX Eax;
    CPUID_VERSION_INFO_EBX Ebx;
    CPUID_VERSION_INFO_ECX Ecx;
    CPUID_VERSION_INFO_EDX Edx;
    UINT32                 DisplayFamily;
    UINT32                 DisplayModel;

    AsmCpuid( CPUID_VERSION_INFO, &Eax.Uint32, &Ebx.Uint32, &Ecx.Uint32, &Edx.Uint32 );
#ifdef DEBUG
    Print(L"  VersionInfo:  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", Eax.Uint32, Ebx.Uint32, Ecx.Uint32, Edx.Uint32);
#endif

    DisplayFamily = Eax.Bits.FamilyId;
    if (Eax.Bits.FamilyId == 0x0F) {
        DisplayFamily |= (Eax.Bits.ExtendedFamilyId << 4);
    }

    DisplayModel = Eax.Bits.Model;
    if (Eax.Bits.FamilyId == 0x06 || Eax.Bits.FamilyId == 0x0f) {
        DisplayModel |= (Eax.Bits.ExtendedModelId << 4);
    }

    Print(L"       Family: 0x%x\n", DisplayFamily);
    Print(L"        Model: 0x%x\n", DisplayModel);
    Print(L"     Stepping: 0x%x\n", Eax.Bits.SteppingId);
}


//
// Display Available Processor Features
//
VOID 
ProcessorFeatures( VOID )
{
    CPUID_EXTENDED_CPU_SIG_ECX xEcx;
    CPUID_EXTENDED_CPU_SIG_EDX xEdx;
    CPUID_VERSION_INFO_EAX     Eax;
    CPUID_VERSION_INFO_EBX     Ebx;
    CPUID_VERSION_INFO_ECX     Ecx;
    CPUID_VERSION_INFO_EDX     Edx;
    BOOLEAN                    FirstRow = TRUE;
    UINT32                     xEax;
    UINT32                     Col = 0;
    CHAR16                     Features[1000];
    CHAR16                     Buf[80];
    UINT8                      Width = WIDTH;
    CHAR16                     *f = Features;

    ZeroMem( Features, 1000 );

    AsmCpuid( CPUID_VERSION_INFO, &Eax.Uint32, &Ebx.Uint32, &Ecx.Uint32, &Edx.Uint32 );
#ifdef DEBUG
    Print(L"  Features:  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", Eax.Uint32, Ebx.Uint32, Ecx.Uint32, Edx.Uint32);
#endif

    AsmCpuid( CPUID_EXTENDED_CPU_SIG, &xEax, NULL, &xEcx.Uint32, &xEdx.Uint32 );
#ifdef DEBUG
    Print (L"  Extended Features:  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", xEax, 0, xEcx.Uint32, xEdx.Uint32);
#endif

    // Presorted list. No sorting routine!
    if (Edx.Bits.ACPI) StrCat(Features, L" ACPI");             // ACPI via MSR Support
    if (Ecx.Bits.AESNI) StrCat(Features, L" AESNT");
    if (Edx.Bits.APIC) StrCat(Features, L" APIC");
    if (Ecx.Bits.AVX) StrCat(Features, L" AVX");               // Advanced Vector Extensions
    if (Edx.Bits.CLFSH) StrCat(Features, L" CLFSH");           // CLFLUSH (Cache Line Flush) Instruction Support
    if (Edx.Bits.CMOV) StrCat(Features, L" CMOV");             // CMOV Instructions Extension 
    if (Ecx.Bits.CMPXCHG16B) StrCat(Features, L" CMPXCHG16B"); // CMPXCHG16B Instruction Support
    if (Ecx.Bits.CNXT_ID) StrCat(Features, L" CNXT_ID");       // L1 Cache Adaptive Or Shared Mode Support
    if (Edx.Bits.CX8) StrCat(Features, L" CX8");               // CMPXCHG8 Instruction Support 
    if (Ecx.Bits.DCA) StrCat(Features, L" DCA");               // Direct Cache Access
    if (Edx.Bits.DE) StrCat(Features, L" DE");                 // Debugging Extensions
    if (Edx.Bits.DS) StrCat(Features, L" DS");                 // Dubug Store Support
    if (Ecx.Bits.DS_CPL) StrCat(Features, L" DS_CPL");         // CPL Qual. Debug Store Support
    if (Ecx.Bits.DTES64) StrCat(Features, L" DTES64");         // 64-bit Debug Store Support
    if (Ecx.Bits.F16C) StrCat(Features, L" F16C");             // 16-bit FP Conversion Instructions Support
    if (Ecx.Bits.FMA) StrCat(Features, L" FMA");               // Fused Multiply-Add
    if (Edx.Bits.FPU) StrCat(Features, L" FPU");               // Floating Point Unit 
    if (Edx.Bits.FXSR) StrCat(Features, L" FXSR");             // FXSAVE/FXRSTOR Support
    if (Edx.Bits.HTT) StrCat(Features, L" HTT");               // Max APIC IDs reserved field is Valid
    if (xEcx.Bits.LAHF_SAHF) StrCat(Features, L" LAHF_SAHF"); 
    if (xEdx.Bits.LM) StrCat(Features, L" LM"); 
    if (xEcx.Bits.LZCNT) StrCat(Features, L" LZCNT"); 
    if (Edx.Bits.MCA) StrCat(Features, L" MCA");               // Machine Check Architecture
    if (Edx.Bits.MCE) StrCat(Features, L" MCE");               // Machine Check Exception
    if (Ecx.Bits.MONITOR) StrCat(Features, L" MONITOR");       // Monitor/Mwait Support (SSE3 supplements)
    if (Edx.Bits.MMX) StrCat(Features, L" MMX");               // Multimedia Extensions
    if (Ecx.Bits.MOVBE) StrCat(Features, L" MOVBE");           // Move Data After Swapping Bytes Instruction Support
    if (Edx.Bits.MSR) StrCat(Features, L" MSR");               // Model Specific Registers
    if (Edx.Bits.MTRR) StrCat(Features, L" MTRR");             // Memory Type Range Registers
    if (xEdx.Bits.NX) StrCat(Features, L" NX"); 
    if (Ecx.Bits.OSXSAVE) StrCat(Features, L" OSXSAVE");       // OS has set bit to support CPU extended state management using XSAVE/XRSTOR
    if (Edx.Bits.PAE) StrCat(Features, L" PAE");               // Physical Address Extensions
    if (xEdx.Bits.Page1GB) StrCat(Features, L" PAGE1GB"); 
    if (Edx.Bits.PAT) StrCat(Features, L" PAT");               // Page Attribute Table
    if (Edx.Bits.PBE) StrCat(Features, L" PBE");               // Pending Break Enable
    if (Ecx.Bits.PCID) StrCat(Features, L" PCID");             // Process Context Identifiers
    if (Ecx.Bits.PCLMULQDQ) StrCat(Features, L" PCLMULQDQ");   // Support Carry-Less Multiplication of Quadword instruction 
    if (Ecx.Bits.PDCM) StrCat(Features, L" PDCM");             // Performance Capabilities
    if (Edx.Bits.PGE) StrCat(Features, L" PGE");               // Page Global Enable
    if (Ecx.Bits.POPCNT) StrCat(Features, L" POPCNT");         // Return the Count of Number of Bits Set to 1 instruction
    if (xEcx.Bits.PREFETCHW) StrCat(Features, L" PREFETCHW"); 
    if (Edx.Bits.PSE) StrCat(Features, L" PSE");               // Page Size Extensions (4MB memory pages)
    if (Edx.Bits.PSE_36) StrCat(Features, L" PSE_36");         // 36-Bit (> 4MB) Page Size Extension 
    if (Edx.Bits.PSN) StrCat(Features, L" PSN");               // Processor Serial Number Support
    if (Ecx.Bits.RDRAND) StrCat(Features, L" RDRAND");         // Read Random Number from hardware random number generator instruction Support
    if (xEdx.Bits.RDTSCP) StrCat(Features, L" RDTSCP"); 
    if (Ecx.Bits.SDBG) StrCat(Features, L" SDBG");             // Silicon Debug Support
    if (Edx.Bits.SEP) StrCat(Features, L" SEP");               // SYSENTER/SYSEXIT Support
    if (Ecx.Bits.SMX) StrCat(Features, L" SMX");
    if (Edx.Bits.SS) StrCat(Features, L" SS");
    if (Edx.Bits.SSE) StrCat(Features, L" SSE");
    if (Edx.Bits.SSE2) StrCat(Features, L" SSE2");
    if (Ecx.Bits.SSE3) StrCat(Features, L" SSE3");
    if (Ecx.Bits.SSE4_1) StrCat(Features, L" SSE4_1");
    if (Ecx.Bits.SSE4_2) StrCat(Features, L" SSE4_2");
    if (Ecx.Bits.SSSE3) StrCat(Features, L" SSSE3");                             // Supplemental SSE-3
    if (xEdx.Bits.SYSCALL_SYSRET) StrCat(Features, L" SYSCALL_SYSRET"); 
    if (Edx.Bits.TSC) StrCat(Features, L" TSC");                                 // Time Stamp Counter
    if (Ecx.Bits.TSC_Deadline) StrCat(Features, L" TSC_DEADLINE");               // TSC Deadline Timer
    if (Edx.Bits.TM) StrCat(Features, L" TM");                                   // Automatic Clock Control (Thermal Monitor)
    if (Ecx.Bits.TM2) StrCat(Features, L" TM2");                                 // Thermal Monitor 2
    if (Edx.Bits.VME) StrCat(Features, L" VME");                                 // Virtual 8086 Mode Enhancements
    if (Ecx.Bits.VMX) StrCat(Features, L" VMX");                                 // Hardware Virtualization Support
    if (Ecx.Bits.x2APIC) StrCat(Features, L" X2APIC");                           // x2APIC Support
    if (Ecx.Bits.XSAVE) StrCat(Features, L" XSAVE");                             // Save Processor Extended States
    if (Ecx.Bits.xTPR_Update_Control) StrCat(Features, L" XTPR_UPDATE_CONTROL"); // Change IA32_MISC_ENABLE Support

    Print(L"     Features:");

    // Not the most elegant output folding code but it works!
    ZeroMem( Buf, 80 );
    while (*f) {
         Buf[Col++] = *f++;
         if (Col > Width) {
             while (Col >= 0 && Buf[Col] != L' ') {
                 Buf[Col] = CHAR_NULL;  
                 f--; Col--;
             } 
             if (FirstRow) {
                 Print(L"%s\n", Buf);
                 FirstRow = FALSE;
             } else {
                 Print(L"              %s\n", Buf);
             }
             ZeroMem(Buf,80);
             Col = 0;
         } 
    }
    if (Col) {
        if (FirstRow) {
            Print(L"%s\n", Buf);
        } else {
            Print(L"              %s\n", Buf);
        }
    }
}


VOID
Usage( BOOLEAN ErrorMsg )
{
    if ( ErrorMsg ) {
        Print(L"ERROR: Unknown option(s).\n");
    }

    Print(L"Usage: Cpuid [ -V | --version ]\n");
}


INTN
EFIAPI
ShellAppMain( UINTN Argc,
              CHAR16 **Argv )
{
    EFI_STATUS Status = EFI_SUCCESS;

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

    Print(L"\n");
    ProcessorSignature();
    ProcessorBrandString();
    ProcessorVersionInfo();
    ProcessorFeatures();
    Print(L"\n");

    return Status;
}
