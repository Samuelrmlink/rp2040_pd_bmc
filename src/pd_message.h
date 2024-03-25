#ifndef _PD_MESSAGE_H
#define _PD_MESSAGE_H

#include <stdio.h>

// USB-PD Message Type
typedef enum {
    // Control messages (no extended bit in header && no data objects)
    controlMsgGoodCrc		= 0x1,
    controlMsgGotoMin		= 0x2,
    controlMsgAccept		= 0x3,
    controlMsgReject		= 0x4,
    controlMsgPing		= 0x5,
    controlMsgPsReady		= 0x6,
    controlMsgGetSourceCap	= 0x7,
    controlMsgGetSinkCap	= 0x8,
    controlMsgDataRoleSwap 	= 0x9,
    controlMsgPowerRoleSwap 	= 0xa,
    controlMsgVconnSwap		= 0xb,
    controlMsgWait		= 0xc,
    controlMsgSoftReset		= 0xd,
    controlMsgDataReset		= 0xe,
    controlMsgDataResetComplete	= 0xf,
    controlMsgNotSupported	= 0x10,
    controlMsgGetSourceCapExt	= 0x11,
    controlMsgGetStatus		= 0x12,
    controlMsgFastRoleSwap	= 0x13,
    controlMsgGetPpsStatus	= 0x14,
    controlMsgGetCountryCodes	= 0x15,
    controlMsgGetSinkCapExt	= 0x16,

    // Data messages (no extended bit && data objects)
    dataMsgSourceCap		= 0x41,
    dataMsgRequest		= 0x42,
    dataMsgBist			= 0x43,
    dataMsgSinkCap		= 0x44,
    dataMsgBatteryStatus	= 0x45,
    dataMsgAlert		= 0x46,
    dataMsgGetCountryInfo	= 0x47,
    dataMsgEnterUsb		= 0x48,
    dataMsgEprRequest		= 0x49,
    dataMsgEprMode		= 0x4a,
    dataMsgSourceInfo		= 0x4b,
    dataMsgRevision		= 0x4c,
    dataMsgVendorDefined	= 0x4f,

    // Extended messages (extended bit in header)
    extMsgSourceCapExt		= 0x81,
    extMsgStatus		= 0x82,
    extMsgGetBatteryCap		= 0x83,
    extMsgGetBatteryStatus	= 0x84,
    extMsgBatteryCap		= 0x85,
    extMsgGetManufacturerInfo	= 0x86,
    extMsgManufacturerInfo	= 0x87,
    extMsgSecurityRequest	= 0x88,
    extMsgSecurityResponse	= 0x89,
    extMsgFirmwareUpdateRequest	= 0x8a,
    extMsgFirmwareUpdateResponse= 0x8b,
    extMsgPpsStatus		= 0x8c,
    extMsgCountryInfo		= 0x8d,
    extMsgCountryCodes		= 0x8e,
    extMsgSinkCapExt		= 0x8f,
    extMsgExtControl		= 0x90,
    extMsgEprSourceCap		= 0x91,
    extMsgEprSinkCap		= 0x92,
    extMsgVendorDefinedExt	= 0x9e
} PDMessageType;

#endif
