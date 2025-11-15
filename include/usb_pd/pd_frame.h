#define MAX_BYTES_IN_PDFRAME_STRUCT 60

/**
 * USB-C PD frame designed to accommodate 
 * additional attributes such as frame type 
 * (SOP, SOP', SOP'', Hard reset, etc.) as 
 * well as frame arrival timestamps. 
 */
typedef union {
    uint8_t raw_bytes[MAX_BYTES_IN_PDFRAME_STRUCT];
    struct {
    uint32_t timestamp_us;          // raw_bytes[0..3]
    union {
        uint32_t ordered_set;       // raw_bytes[4..7]
        struct {
        uint8_t kcode1;             // raw_bytes[4]
        uint8_t kcode2;             // raw_bytes[5]
        uint8_t kcode3;             // raw_bytes[6]
        uint8_t kcode4;             // raw_bytes[7]
        } __attribute__((packed));
    } __attribute__((packed));
    uint16_t __padding1;            // raw_bytes[8..9]
    uint16_t hdr;                   // raw_bytes[10..11]
    union {
        // 11 32-bit objects + 1 32-bit CRC
        uint32_t obj[12];           // raw_bytes[12..55]
        uint8_t data[48];           // raw_bytes[12..55]
    } __attribute__((packed));
    } __attribute__((packed));
} __attribute__((packed)) pd_frame;

typedef enum {
    pdfTypeInvalid,     // 0
    pdfTypeHardReset,   // 1
    pdfTypeCableReset,  // 2
    pdfTypeSop,         // 3
    pdfTypeSopP,        // 4
    pdfTypeSopDp,       // 5
    pdfTypeSopPDbg,     // 6
    pdfTypeSopDpDbg     // 7
} pdfTypes;

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
    controlMsgGoodCrc           = 0x1,
    controlMsgGotoMin           = 0x2,
    controlMsgAccept            = 0x3,
    controlMsgReject            = 0x4,
    controlMsgPing              = 0x5,
    controlMsgPsReady           = 0x6,
    controlMsgGetSourceCap      = 0x7,
    controlMsgGetSinkCap        = 0x8,
    controlMsgDataRoleSwap      = 0x9,
    controlMsgPowerRoleSwap     = 0xa,
    controlMsgVconnSwap         = 0xb,
    controlMsgWait              = 0xc,
    controlMsgSoftReset         = 0xd,
    controlMsgDataReset         = 0xe,
    controlMsgDataResetComplete = 0xf,
    controlMsgNotSupported      = 0x10,
    controlMsgGetSourceCapExt   = 0x11,
    controlMsgGetStatus         = 0x12,
    controlMsgFastRoleSwap      = 0x13,
    controlMsgGetPpsStatus      = 0x14,
    controlMsgGetCountryCodes   = 0x15,
    controlMsgGetSinkCapExt     = 0x16,

    // Data messages (no extended bit && data objects)
    dataMsgSourceCap            = 0x41,
    dataMsgRequest              = 0x42,
    dataMsgBist                 = 0x43,
    dataMsgSinkCap              = 0x44,
    dataMsgBatteryStatus        = 0x45,
    dataMsgAlert                = 0x46,
    dataMsgGetCountryInfo       = 0x47,
    dataMsgEnterUsb             = 0x48,
    dataMsgEprRequest           = 0x49,
    dataMsgEprMode              = 0x4a,
    dataMsgSourceInfo           = 0x4b,
    dataMsgRevision             = 0x4c,
    dataMsgVendorDefined        = 0x4f,

    // Extended messages (extended bit in header)
    extMsgSourceCapExt          = 0x81,
    extMsgStatus                = 0x82,
    extMsgGetBatteryCap         = 0x83,
    extMsgGetBatteryStatus      = 0x84,
    extMsgBatteryCap            = 0x85,
    extMsgGetManufacturerInfo   = 0x86,
    extMsgManufacturerInfo      = 0x87,
    extMsgSecurityRequest       = 0x88,
    extMsgSecurityResponse      = 0x89,
    extMsgFirmwareUpdateRequest = 0x8a,
    extMsgFirmwareUpdateResponse= 0x8b,
    extMsgPpsStatus             = 0x8c,
    extMsgCountryInfo           = 0x8d,
    extMsgCountryCodes          = 0x8e,
    extMsgSinkCapExt            = 0x8f,
    extMsgExtControl            = 0x90,
    extMsgEprSourceCap          = 0x91,
    extMsgEprSinkCap            = 0x92,
    extMsgVendorDefinedExt      = 0x9e
} PDMessageType;

typedef enum {
    controlMsg,
    dataMsg,
    extMsg
} pdMsgType;

// USB-PD Message Type - strings array
static const char* const pdMsgControlTypeNames[] = {
    "GoodCRC",                  // controlMsgGoodCrc
    "GotoMin",                  // controlMsgGotoMin
    "Accept",                   // controlMsgAccept
    "Reject",                   // controlMsgReject
    "Ping",                     // controlMsgPing
    "PS_Ready",                 // controlMsgPsReady
    "Get_Source_Cap",           // controlMsgGetSourceCap
    "Get_Sink_Cap",             // controlMsgGetSinkCap
    "Data_Role_Swap ",          // controlMsgDataRoleSwap
    "Power_Role_Swap ",         // controlMsgPowerRoleSwap
    "VCONN_Swap",               // controlMsgVconnSwap
    "Wait",                     // controlMsgWait
    "Soft_Reset",               // controlMsgSoftReset
    "Data_Reset",               // controlMsgDataReset
    "Data_Reset_Complete",      // controlMsgDataResetComplete
    "Not_Supported",            // controlMsgNotSupported
    "Get_Source_Cap_Ext",       // controlMsgGetSourceCapExt
    "Get_Status",               // controlMsgGetStatus
    "Fast_Role_Swap",           // controlMsgFastRoleSwap
    "Get_PPS_Status",           // controlMsgGetPpsStatus
    "Get_Country_Codes",        // controlMsgGetCountryCodes
    "Get_Sink_Cap_Ext"          // controlMsgGetSinkCapExt
};
static const char* const pdMsgDataTypeNames[] = {
    "Source Capabilities",      // dataMsgSourceCap
    "Request",                  // dataMsgRequest
    "BIST",                     // dataMsgBist
    "Sink Capabilities",        // dataMsgSinkCap
    "Battery_Status",           // dataMsgBatteryStatus
    "Alert",                    // dataMsgAlert
    "Get_Country_Info",         // dataMsgGetCountryInfo
    "Enter_USB",                // dataMsgEnterUsb
    "EPR_Request",              // dataMsgEprRequest
    "EPR_Mode",                 // dataMsgEprMode
    "Source_Info",              // dataMsgSourceInfo
    "Revision",                 // dataMsgRevision
    "Vendor_Defined"            // dataMsgVendorDefined
};
static const char* const pdMsgExtTypeNames[] = {
    "Source_Cap_Ext",           // extMsgSourceCapExt
    "Status",                   // extMsgStatus
    "Get_Battery_Cap",          // extMsgGetBatteryCap
    "Get_Battery_Status",       // extMsgGetBatteryStatus
    "Battery_Cap",              // extMsgBatteryCap
    "Get_Manufacturer_Info",    // extMsgGetManufacturerInfo
    "Manufacturer_Info",        // extMsgManufacturerInfo
    "Security_Request",         // extMsgSecurityRequest
    "Security_Response",        // extMsgSecurityResponse
    "Firmware_Update_Request"   // extMsgFirmwareUpdateRequest
};
typedef enum {
    pdoTypeFixed,
    pdoTypeBattery,
    pdoTypeVariable,
    pdoTypeAugmented
} pdoBaseTypes;
typedef enum {
    pdoTypeAugmentedSprPps,
    pdoTypeAugmentedEprAvs,
    pdoTypeAugmentedSprAvs,
    pdoTypeAugmentedReserved
} pdoAugmentedTypes;

static const char* const pdoBaseTypeNames[] = {
    "Fixed",
    "Battery",
    "Variable",
    "Augmented"
};
static const char* const pdoAugmentedTypeNames[] = {
    "SPR PPS",
    "EPR AVS",
    "SPR AVS",
    "Reserved"
};

typedef enum {
    pdSpecRev1,
    pdSpecRev2,
    pdSpecRev3,
    pdSpecReserved
} pdSpecRevisions;
static const char* const pdSpecRevisionNames[] = {
    "2.0",
    "3.0",
    "3.1",
    "Reserved" // 3.2 does not use this
};

uint typec_pdframe_orderedset_get_idx(uint32_t input);
bool typec_pdframe_valid(pd_frame *pdf);
uint typec_pdframe_extended_unchunked_bytes(pd_frame *pdf);
void typec_pdframe_generate_goodcrc(pd_frame *input_frame, pd_frame *output_frame);
uint typec_pdframe_get_msgid(pd_frame *pdf);
void typec_pdframe_set_msgid(pd_frame *pdf, uint msgid);
void typec_pdframe_inc_msgid(pd_frame *pdf);
bool typec_pdframe_compare(pd_frame *pdf_a, pd_frame *pdf_b);
