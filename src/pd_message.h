#ifndef _PD_MESSAGE_H
#define _PD_MESSAGE_H

#include <stdio.h>


// USB-PD Frame Type - strings array
typedef enum {
    PdfTypeInvalid,
    PdfTypeHardReset,
    PdfTypeCableReset,
    PdfTypeSop,
    PdfTypeSopP,
    PdfTypeSopDp,
    PdfTypeSopPDbg,
    PdfTypeSopDpDbg
} PDFrameType;

// USB-PD Frame Type - strings array
static const char* const sopFrameTypeNames[] = {
    NULL,
    "Hard Rst",
    "Cable Rst",
    "SOP",
    "SOP'",
    "SOP\"",
    "SOP' Dbg",
    "SOP\" Dbg"
};

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

// USB-PD Message Type - strings array
static const char* const pdMsgTypeNames[] = {
    [(int)controlMsgGoodCrc]		= "GoodCRC",
    [(int)controlMsgGotoMin]		= "GotoMin",
    [(int)controlMsgAccept]		= "Accept",
    [(int)controlMsgReject]		= "Reject",
    [(int)controlMsgPing]		= "Ping",
    [(int)controlMsgPsReady]		= "PS_Ready",
    [(int)controlMsgGetSourceCap]	= "Get_Source_Cap",
    [(int)controlMsgGetSinkCap]		= "Get_Sink_Cap",
    [(int)controlMsgDataRoleSwap ]	= "Data_Role_Swap ",
    [(int)controlMsgPowerRoleSwap ]	= "Power_Role_Swap ",
    [(int)controlMsgVconnSwap]		= "VCONN_Swap",
    [(int)controlMsgWait]		= "Wait",
    [(int)controlMsgSoftReset]		= "Soft_Reset",
    [(int)controlMsgDataReset]		= "Data_Reset",
    [(int)controlMsgDataResetComplete]	= "Data_Reset_Complete",
    [(int)controlMsgNotSupported]	= "Not_Supported",
    [(int)controlMsgGetSourceCapExt]	= "Get_Source_Cap_Ext",
    [(int)controlMsgGetStatus]		= "Get_Status",
    [(int)controlMsgFastRoleSwap]	= "Fast_Role_Swap",
    [(int)controlMsgGetPpsStatus]	= "Get_PPS_Status",
    [(int)controlMsgGetCountryCodes]	= "Get_Country_Codes",
    [(int)controlMsgGetSinkCapExt]	= "Get_Sink_Cap_Ext",
    [(int)dataMsgSourceCap]		= "Source Capabilities",
    [(int)dataMsgRequest]		= "Request",
    [(int)dataMsgBist]			= "BIST",
    [(int)dataMsgSinkCap]		= "Sink Capabilities",
    [(int)dataMsgBatteryStatus]		= "Battery_Status",
    [(int)dataMsgAlert]			= "Alert",
    [(int)dataMsgGetCountryInfo]	= "Get_Country_Info",
    [(int)dataMsgEnterUsb]		= "Enter_USB",
    [(int)dataMsgEprRequest]		= "EPR_Request",
    [(int)dataMsgEprMode]		= "EPR_Mode",
    [(int)dataMsgSourceInfo]		= "Source_Info",
    [(int)dataMsgRevision]		= "Revision",
    [(int)dataMsgVendorDefined]		= "Vendor_Defined",
    [(int)extMsgSourceCapExt]		= "Source_Cap_Ext",
    [(int)extMsgStatus]			= "Status",
    [(int)extMsgGetBatteryCap]		= "Get_Battery_Cap",
    [(int)extMsgGetBatteryStatus]	= "Get_Battery_Status",
    [(int)extMsgBatteryCap]		= "Battery_Cap",
    [(int)extMsgGetManufacturerInfo]	= "Get_Manufacturer_Info",
    [(int)extMsgManufacturerInfo]	= "Manufacturer_Info",
    [(int)extMsgSecurityRequest]	= "Security_Request",
    [(int)extMsgSecurityResponse]	= "Security_Response",
    [(int)extMsgFirmwareUpdateRequest]	= "Firmware_Update_Request",
};
typedef enum {
    pdoTypeFixed,
    pdoTypeBattery,
    pdoTypeVariable,
    pdoTypeAugmented
} pdoTypes;

struct pdo_accept_criteria {
    uint32_t mV_min;
    uint32_t mV_max;
    uint16_t mA_min;
    uint16_t mA_max;
};
typedef struct pdo_accept_criteria pdo_accept_criteria;

PDMessageType pdf_get_sop_msg_type(pd_frame *msg);

#endif
