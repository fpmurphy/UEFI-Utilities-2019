//
//  Copyright (c) 2015-2019  Finnbarr P. Murphy.   All rights reserved.
//
//  Show BGRT info, display image or save image to file 
//
//  License: EDKII license applies to code from EDKII source,
//           BSD 2 clause license applies to all other code.
//
//  Portions Copyright (c) 2016-2017, Microsoft Corporation.
//           Copyright (c) 2018, Intel Corporation.
//           See relevant code in EDK11 for exact details
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
#include <Protocol/AcpiSystemDescriptionTable.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/Shell.h>

#include <Guid/Acpi.h>

#include <IndustryStandard/Bmp.h>
#include <IndustryStandard/Acpi61.h>

#define UTILITY_VERSION L"20190611"
#undef DEBUG

// for option setting
typedef enum {
   Verbose = 1,
   HexDump,
   SaveImageMode,
   DisplayImageMode
} MODE;


VOID AsciiToUnicodeSizeQuote(CHAR8 *, UINT8, CHAR16 *, BOOLEAN);


EFI_GRAPHICS_OUTPUT_BLT_PIXEL EfiGraphicsColors[16] = {
    // B    G    R   reserved
    {0x00, 0x00, 0x00, 0x00},  // BLACK
    {0x98, 0x00, 0x00, 0x00},  // LIGHTBLUE
    {0x00, 0x98, 0x00, 0x00},  // LIGHGREEN
    {0x98, 0x98, 0x00, 0x00},  // LIGHCYAN
    {0x00, 0x00, 0x98, 0x00},  // LIGHRED
    {0x98, 0x00, 0x98, 0x00},  // MAGENTA
    {0x00, 0x98, 0x98, 0x00},  // BROWN
    {0x98, 0x98, 0x98, 0x00},  // LIGHTGRAY
    {0x30, 0x30, 0x30, 0x00},  // DARKGRAY
    {0xff, 0x00, 0x00, 0x00},  // BLUE
    {0x00, 0xff, 0x00, 0x00},  // LIME
    {0xff, 0xff, 0x00, 0x00},  // CYAN
    {0x00, 0x00, 0xff, 0x00},  // RED
    {0xff, 0x00, 0xff, 0x00},  // FUCHSIA
    {0x00, 0xff, 0xff, 0x00},  // YELLOW
    {0xff, 0xff, 0xff, 0x00}   // WHITE
};


VOID
GetBackgroundColor( EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Background )
{
    INTN Attribute;

    Attribute = gST->ConOut->Mode->Attribute & 0x7F;
    *Background = EfiGraphicsColors[Attribute >> 4];
    // DEBUG  *Background = EfiGraphicsColors[10];
}


VOID
GetCursorPosition( UINTN *x,
                   UINTN *y )
{
    *x = gST->ConOut->Mode->CursorColumn;
    *y = gST->ConOut->Mode->CursorRow;
}


VOID
SetCursorPosition( UINTN x,
                   UINTN y )
{
    gST->ConOut->SetCursorPosition( gST->ConOut, x, y);
}


VOID
AsciiToUnicodeSizeQuote( CHAR8 *String,
                         UINT8 length,
                         CHAR16 *UniString,
                         BOOLEAN Quote )
{
    int len = length;

    if (Quote)
        *(UniString++) = L'"';
    while (*String != '\0' && len > 0) {
        *(UniString++) = (CHAR16) *(String++);
        len--;
    }
    if (Quote)
        *(UniString++) = L'"';
    *UniString = '\0';
}


VOID
DumpHex( UINT8 *ptr,
         int   Count )
{
    int i = 0;

    Print(L"  ");
    for ( i = 0; i < Count; i++ ) {
        if ( i > 0 && i%16 == 0 )
            Print(L"\n  ");
        Print(L"0x%02x ", 0xff & *ptr++);
    }
    Print(L"\n");
}


//
// Display the BMP image, convert to 24-bit if necessary, scroll screen if necessary
//
EFI_STATUS
DisplayImage( EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop, 
              EFI_HANDLE *BmpBuffer )
{
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL Background;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL *BltBuffer;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Blt;
    BMP_IMAGE_HEADER *BmpHeader;
    BMP_COLOR_MAP *BmpColorMap;
    EFI_STATUS Status = EFI_SUCCESS;
    UINT32 *Palette;
    UINT8  *BitmapData;
    UINT8  *Image;
    UINT8  *ImageHeader;
    UINTN  SizeOfInfo;
    UINTN  Pixels;
    UINTN  Width, Height;
    UINTN  ImageIndex;
    UINTN  Index;
    UINTN  ImageHeight;
    UINTN  ImageRows;
    UINTN  CurRow, CurCol;
    UINTN  MaxRows, MaxCols;
    UINTN  VertPixelDelta = 0;		
    UINTN  ImagePixelDelta = 0;		

    if (BmpBuffer == NULL) {
        return RETURN_INVALID_PARAMETER;
    }

    BmpHeader  = (BMP_IMAGE_HEADER *) BmpBuffer;
    BitmapData = (UINT8*)BmpBuffer + BmpHeader->ImageOffset;
    Palette    = (UINT32*) ((UINT8*)BmpBuffer + 0x36);
    Pixels     = BmpHeader->PixelWidth * BmpHeader->PixelHeight;

    BltBuffer = AllocateZeroPool( sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL) * Pixels);
    if (BltBuffer == NULL) {
        Print(L"ERROR: BltBuffer. No memory resources\n");
        return EFI_OUT_OF_RESOURCES;
    }

    Image       = (UINT8 *)BmpBuffer;
    BmpColorMap = (BMP_COLOR_MAP *) (Image + sizeof (BMP_IMAGE_HEADER));

    Image       = ((UINT8 *)BmpBuffer) + BmpHeader->ImageOffset;
    ImageHeader = Image;

    // fill blt buffer
    for (Height = 0; Height < BmpHeader->PixelHeight; Height++) {
        Blt = &BltBuffer[(BmpHeader->PixelHeight - Height - 1) * BmpHeader->PixelWidth];
        for (Width = 0; Width < BmpHeader->PixelWidth; Width++, Image++, Blt++) {
            switch (BmpHeader->BitPerPixel) {
                case 1:                                 // Convert 1-bit BMP to 24-bit color
                    for (Index = 0; Index < 8 && Width < BmpHeader->PixelWidth; Index++) {
                        Blt->Blue  = BmpColorMap[((*Image) >> (7 - Index)) & 0x1].Blue;
                        Blt->Green = BmpColorMap[((*Image) >> (7 - Index)) & 0x1].Green;
                        Blt->Red   = BmpColorMap[((*Image) >> (7 - Index)) & 0x1].Red;
                        Blt++; Width++;
                    }
                    Blt--; Width--;
                    break;

                case 4:                                 // Convert 4-bit BMP Palette to 24-bit color
                    Index      = (*Image) >> 4;
                    Blt->Blue  = BmpColorMap[Index].Blue;
                    Blt->Green = BmpColorMap[Index].Green;
                    Blt->Red   = BmpColorMap[Index].Red;
                    if (Width < (BmpHeader->PixelWidth - 1)) {
                        Blt++; Width++;
                        Index      = (*Image) & 0x0f;
                        Blt->Blue  = BmpColorMap[Index].Blue;
                        Blt->Green = BmpColorMap[Index].Green;
                        Blt->Red   = BmpColorMap[Index].Red;
                    }
                    break;

                case 8:                                 // Convert 8-bit BMP palette to 24-bit color
                    Blt->Blue  = BmpColorMap[*Image].Blue;
                    Blt->Green = BmpColorMap[*Image].Green;
                    Blt->Red   = BmpColorMap[*Image].Red;
                    break;

                case 24:                                // No conversion needed
                    Blt->Blue  = *Image++;
                    Blt->Green = *Image++;
                    Blt->Red   = *Image;
                    break;

                case 32:                                // Convert to 24-bit by ignoring final byte of each pixel.
                    Blt->Blue  = *Image++;
                    Blt->Green = *Image++;
                    Blt->Red   = *Image++;
                    break;

                default:
                    FreePool(BltBuffer);
                    return EFI_UNSUPPORTED;
                    break;
            };
        }

        // start each row on a 32-bit boundary!
        ImageIndex = (UINTN)Image - (UINTN)ImageHeader;
        if ((ImageIndex % 4) != 0) {
             Image = Image + (4 - (ImageIndex % 4));
        }
    }

    // get max rows and columns for current mode
    gST->ConOut->QueryMode( gST->ConOut,
                            gST->ConOut->Mode->Mode,
                            &MaxCols,
                            &MaxRows );

    GetCursorPosition( &CurCol, &CurRow );
#ifdef DEBUG
    Print(L"CURSOR %02d %02d\n", CurCol, CurRow);
#endif

    // get screen details for current mode
    Gop->QueryMode( Gop, 
                    Gop->Mode->Mode, 
                    &SizeOfInfo, 
                    &Info );

    // calculate required image and screen properties
    Width  = Info->HorizontalResolution;
    ImageHeight = BmpHeader->PixelHeight;
    ImageRows = ImageHeight/EFI_GLYPH_HEIGHT;
    if ((ImageRows * EFI_GLYPH_HEIGHT) < ImageHeight) {
        ImagePixelDelta = (ImageHeight - (ImageRows * EFI_GLYPH_HEIGHT))/2;
        ImageRows++;        
    }
    if ((MaxRows * EFI_GLYPH_HEIGHT) < Info->VerticalResolution) {
        VertPixelDelta = (Info->VerticalResolution - (MaxRows * EFI_GLYPH_HEIGHT))/2;
    }

    // scroll required?
    if ((CurRow + ImageRows + 1) >= MaxRows) {
        GetBackgroundColor( &Background );

        // calculate number of rows to scroll
        UINTN ScrollRows = (ImageRows - (MaxRows - CurRow) +1); 
#ifdef DEBUG
        Print(L"CurRow: %d  ImageRows: %d  ImagePixelDelta: %d  ScrollRows: %d  VertPixelDelta: %d\n", 
                CurRow, ImageRows, ImagePixelDelta, ScrollRows, VertPixelDelta );
#endif

        // scroll up to make room to display image 
        Status = Gop->Blt( Gop,
                           NULL,
                           EfiBltVideoToVideo,
                           0, VertPixelDelta + (ScrollRows * EFI_GLYPH_HEIGHT),  // Source X,Y        
                           0, VertPixelDelta,                                    // Destination X,Y
                           Width, (MaxRows - ScrollRows) * EFI_GLYPH_HEIGHT,
                           0 );
        if (EFI_ERROR (Status)) {
            Print(L"ERROR: Scroll Up, Gop->Blt [%d]\n", Status);
            goto cleanup;
        } 

        // color background of the scrolled area
        Status = Gop->Blt( Gop,
                           &Background,
                           EfiBltVideoFill,
                           0, 0,                                // Not Used
                           0, (MaxRows - ScrollRows) * EFI_GLYPH_HEIGHT, 
                           Width - 1, ScrollRows * EFI_GLYPH_HEIGHT,
                           0 );
        if (EFI_ERROR (Status)) {
            Print(L"ERROR: Color Fill, Gop->Blt [%d]\n", Status);
            goto cleanup;
        } 

        // display the image
        Status = Gop->Blt( Gop,
                           BltBuffer,
                           EfiBltBufferToVideo,
                           0, 0,                                                              // Source X,Y 
                           0, ImagePixelDelta + ((MaxRows - ImageRows) * EFI_GLYPH_HEIGHT),   // Destination X,Y
                           BmpHeader->PixelWidth, BmpHeader->PixelHeight, 
                           0 );
        if (EFI_ERROR (Status)) {
            Print(L"ERROR: Image Display Gop->Blt [%d]\n", Status);
            goto cleanup;
        } 

        SetCursorPosition(  0, MaxRows - 1 );
    } else {
        // just display the image
        Status = Gop->Blt( Gop,
                           BltBuffer,
                           EfiBltBufferToVideo,
                           0, 0,                                                        // Source X,Y 
                           0, ImagePixelDelta + ((CurRow + 1) * EFI_GLYPH_HEIGHT),      // Destination X,Y 
                           BmpHeader->PixelWidth, BmpHeader->PixelHeight, 
                           0 );
        if (EFI_ERROR (Status)) {
            Print(L"ERROR: Image Display Gop->Blt [%d]\n", Status);
            goto cleanup;
        } 
        SetCursorPosition(  0, CurRow + ImageRows );
    }

cleanup:
    FreePool(BltBuffer);

    return Status;
}



//
// Save image as a BMP file
//
EFI_STATUS 
SaveImage( CHAR16 *FileName,
           UINT8  *FileData,
           UINTN  FileDataLength )
{
    SHELL_FILE_HANDLE FileHandle = NULL;
    EFI_STATUS        Status = EFI_SUCCESS;
    CONST CHAR16      *CurDir = NULL;
    CONST CHAR16      *PathName = NULL;
    CHAR16            *FullPath = NULL;
    UINTN             Length = 0;
    
    CurDir = gEfiShellProtocol->GetCurDir(NULL);
    if (CurDir == NULL) {
        Print(L"ERROR: Cannot retrieve current directory\n");
	return EFI_NOT_FOUND;
    }
    PathName = CurDir;
    StrnCatGrow(&FullPath, &Length, PathName, 0);
    StrnCatGrow(&FullPath, &Length, L"\\", 0);
    StrnCatGrow(&FullPath, &Length, FileName, 0);
#ifdef DEBUG
    Print(L"FullPath: [%s]\n", FullPath);
#endif

    Status = gEfiShellProtocol->OpenFileByName( FullPath,
                                                &FileHandle,
                                                EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE);
    if (EFI_ERROR(Status)) {
        Print(L"ERROR: OpenFileByName [%s] [%d]\n", FullPath, Status);
        return Status;
    } 

    // BufferSize = FileDataLength;
    Status = gEfiShellProtocol->WriteFile( FileHandle, 
                                           &FileDataLength, 
                                           FileData );
    gEfiShellProtocol->CloseFile( FileHandle );

    if (EFI_ERROR(Status)) {
        Print(L"ERROR: Saving image to file: %x\n", Status);
    } else {
        Print(L"Successfully saved image to %s\n", FullPath);
    }

    return Status;
}


VOID
PrintAcpiHeader( EFI_ACPI_DESCRIPTION_HEADER *Ptr )
{
    CHAR16 Buffer[50];

    Print(L"  ACPI Header:\n");
    AsciiToUnicodeSizeQuote((CHAR8 *)&(Ptr->Signature), 4, Buffer, TRUE);
    Print(L"    Signature         : %s\n", Buffer);
    Print(L"    Length            : 0x%08x (%d)\n", Ptr->Length, Ptr->Length);
    Print(L"    Revision          : 0x%02x (%d)\n", Ptr->Revision, Ptr->Revision);
    Print(L"    Checksum          : 0x%02x (%d)\n", Ptr->Checksum, Ptr->Checksum);
    AsciiToUnicodeSizeQuote((CHAR8 *)&(Ptr->OemId), 6, Buffer, TRUE);
    Print(L"    OEM ID            : %s\n", Buffer);
    AsciiToUnicodeSizeQuote((CHAR8 *)&(Ptr->OemTableId), 8, Buffer, TRUE);
    Print(L"    OEM Table ID      : %s\n", Buffer);
    Print(L"    OEM Revision      : 0x%08x (%d)\n", Ptr->OemRevision, Ptr->OemRevision);
    AsciiToUnicodeSizeQuote((CHAR8 *)&(Ptr->CreatorId), 4, Buffer, TRUE);
    Print(L"    Creator ID        : %s\n", Buffer);
    Print(L"    Creator Revision  : 0x%08x (%d)\n", Ptr->CreatorRevision, Ptr->CreatorRevision);
    Print(L"\n");
}


//
// Print the BMP header details
//
VOID
PrintBMPHeader( EFI_HANDLE *BmpBuffer )
{
    BMP_IMAGE_HEADER *BmpHeader;
    CHAR16 Buffer[100];

    BmpHeader = (BMP_IMAGE_HEADER *) BmpBuffer;

    AsciiToUnicodeSizeQuote( (CHAR8 *)BmpHeader, 2, Buffer, FALSE );

    Print(L"\n");
    Print(L"  BMP Header:\n");
    Print(L"    BMP Signature      : %s\n", Buffer);
    Print(L"    Size               : %d\n", BmpHeader->Size);
    Print(L"    Image Offset       : %d\n", BmpHeader->ImageOffset);
    Print(L"    Header Size        : %d\n", BmpHeader->HeaderSize);
    Print(L"    Image Width        : %d\n", BmpHeader->PixelWidth);
    Print(L"    Image Height       : %d\n", BmpHeader->PixelHeight);
    Print(L"    Planes             : %d\n", BmpHeader->Planes);
    Print(L"    Bits Per Pixel     : %d\n", BmpHeader->BitPerPixel);
    Print(L"    Compression Type   : %d\n", BmpHeader->CompressionType);
    Print(L"    Image Size         : %d\n", BmpHeader->ImageSize);
    Print(L"    X Pixels Per Meter : %d\n", BmpHeader->XPixelsPerMeter);
    Print(L"    Y Pixels Per Meter : %d\n", BmpHeader->YPixelsPerMeter);
    Print(L"    Number of Colors   : %d\n", BmpHeader->NumberOfColors);
    Print(L"    Important Colors   : %d\n", BmpHeader->ImportantColors);
    Print(L"\n");
}


//
// Parse the in-memory BMP header
//
EFI_STATUS
ParseImage( UINT64 BmpImage,
            MODE   Mode )
{
    BMP_IMAGE_HEADER *BmpHeader = (BMP_IMAGE_HEADER *)BmpImage;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop = NULL;
    EFI_DEVICE_PATH_PROTOCOL     *Dpp;
    EFI_STATUS                   Status = EFI_SUCCESS;
    EFI_HANDLE                   *Handles = NULL;
    UINTN                        HandleCount = 0;

    // not BMP format
    if (BmpHeader->CharB != 'B' || BmpHeader->CharM != 'M') {
        Print(L"ERROR: Unsupported image format\n"); 
        return EFI_UNSUPPORTED;
    }

    // BITMAPINFOHEADER format unsupported
    if (BmpHeader->HeaderSize != sizeof (BMP_IMAGE_HEADER) \
        - ((UINTN) &(((BMP_IMAGE_HEADER *)0)->HeaderSize))) {
        Print(L"ERROR: Unsupported BITMAPFILEHEADER\n");
        return EFI_UNSUPPORTED;
    }

    // supported bits per pixel
    if (BmpHeader->BitPerPixel != 1 &&
        BmpHeader->BitPerPixel != 8 &&
        BmpHeader->BitPerPixel != 8 &&
        BmpHeader->BitPerPixel != 12 &&
        BmpHeader->BitPerPixel != 24 &&
        BmpHeader->BitPerPixel != 32) {
        Print(L"ERROR: BitPerPixel is not one of 1, 4, 8, 12, 24 or 32\n");
        return EFI_UNSUPPORTED;
    }

    if (Mode == SaveImageMode) {
        Status = SaveImage( L"BGRTimage.bmp", 
                            (UINT8 *)BmpImage, 
                            BmpHeader->Size );
    } else if (Mode == DisplayImageMode) {
        // Try locating GOP by handle
        Status = gBS->LocateHandleBuffer( ByProtocol,
                                          &gEfiGraphicsOutputProtocolGuid,
                                          NULL,
                                          &HandleCount,
                                          &Handles );
        if (EFI_ERROR (Status)) {
            Print(L"ERROR: No GOP handles found via LocateHandleBuffer\n");
            return Status; 
        } 

#ifdef DEBUG
        Print(L"Found %d GOP handles via LocateHandleBuffer\n", HandleCount);
#endif

        // Make sure we use the correct GOP handle
        for (UINTN Handle = 0; Handle < HandleCount; Handle++) {
            Status = gBS->HandleProtocol( Handles[Handle], 
                                          &gEfiDevicePathProtocolGuid, 
                                          (VOID **)&Dpp );
            if (!EFI_ERROR(Status)) {
                Status = gBS->HandleProtocol( Handles[Handle],
                                              &gEfiGraphicsOutputProtocolGuid, 
                                              (VOID **)&Gop );
                if (!EFI_ERROR(Status)) {
                   break;
                }
            }
        }
        FreePool(Handles);
        if (Gop == NULL) {
            Print(L"ERROR: No graphics console found.\n");
            return Status;
        }
        Status = DisplayImage( Gop, (EFI_HANDLE *)BmpImage );
    } else if (Mode == Verbose) {
        PrintBMPHeader( (EFI_HANDLE *)BmpImage );
    } 
    
    return Status;
}


//
// Parse Boot Graphic Resource Table (BGRT)
//
VOID
ParseBGRT( EFI_ACPI_6_1_BOOT_GRAPHICS_RESOURCE_TABLE *Bgrt, 
           MODE Mode )
{
    Print(L"\n");

    if ( Mode == HexDump ) {
        DumpHex( (UINT8 *)Bgrt, (int)sizeof(EFI_ACPI_6_1_BOOT_GRAPHICS_RESOURCE_TABLE) );
    } else {
        if ( Mode == Verbose) {
            PrintAcpiHeader( (EFI_ACPI_DESCRIPTION_HEADER *)&(Bgrt->Header) );
        }
        if ( Mode != SaveImageMode && Mode != DisplayImageMode ) {
            Print(L"  BGRT Header:\n");
            Print(L"    Version           : %d\n", Bgrt->Version);
            Print(L"    Status            : %d", Bgrt->Status);
            if (Bgrt->Status == EFI_ACPI_5_0_BGRT_STATUS_NOT_DISPLAYED) {
                Print(L" (Not displayed)");
            }
            if (Bgrt->Status == EFI_ACPI_5_0_BGRT_STATUS_DISPLAYED) {
                Print(L" (Displayed)");
            }
            Print(L"\n"); 
            Print(L"    Image Type        : %d", Bgrt->ImageType); 
            if (Bgrt->ImageType == EFI_ACPI_5_0_BGRT_IMAGE_TYPE_BMP) {
                Print(L" (BMP format)");
            }
            Print(L"\n"); 
            Print(L"    Offset Y          : %ld\n", Bgrt->ImageOffsetY);
            Print(L"    Offset X          : %ld\n", Bgrt->ImageOffsetX);
        }
        if (Mode == Verbose) {
            Print(L"    Physical Address  : 0x%08x\n", Bgrt->ImageAddress);
        }
        ParseImage( Bgrt->ImageAddress, Mode );
    }
 
    Print(L"\n");
}


int
ParseRSDP( EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER *Rsdp, 
           CHAR16* GuidStr,
           MODE Mode )
{
    EFI_ACPI_DESCRIPTION_HEADER *Xsdt, *Entry;
    UINT32 EntryCount;
    UINT64 *EntryPtr;

#ifdef DEBUG
    Print(L"\n\nACPI GUID: %s\n", GuidStr);
#endif

    if (Rsdp->Revision >= EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER_REVISION) {
        Xsdt = (EFI_ACPI_DESCRIPTION_HEADER *)(Rsdp->XsdtAddress);
    } else {
#ifdef DEBUG
        Print(L"ERROR: RSDP table < revision ACPI 2.0 found.\n");
#endif
        return 1;
    }

    if (Xsdt->Signature != SIGNATURE_32 ('X', 'S', 'D', 'T')) {
        return 1;
    }

    EntryCount = (Xsdt->Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)) / sizeof(UINT64);

    EntryPtr = (UINT64 *)(Xsdt + 1);
    for (int Index = 0; Index < EntryCount; Index++, EntryPtr++) {
        Entry = (EFI_ACPI_DESCRIPTION_HEADER *)((UINTN)(*EntryPtr));
        if (Entry->Signature == SIGNATURE_32 ('B', 'G', 'R', 'T')) {
            ParseBGRT((EFI_ACPI_6_1_BOOT_GRAPHICS_RESOURCE_TABLE *)((UINTN)(*EntryPtr)), Mode);
        }
    }

    return 0;
}


VOID
Usage( BOOLEAN ErrorMsg )
{
    if ( ErrorMsg ) {
        Print(L"ERROR: Unknown option.\n");
    }
    Print(L"Usage: ShowBGRT [-v | --verbose]\n");
    Print(L"       ShowBGRT [-s | --save]\n");
    Print(L"       ShowBGRT [-D | --dump]\n");
    Print(L"       ShowBGRT [-d | --display]\n");
    Print(L"       ShowBGRT [-V | --version]\n");
}


INTN
EFIAPI
ShellAppMain( UINTN Argc, 
              CHAR16 **Argv )
{
    EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER *Rsdp = NULL;
    EFI_CONFIGURATION_TABLE *ect = gST->ConfigurationTable;
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_GUID   gAcpi20TableGuid = EFI_ACPI_20_TABLE_GUID;
    EFI_GUID   gAcpi10TableGuid = ACPI_10_TABLE_GUID;
    CHAR16     GuidStr[100];
    MODE       Mode;

    if (Argc == 2) {
        if (!StrCmp(Argv[1], L"--verbose") ||
            !StrCmp(Argv[1], L"-v")) {
            Mode = Verbose;
        } else if (!StrCmp(Argv[1], L"--dump") ||
            !StrCmp(Argv[1], L"-D")) {
            Mode = HexDump;
        } else if (!StrCmp(Argv[1], L"--display") ||
            !StrCmp(Argv[1], L"-d")) {
            Mode = DisplayImageMode;
        } else if (!StrCmp(Argv[1], L"--save") ||
            !StrCmp(Argv[1], L"-s")) {
            Mode = SaveImageMode;
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

    // locate RSDP (Root System Description Pointer) 
    for (int i = 0; i < gST->NumberOfTableEntries; i++) {
        if ((CompareGuid (&(gST->ConfigurationTable[i].VendorGuid), &gAcpi20TableGuid)) ||
            (CompareGuid (&(gST->ConfigurationTable[i].VendorGuid), &gAcpi10TableGuid))) {
            if (!AsciiStrnCmp("RSD PTR ", (CHAR8 *)(ect->VendorTable), 8)) {
                UnicodeSPrint(GuidStr, sizeof(GuidStr), L"%g", &(gST->ConfigurationTable[i].VendorGuid));
                Rsdp = (EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER *)ect->VendorTable;
                ParseRSDP( Rsdp, GuidStr, Mode );
            }
        }
        ect++;
    }

    if (Rsdp == NULL) {
	Print(L"ERROR: Could not find an ACPI RSDP table.\n");
	Status = EFI_NOT_FOUND;
    }

    return Status;
}
