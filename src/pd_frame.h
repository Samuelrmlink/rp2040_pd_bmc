/**
 * USB-C PD frame designed to accommodate 
 * additional attributes such as frame type 
 * (SOP, SOP', SOP'', Hard reset, etc.) as 
 * well as frame arrival timestamps. 
 */
typedef union {
    uint8_t raw_bytes[56];
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
	uint16_t hdr;          			// raw_bytes[10..11]
	union {
	    uint32_t obj[11];           // raw_bytes[12..55]
	    uint8_t data[44];           // raw_bytes[12..55]
	} __attribute__((packed));
    } __attribute__((packed));
} __attribute__((packed)) pd_frame;
