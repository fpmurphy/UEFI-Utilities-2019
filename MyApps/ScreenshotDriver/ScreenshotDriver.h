//
//  Copyright (c) 2010-2019  Finnbarr P. Murphy.   All rights reserved.
//
//  Screen snapshot driver 
//
//  License: BSD 2 clause License
//
//  Portions Copyright (c) 2018, Intel Corporation. All rights reserved.
//           See relevant code in EDK11 for exact details
//


#ifndef _SCREENSHOTDRIVER_H_
#define _SCREENSHOTDRIVER_H_

#define SCREENSHOTDRIVER_DEV_SIGNATURE SIGNATURE_32 ('s', 's', 'd', 'r')

typedef struct {
    UINT32                   Signature;
    EFI_HANDLE               Handle;
    VOID                     *ScreenshotDriverVariable;
    UINT8                    DeviceType;
    BOOLEAN                  FixedDevice;
    UINT16                   Reserved;
    EFI_UNICODE_STRING_TABLE *ControllerNameTable;
} SCREENSHOTDRIVER_DEV;

#define SCREENSHOTDRIVER_DEV_FROM_THIS(a)  CR (a, SCREENSHOTDRIVER_DEV, ScreenshotDriverVariable, SCREENSHOTDRIVER_DEV_SIGNATURE)

// Global Variables
extern EFI_DRIVER_BINDING_PROTOCOL        gScreenshotDriverBinding;
extern EFI_COMPONENT_NAME2_PROTOCOL       gScreenshotComponentName2;
extern EFI_DRIVER_DIAGNOSTICS2_PROTOCOL   gScreenshotDriverDiagnostics2;
extern EFI_DRIVER_CONFIGURATION2_PROTOCOL gScreenshotDriverConfiguration2;


EFI_STATUS
EFIAPI
ScreenshotDriverBindingSupported( IN EFI_DRIVER_BINDING_PROTOCOL *This,
                                  IN EFI_HANDLE                  Controller,
                                  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath );

EFI_STATUS
EFIAPI
ScreenshotDriverBindingStart( IN EFI_DRIVER_BINDING_PROTOCOL *This,
                              IN EFI_HANDLE                  Controller,
                              IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath );

EFI_STATUS
EFIAPI
ScreenshotDriverBindingStop( IN EFI_DRIVER_BINDING_PROTOCOL *This,
                             IN EFI_HANDLE                  Controller,
                             IN UINTN                       NumberOfChildren,
                             IN EFI_HANDLE                  *ChildHandleBuffer );

EFI_STATUS
EFIAPI
ScreenshotDriverComponentNameGetDriverName( IN  EFI_COMPONENT_NAME2_PROTOCOL *This,
                                            IN  CHAR8                        *Language,
                                            OUT CHAR16                       **DriverName );

EFI_STATUS
EFIAPI
ScreenshotDriverComponentNameGetControllerName( IN  EFI_COMPONENT_NAME2_PROTOCOL *This,
                                                IN  EFI_HANDLE                   ControllerHandle,
                                                IN  EFI_HANDLE                   ChildHandleL,
                                                IN  CHAR8                        *Language,
                                                OUT CHAR16                       **ControllerName );
 
EFI_STATUS
EFIAPI
ScreenshotDriverRunDiagnostics( IN  EFI_DRIVER_DIAGNOSTICS2_PROTOCOL *This,
                                IN  EFI_HANDLE                       ControllerHandle,
                                IN  EFI_HANDLE                       ChildHandle,
                                IN  EFI_DRIVER_DIAGNOSTIC_TYPE       DiagnosticType,
                                IN  CHAR8                            *Language,
                                OUT EFI_GUID                         **ErrorType,
                                OUT UINTN                            *BufferSize,
                                OUT CHAR16                           **Buffer );

EFI_STATUS
EFIAPI
ScreenshotDriverConfigurationSetOptions( IN  EFI_DRIVER_CONFIGURATION2_PROTOCOL       *This,
                                         IN  EFI_HANDLE                               ControllerHandle,
                                         IN  EFI_HANDLE                               ChildHandle,
                                         IN  CHAR8                                    *Language,
                                         OUT EFI_DRIVER_CONFIGURATION_ACTION_REQUIRED *ActionRequired );

EFI_STATUS
EFIAPI
ScreenshotDriverConfigurationOptionsValid( IN EFI_DRIVER_CONFIGURATION2_PROTOCOL *This,
                                           IN EFI_HANDLE                         ControllerHandle,
                                           IN EFI_HANDLE                         ChildHandle );

EFI_STATUS
EFIAPI
ScreenshotDriverConfigurationForceDefaults( IN  EFI_DRIVER_CONFIGURATION2_PROTOCOL       *This,
                                            IN  EFI_HANDLE                               ControllerHandle,
                                            IN  EFI_HANDLE                               ChildHandle,
                                            IN  UINT32                                   DefaultType,
                                            OUT EFI_DRIVER_CONFIGURATION_ACTION_REQUIRED *ActionRequired );

#endif // _SCREENSHOTDRIVER_H_
