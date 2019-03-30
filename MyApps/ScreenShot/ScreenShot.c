//
//  Copyright (c) 2010-2019  Finnbarr P. Murphy.   All rights reserved.
//
//  Take screen snapshot 
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
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/PrintLib.h>

#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/Shell.h>

#include <IndustryStandard/Bmp.h>

EFI_GRAPHICS_OUTPUT_BLT_PIXEL EfiGraphicsColors[16] = {
    // B    G    R   reserved
    {0x00, 0x00, 0x00, 0x00},  // BLACK
    {0x98, 0x00, 0x00, 0x00},  // LIGHTBLUE
    {0x00, 0x98, 0x00, 0x00},  // LIGHTGREEN
    {0x98, 0x98, 0x00, 0x00},  // LIGHTCYAN
    {0x00, 0x00, 0x98, 0x00},  // LIGHTRED
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

typedef enum {
    Black = 0,
    LightBlue,
    LightGreen,
    LightCyan,
    LightRed,
    Magenta,
    Brown,
    LightGray,
    DarkGray,
    Blue,
    Lime,
    Cyan,
    Red,
    Fuchsia,
    Yellow,
    White
} COLOR;

#define UTILITY_VERSION L"20190314"
#undef DEBUG


// Determines the size of status square
#define STATUS_SQUARE_SIDE 10


BOOLEAN
IsUnicodeDecimalDigit( CHAR16 Char )
{
    return ((BOOLEAN)(Char >= L'0' && Char <= L'9'));
}


EFI_STATUS
UnicodeStringToInteger( CHAR16 *String,
                        UINTN  *Value )
{
    UINTN Result = 0;

    if (String == NULL || StrSize (String) == 0 || Value == NULL) {
        return (EFI_INVALID_PARAMETER);
    }

    while (IsUnicodeDecimalDigit(*String)) {
        if (!(Result <= (DivU64x32((((UINT64) ~0) - (*String - L'0')),10)))) {
            return (EFI_DEVICE_ERROR);
        }

        Result = MultU64x32(Result, 10) + (*String - L'0');
        String++;
    }

    *Value = Result;

    return (EFI_SUCCESS);
}


EFI_STATUS
ShowStatus( EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop,
            UINT8 Color, 
            UINTN StartX, 
            UINTN StartY,
            UINTN Width, 
            UINTN Height ) 
{
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL Square[STATUS_SQUARE_SIDE * STATUS_SQUARE_SIDE];
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL BackupTL[STATUS_SQUARE_SIDE * STATUS_SQUARE_SIDE];
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL BackupTR[STATUS_SQUARE_SIDE * STATUS_SQUARE_SIDE];
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL BackupBL[STATUS_SQUARE_SIDE * STATUS_SQUARE_SIDE];
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL BackupBR[STATUS_SQUARE_SIDE * STATUS_SQUARE_SIDE];
    
    // set square color
    for (UINTN i = 0 ; i < STATUS_SQUARE_SIDE * STATUS_SQUARE_SIDE; i++) {
        Square[i].Blue = EfiGraphicsColors[Color].Blue;
        Square[i].Green = EfiGraphicsColors[Color].Green;
        Square[i].Red = EfiGraphicsColors[Color].Red;
        Square[i].Reserved = 0x00;
    }
    
    Width = Width - STATUS_SQUARE_SIDE -1;
    Height = Height - STATUS_SQUARE_SIDE -1;
    
    // backup current squares
    Gop->Blt(Gop, BackupTL, EfiBltVideoToBltBuffer, StartX, StartY, 0, 0, STATUS_SQUARE_SIDE, STATUS_SQUARE_SIDE, 0);
    Gop->Blt(Gop, BackupTR, EfiBltVideoToBltBuffer, StartX + Width, StartY, 0, 0, STATUS_SQUARE_SIDE, STATUS_SQUARE_SIDE, 0);
    Gop->Blt(Gop, BackupBL, EfiBltVideoToBltBuffer, StartX, StartY + Height, 0, 0, STATUS_SQUARE_SIDE, STATUS_SQUARE_SIDE, 0);
    Gop->Blt(Gop, BackupBR, EfiBltVideoToBltBuffer, StartX + Width, StartY + Height, 0, 0, STATUS_SQUARE_SIDE, STATUS_SQUARE_SIDE, 0);
        
    // draw status square
    Gop->Blt(Gop, Square, EfiBltBufferToVideo, 0, 0, StartX, StartY, STATUS_SQUARE_SIDE, STATUS_SQUARE_SIDE, 0);
    Gop->Blt(Gop, Square, EfiBltBufferToVideo, 0, 0, StartX + Width, StartY, STATUS_SQUARE_SIDE, STATUS_SQUARE_SIDE, 0);
    Gop->Blt(Gop, Square, EfiBltBufferToVideo, 0, 0, StartX, StartY + Height, STATUS_SQUARE_SIDE, STATUS_SQUARE_SIDE, 0);
    Gop->Blt(Gop, Square, EfiBltBufferToVideo, 0, 0, StartX + Width, StartY + Height, STATUS_SQUARE_SIDE, STATUS_SQUARE_SIDE, 0);
        
    // wait 500ms
    gBS->Stall(500*1000);
        
    // restore squares from backups
    Gop->Blt(Gop, BackupTL, EfiBltBufferToVideo, 0, 0, StartX, StartY, STATUS_SQUARE_SIDE, STATUS_SQUARE_SIDE, 0);
    Gop->Blt(Gop, BackupTR, EfiBltBufferToVideo, 0, 0, StartX + Width, StartY, STATUS_SQUARE_SIDE, STATUS_SQUARE_SIDE, 0);
    Gop->Blt(Gop, BackupBL, EfiBltBufferToVideo, 0, 0, StartX, StartY + Height, STATUS_SQUARE_SIDE, STATUS_SQUARE_SIDE, 0);
    Gop->Blt(Gop, BackupBR, EfiBltBufferToVideo, 0, 0, StartX + Width, StartY + Height, STATUS_SQUARE_SIDE, STATUS_SQUARE_SIDE, 0);
    
    return EFI_SUCCESS;
}


//
// Save image to file
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


EFI_STATUS
PrepareBMPFile( EFI_GRAPHICS_OUTPUT_BLT_PIXEL *BltBuffer,
                UINT32 Width, 
                UINT32 Height )
{
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Pixel;
    BMP_IMAGE_HEADER  *BmpHeader;
    EFI_STATUS        Status = EFI_SUCCESS;
    EFI_TIME          Time;
    CHAR16            FileName[40]; 
    UINT8             *FileData;
    UINTN             FileDataLength;
    UINT8             *ImagePtr;
    UINT8             *ImagePtrBase;
    UINTN             ImageLineOffset;
    UINTN             x, y;

    ImageLineOffset = Width * 3;
    if ((ImageLineOffset % 4) != 0) {
        ImageLineOffset = ImageLineOffset + (4 - (ImageLineOffset % 4));
    }

    // allocate buffer for data
    FileDataLength = sizeof(BMP_IMAGE_HEADER) + Height * ImageLineOffset;
    FileData = AllocateZeroPool( FileDataLength );
    if (FileData == NULL) {
        FreePool( BltBuffer );
        Print(L"ERROR: AllocateZeroPool. No memony resources\n");
        return EFI_OUT_OF_RESOURCES;
    }
    
    // fill header
    BmpHeader = (BMP_IMAGE_HEADER *)FileData;
    BmpHeader->CharB = 'B';
    BmpHeader->CharM = 'M';
    BmpHeader->Size = (UINT32)FileDataLength;
    BmpHeader->ImageOffset = sizeof(BMP_IMAGE_HEADER);
    BmpHeader->HeaderSize = 40;
    BmpHeader->PixelWidth = Width;
    BmpHeader->PixelHeight = Height;
    BmpHeader->Planes = 1;
    BmpHeader->BitPerPixel = 24;
    BmpHeader->CompressionType = 0;
    BmpHeader->XPixelsPerMeter = 0;
    BmpHeader->YPixelsPerMeter = 0;
    
    // fill pixel buffer
    ImagePtrBase = FileData + BmpHeader->ImageOffset;
    for (y = 0; y < Height; y++) {
        ImagePtr = ImagePtrBase;
        ImagePtrBase += ImageLineOffset;
        Pixel = BltBuffer + (Height - 1 - y) * Width;
        
        for (x = 0; x < Width; x++) {
            *ImagePtr++ = Pixel->Blue;
            *ImagePtr++ = Pixel->Green;
            *ImagePtr++ = Pixel->Red;
            Pixel++;
        }
    }

    FreePool(BltBuffer);

    Status = gRT->GetTime(&Time, NULL);
    if (!EFI_ERROR(Status)) {
        UnicodeSPrint( FileName, 62, L"screenshot-%04d%02d%02d-%02d%02d%02d.bmp", 
                       Time.Year, Time.Month, Time.Day, Time.Hour, Time.Minute, Time.Second );
    } else {
        UnicodeSPrint( FileName, 30, L"screenshot.bmp" );
    }
    
    SaveImage( FileName, FileData, FileDataLength );

    FreePool( FileData );

    return Status;
}


EFI_STATUS
SnapShot( EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop, 
          UINTN                        StartX, 
          UINTN                        StartY,
          UINTN                        Width, 
          UINTN                        Height ) 
{
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL *BltBuffer = NULL;
    EFI_STATUS Status = EFI_SUCCESS;
    UINTN      ImageSize;  

    ImageSize = Width * Height;
            
    // allocate memory for snapshot
    BltBuffer = AllocateZeroPool( sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL) * ImageSize );
    if (BltBuffer == NULL) {
        Print(L"ERROR: BltBuffer. No memory resources\n");
        return EFI_OUT_OF_RESOURCES;
    }

    // take screenshot
    Status = Gop->Blt( Gop, BltBuffer, EfiBltVideoToBltBuffer, StartX, StartY, 0, 0, Width, Height, 0 );
    if (EFI_ERROR(Status)) {
        Print(L"ERROR: Gop->Blt [%d]\n", Status);
        FreePool( BltBuffer );
        return Status;
    }
            
    PrepareBMPFile( BltBuffer, Width, Height );

    return Status;
}


VOID
Usage( BOOLEAN ErrorMsg )
{
    if ( ErrorMsg ) {
        Print(L"ERROR: Unknown option(s).\n");
    }

    Print(L"Usage: ScreenShot [StartX StartY WidthX HeightY]\n");
    Print(L"       ScreenShot [-i | --info]\n");
    Print(L"       ScreenShot [-V | --version]\n");
}


INTN
EFIAPI
ShellAppMain( UINTN Argc,
              CHAR16 **Argv )
{
    EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop = NULL;
    EFI_DEVICE_PATH_PROTOCOL     *Dpp;
    EFI_STATUS                   Status = EFI_SUCCESS;
    EFI_HANDLE                   *Handles = NULL;
    BOOLEAN                      DisplayInfo = FALSE;
    UINTN                        HandleCount = 0;
    UINTN                        StartX = 0, StartY = 0, Width = 0, Height = 0; 

    if (Argc == 2) {
        if (!StrCmp(Argv[1], L"--version") ||
            !StrCmp(Argv[1], L"-V")) {
            Print(L"Version: %s\n", UTILITY_VERSION);
            return Status;
        } else if (!StrCmp(Argv[1], L"--help") ||
            !StrCmp(Argv[1], L"-h")) {
            Usage(FALSE);
            return Status;
        } else if (!StrCmp(Argv[1], L"--info") ||
            !StrCmp(Argv[1], L"-i")) {
            DisplayInfo = TRUE;
        } else if (Argv[1][0] == L'-') {
            Usage(TRUE);
            return Status;
        }
    } else if (Argc == 5) {
         UnicodeStringToInteger( Argv[1], &StartX );
         UnicodeStringToInteger( Argv[2], &StartY );
         UnicodeStringToInteger( Argv[3], &Width );
         UnicodeStringToInteger( Argv[4], &Height );
    } else if (Argc > 1) {
        Usage(FALSE);
        return Status;
    }

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
    FreePool( Handles );
    if (Gop == NULL) {
        Print(L"ERROR: No graphics console found.\n");
        return Status;
    }

    if ( DisplayInfo ) {
        Print(L"\nScreen - Width: %d\n", Gop->Mode->Info->HorizontalResolution);
        Print(L"        Height: %d\n\n", Gop->Mode->Info->VerticalResolution);
        return Status;
    }
 
    if ( Width == 0 )
        Width = Gop->Mode->Info->HorizontalResolution;
    if ( Height == 0 )
        Height = Gop->Mode->Info->VerticalResolution;

    Status = ShowStatus( Gop, Yellow, StartX, StartY, Width, Height ); 
    gBS->Stall(500*1000);
    Status = SnapShot( Gop , StartX, StartY, Width, Height );
    if (EFI_ERROR(Status)) {
        Status = ShowStatus( Gop, Red, StartX, StartY, Width, Height ); 
    } else {
        Status = ShowStatus( Gop, Lime, StartX, StartY, Width, Height ); 
    }

    return Status;
}
