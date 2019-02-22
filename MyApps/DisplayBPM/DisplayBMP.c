//
//  Copyright (c) 2015-2019   Finnbarr P. Murphy.   All rights reserved.
//
//  Display an uncompressed BMP image 
//
//  License: BSD 2 clause License
//
//  Portions Copyright (c) 2016-2017, Microsoft Corporation
//           Copyright (c) 2018, Intel Corporation. All rights reserved.
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
#include <Library/SafeIntLib.h>

#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/GraphicsOutput.h>

#include <IndustryStandard/Bmp.h>

#define UTILITY_VERSION L"20190201"
#undef DEBUG

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
AsciiToUnicodeSize( CHAR8 *String,
                    UINT8 length,
                    CHAR16 *UniString )
{
    int len = length;

    while (*String != '\0' && len > 0) {
        *(UniString++) = (CHAR16) *(String++);
        len--;
    }
    *UniString = '\0';
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
// Print the BMP header details
//
VOID
PrintBMPHeader( EFI_HANDLE *BmpBuffer )
{
    BMP_IMAGE_HEADER *BmpHeader;
    CHAR16 Buffer[100];

    BmpHeader = (BMP_IMAGE_HEADER *) BmpBuffer;

    AsciiToUnicodeSize( (CHAR8 *)BmpHeader, 2, Buffer );

    Print(L"\n");
    Print(L"  BMP Signature      : %s\n", Buffer);
    Print(L"  Size               : %d\n", BmpHeader->Size);
    Print(L"  Image Offset       : %d\n", BmpHeader->ImageOffset);
    Print(L"  Header Size        : %d\n", BmpHeader->HeaderSize);
    Print(L"  Image Width        : %d\n", BmpHeader->PixelWidth);
    Print(L"  Image Height       : %d\n", BmpHeader->PixelHeight);
    Print(L"  Planes             : %d\n", BmpHeader->Planes);
    Print(L"  Bits Per Pixel     : %d\n", BmpHeader->BitPerPixel);
    Print(L"  Compression Type   : %d\n", BmpHeader->CompressionType);
    Print(L"  Image Size         : %d\n", BmpHeader->ImageSize);
    Print(L"  X Pixels Per Meter : %d\n", BmpHeader->XPixelsPerMeter);
    Print(L"  Y Pixels Per Meter : %d\n", BmpHeader->YPixelsPerMeter);
    Print(L"  Number of Colors   : %d\n", BmpHeader->NumberOfColors);
    Print(L"  Important Colors   : %d\n", BmpHeader->ImportantColors);
    Print(L"\n");
}


//
// Check that the image is a valid supported BMP
//
EFI_STATUS
CheckBMPHeader( EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop, 
                EFI_HANDLE *BmpBuffer,
                INTN   BmpImageSize )
{
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
    BMP_IMAGE_HEADER *BmpHeader;
    BMP_COLOR_MAP *BmpColorMap;
    EFI_STATUS Status = EFI_SUCCESS;
    UINT32 BltBufferSize;
    UINT32 ColorMapNum;
    UINT32 DataSize;
    UINT32 DataSizePerLine;
    UINTN  SizeOfInfo;
    UINT8  *Image;

    // check parameters
    if (BmpBuffer == NULL) {
        return EFI_INVALID_PARAMETER;
    } 
    if (BmpImageSize < sizeof (BMP_IMAGE_HEADER)) {
        Print(L"ERROR: BmpImageSize too small\n");
        return EFI_INVALID_PARAMETER;
    }

    BmpHeader = (BMP_IMAGE_HEADER *) BmpBuffer;

    // not BMP format
    if (BmpHeader->CharB != 'B' || BmpHeader->CharM != 'M') {
        Print(L"ERROR: Unsupported image format. Not a BMP\n");
        return EFI_UNSUPPORTED;
    }

    // BITMAPINFOHEADER format unsupported
    if (BmpHeader->HeaderSize != sizeof (BMP_IMAGE_HEADER) \
        - ((UINTN) &(((BMP_IMAGE_HEADER *)0)->HeaderSize))) {
        Print(L"ERROR: Unsupported BITMAPFILEHEADER\n");
        return EFI_UNSUPPORTED;
    }

    // compression type not 0
    if (BmpHeader->CompressionType != 0) {
        Print(L"ERROR: Compression type not 0\n");
        return EFI_UNSUPPORTED;
    }

    if ((BmpHeader->PixelHeight == 0) || (BmpHeader->PixelWidth == 0)) {
        Print(L"ERROR: BMP Header PixelHeight or PixelWidth is 0\n");
        return EFI_UNSUPPORTED;
    }

    // data size in each line must be 4 byte aligned
    Status = SafeUint32Mult( BmpHeader->PixelWidth,
                             BmpHeader->BitPerPixel,
                             &DataSizePerLine );
    if (EFI_ERROR (Status)) {
        Print(L"ERROR: Invalid BMP. PixelWidth or BitPerPixel\n");
        return EFI_UNSUPPORTED;
    }

    Status = SafeUint32Add( DataSizePerLine, 
                            31,
                            &DataSizePerLine );
    if (EFI_ERROR (Status)) {
        Print(L"ERROR: Invalid BMP. DataSizePerLine\n");
        return EFI_UNSUPPORTED;
    }

    DataSizePerLine = (DataSizePerLine >> 3) &(~0x3);
    Status = SafeUint32Mult( DataSizePerLine,
                             BmpHeader->PixelHeight,
                             &BltBufferSize );
    if (EFI_ERROR (Status)) {
        Print(L"ERROR: Invalid BMP. DataSizePerLine or PixelHeight\n");
        return EFI_UNSUPPORTED;
    }

    Status = SafeUint32Mult( BmpHeader->PixelHeight,
                             DataSizePerLine,
                             &DataSize );
    if (EFI_ERROR (Status)) {
        Print(L"ERROR: Invalid BMP. DataSizePerLine or PixelHeight\n");
        return EFI_UNSUPPORTED;
    }

    if ((BmpHeader->Size != BmpImageSize) || 
        (BmpHeader->Size < BmpHeader->ImageOffset) ||
        (BmpHeader->Size - BmpHeader->ImageOffset !=  DataSize)) {
        Print(L"ERROR: Invalid image size\n");
        return EFI_UNSUPPORTED;
    }

    // calculate colormap offset in the image.
    Image       = (UINT8 *)BmpBuffer;
    BmpColorMap = (BMP_COLOR_MAP *) (Image + sizeof (BMP_IMAGE_HEADER));
    if (BmpHeader->ImageOffset < sizeof (BMP_IMAGE_HEADER)) {
        Print(L"ERROR: Invalid colormap offset\n");
        return EFI_UNSUPPORTED;
    }

    if (BmpHeader->ImageOffset > sizeof (BMP_IMAGE_HEADER)) {
        switch (BmpHeader->BitPerPixel) {
            case 1:
                ColorMapNum = 2;
                break;
            case 4:
                ColorMapNum = 16;
                break;
            case 8:
                ColorMapNum = 256;
                break;
            default:
                ColorMapNum = 0;
                break;
        }
        if (BmpHeader->ImageOffset - sizeof (BMP_IMAGE_HEADER) != sizeof (BMP_COLOR_MAP) * ColorMapNum) {
            Print(L"ERROR: Invalid colormap offset\n");
            return EFI_UNSUPPORTED;
        }
    }

    // image size less than screen size
    Gop->QueryMode( Gop, 
                    Gop->Mode->Mode, 
                    &SizeOfInfo, 
                    &Info );

    if ((BmpHeader->PixelWidth > (Info->HorizontalResolution - EFI_GLYPH_WIDTH*5)) || 
        (BmpHeader->PixelHeight > (Info->VerticalResolution - EFI_GLYPH_HEIGHT*5))) {
            Print(L"ERROR: Image too big for screen at current resolution\n");
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

    return Status;
}


VOID
Usage( BOOLEAN ErrorMsg )
{
    if ( ErrorMsg ) {
        Print(L"ERROR: Unknown option(s).\n");
    }

    Print(L"Usage: DisplayBMP [-v | --verbose] BMPfile\n"); 
    Print(L"       DisplayBMP [-V | --version]\n"); 
}


INTN
EFIAPI
ShellAppMain( UINTN Argc, 
              CHAR16 **Argv )
{
    EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop;
    EFI_DEVICE_PATH_PROTOCOL     *Dpp;
    SHELL_FILE_HANDLE            FileHandle;
    EFI_FILE_INFO                *FileInfo = NULL;
    EFI_STATUS                   Status = EFI_SUCCESS;
    EFI_HANDLE                   *Handles = NULL;
    EFI_HANDLE                   *FileBuffer = NULL;
    BOOLEAN                      Verbose = FALSE;
    UINTN                        HandleCount = 0;
    UINTN                        FileSize;

    if (Argc == 2) {
        if (!StrCmp(Argv[1], L"--version") ||
            !StrCmp(Argv[1], L"-V")) {
            Print(L"Version: %s\n", UTILITY_VERSION);
            return Status;
        } else if (!StrCmp(Argv[1], L"--help") ||
            !StrCmp(Argv[1], L"-h")) {
            Usage(FALSE);
            return Status;
        } else if (Argv[1][0] == L'-') {
            Usage(TRUE);
            return Status;
        }
    } else if (Argc == 3) {
        if (!StrCmp(Argv[1], L"--verbose") ||
            !StrCmp(Argv[1], L"-v")) {
            Verbose = TRUE;
        } else if (Argv[1][0] == L'-') {
            Usage(FALSE);
            return Status;
        }
    } else {
        Usage(FALSE);
        return Status;
    }

    // Check last argument is not an option!  
    if (Argv[Argc-1][0] == L'-') {
        Usage(TRUE);
        return Status;
    }

    // Open the file (has to be LAST arguement on command line)
    Status = ShellOpenFileByName( Argv[Argc - 1], 
                                  &FileHandle,
                                  EFI_FILE_MODE_READ , 0);
    if (EFI_ERROR (Status)) {
        Print(L"ERROR: Could not open specified file [%d]\n", Status);
        return Status;
    }            

    // Allocate buffer for file contents
    FileInfo = ShellGetFileInfo( FileHandle );    
    FileBuffer = AllocateZeroPool( (UINTN)FileInfo->FileSize );
    if (FileBuffer == NULL) {
        Print(L"ERROR: File buffer. No memory resources\n");
        return (SHELL_OUT_OF_RESOURCES);   
    }

    // Read file contents into allocated buffer
    FileSize = (UINTN) FileInfo->FileSize;
    Status = ShellReadFile( FileHandle,
                            &FileSize,
                            FileBuffer );
    if (EFI_ERROR (Status)) {
        Print(L"ERROR: ShellReadFile failed [%d]\n", Status);
        goto cleanup;
    }            
  
    ShellCloseFile( &FileHandle );

    // Try locating GOP by handle
    Status = gBS->LocateHandleBuffer( ByProtocol,
                                      &gEfiGraphicsOutputProtocolGuid,
                                      NULL,
                                      &HandleCount,
                                      &Handles );
    if (EFI_ERROR (Status)) {
        Print(L"ERROR: No GOP handles found via LocateHandleBuffer\n");
        goto cleanup;
    } 

#ifdef DEBUG
    Print(L"Found %d GOP handles via LocateHandleBuffer\n", HandleCount);
#endif

    // Make sure we use the correct GOP handle
    Gop = NULL;
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
        Print(L"Exiting. Graphics console not found.\n");
        goto cleanup;
    }

    Status = CheckBMPHeader( Gop, FileBuffer, FileSize );
    if (EFI_ERROR (Status)) {
        goto cleanup;
    }

    if (Verbose) {
        PrintBMPHeader( FileBuffer );
    }

    DisplayImage( Gop, FileBuffer );
    
cleanup:
    FreePool( FileBuffer );
    return Status;
}
