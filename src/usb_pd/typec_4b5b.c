#include "main_i.h"

#define NUM_BITS_SYMBOL 5

// Ensures that the raw bits are aligned before interpretation
//      * Does NOT modify the input data - but DOES change the input_offset variable if needed
//      Returns true if preamble is found & in alignment
static bool typec_4b5b_preamble_align(uint *input_offset, uint32_t in) {
    uint offset = 0;
    bool aligned;
    switch(in & 0x0FFFFFF0) {
        case(0x05555550):
            offset = 1;
            // Falls through..
        case(0x0AAAAAA0):
            // Preamble has been found
            aligned = true;
            break;
        default:
            aligned = false;
            break;
    }
    // Consume all bits (except for offset one)
    *input_offset = 32 - offset;
    // Return
    return aligned;
}
/* Locates the 'Ordered Set' - which does the following:
 *      * Specifies the purpose of the received frame
 *          SOP         - Communication with port-partner (device attached to the other end of a USB-C cable - this could be a phone, laptop, wall charger, battery pack, etc.)
 *          SOP'        - Communication with an E-Marker (typically used to identify the capabilities of an attached cable)
 *          SOP"        - Similar to above
 *      * Provides some low-level hardware resets
 *          Cable Reset - Used to reset the USB Type-C E-Marker state machines
 *          Hard Reset  - Terminates any active power contract & resets the state machines on the attached devices
*/
static bool typec_4b5b_find_ordered_set(uint *input_offset, uint *after_scrap_offset, uint *scrap_bits, uint *output_offset, uint32_t in) {
    bool found_ordset = false;
    while(*input_offset + *after_scrap_offset <= 30) {
        if( (*input_offset + *after_scrap_offset <= 27 && (
            ((*scrap_bits | (in >> *input_offset) << *after_scrap_offset) & 0x1F) == symKcodeS1 ||  // K-Code S1 (starting symbol for ordsetSop*) ---or---
            ((*scrap_bits | (in >> *input_offset) << *after_scrap_offset) & 0x1F) == symKcodeR1))) {  // K-Code R1 (starting symbol for ordsetHardReset, ordsetCableReset)
                // Ordered Set HAS been found
                found_ordset = true;
                break;
        } else {
            // Ordered Set has not been found yet
            // Consume bits - while maintaining alignment with the preamble
            uint num_scrap_consuming;
            (*after_scrap_offset > 1) ? (num_scrap_consuming = 2) : (num_scrap_consuming = 1);
            // Use scrap bits first
            if(*after_scrap_offset) {
                *after_scrap_offset -= num_scrap_consuming;
                *scrap_bits >>= num_scrap_consuming;
            }
            // Ensure that we are maintaining preamble alignment
            *input_offset += (2 - num_scrap_consuming);
        }
    }
    // Save scrap bits
    if(*input_offset > 27) {
        *scrap_bits = in >> *input_offset;
        *after_scrap_offset = 32 - *input_offset;
    }
    return found_ordset;
}
// Returns 4 bits (combined from both scrap and input)
static uint typec_4b5b_pull_5b(uint *input_offset, uint *after_scrap_offset, uint *scrap_bits, uint32_t input) {
    uint decode_4b = bmc5bTo4b[(*scrap_bits | (input >> *input_offset) << *after_scrap_offset) & 0x1F];
    // Increment to account for bits consumed from input
    *input_offset += 5 - *after_scrap_offset;
    // We can reset to zero since the *scrap_bits variable never holds more than 4 bits
    *scrap_bits = 0;
    *after_scrap_offset = 0;
    return decode_4b;
}
// Returns true if EOP is found
static bool typec_4b5b_symbols_decode(uint *input_offset, uint *after_scrap_offset, uint *scrap_bits, uint *output_offset, uint32_t input, pd_frame *pdf) {
    uint decoded_4b;
    static bool upper_symbol;
    // Exit if there aren't enough bits to process
    if(*input_offset + *after_scrap_offset > 27) { return false; }
    while(*input_offset + *after_scrap_offset <= 27) {
        decoded_4b = typec_4b5b_pull_5b(input_offset, after_scrap_offset, scrap_bits, input);
        // Ensure that hexadecimal data discovered near the start of frame ends up as header
        if(*output_offset < 10 && !(decoded_4b & 0x10)) {
            *output_offset = 10;
            // Check whether this is a Hard/Cable Reset - those types don't have an EOP symbol
            uint ordset_idx = typec_pdframe_orderedset_get_idx(pdf->ordered_set);
            if(ordset_idx == pdfTypeHardReset || ordset_idx == pdfTypeCableReset) { return true; }
        }
        if(decoded_4b == sym4bKcodeEop) {
            // End of panel symbol
            return true;
        }
        if(*output_offset >= MAX_BYTES_IN_PDFRAME_STRUCT) {
            // Overflow protection
            printf("\nOVERFLOW\n");
            pdf->timestamp_us = 0;
            return true;
        }
        assert(*output_offset < MAX_BYTES_IN_PDFRAME_STRUCT);
        pdf->raw_bytes[*output_offset] |= decoded_4b << (4 * (upper_symbol & 1u));
        if(upper_symbol || decoded_4b & 0x10) {
            upper_symbol = false;
            *output_offset += 1;
        } else {
            upper_symbol = true;
        }
    }
    if(*input_offset > 27) {
        *scrap_bits = input >> *input_offset;
        *after_scrap_offset = 32 - *input_offset;
    }
    return false;
}
bool typec_4b5b_decode(pd_frame *pdf, uint32_t raw_data) {
    static uint input_offset;       // [ >> shift direction ]
    static uint after_scrap_offset; // [ << shift direction ]
    static uint scrap_bits;
    static uint output_offset;
    static bool preamble_aligned;
    bool ret = false;
    assert(output_offset < MAX_BYTES_IN_PDFRAME_STRUCT);
    // Ensure preamble alignment
    if(!preamble_aligned) {
        scrap_bits = 0;
        after_scrap_offset = 0;
        if(typec_4b5b_preamble_align(&input_offset, raw_data)) {
            preamble_aligned = true;
            pdf->timestamp_us = time_us_32();
        }
    }
    // Locate first symbol of the Ordered Set
    if(!output_offset) {
        if(typec_4b5b_find_ordered_set(&input_offset, &after_scrap_offset, &scrap_bits, &output_offset, raw_data)) {
            // Ordered Set has been found
            output_offset = 4;
        }
    }
    if(typec_4b5b_symbols_decode(&input_offset, &after_scrap_offset, &scrap_bits, &output_offset, raw_data, pdf)) {
        // EOP Received
        // Reset internal variables
        preamble_aligned = false;
        output_offset = 0;
        scrap_bits = 0;
        after_scrap_offset = 0;
        // Set return flag
        ret = true;
    }
    input_offset = 0;
    return ret;
}
static uint typec_pretx_unchunked_bytes(pd_frame *pdf) {
    return (pdf->hdr >> 15) ? 2 + typec_pdframe_extended_unchunked_bytes(pdf) : 0;
}
uint typec_pretx_num_leading_zeros(uint32_t obj) {
    for(uint i = 0; i < 32; i++) {
        if(obj & (1u << i)) { return i - 1; }
    }
    return 0;
}
static void typec_pretx_buf_write(uint32_t input_bits, uint num_input_bits, uint32_t *out_buf, uint *out_buf_pos) {
    uint input_lshift = 0;
    // Loop while there is data to transfer
    while(num_input_bits) {
        // Determine how many bits we can transfer this round
        uint out_obj_empty_bits = 32 - (*out_buf_pos % 32);
        uint num_transfer_bits = num_input_bits <= out_obj_empty_bits ? num_input_bits : out_obj_empty_bits;
        // Transfer bits
        out_buf[*out_buf_pos / 32] |= ((input_bits >> input_lshift) & (0xFFFFFFFF >> (32 - num_transfer_bits))) << (*out_buf_pos % 32);
        *out_buf_pos += num_transfer_bits;
        num_input_bits -= num_transfer_bits;
        input_lshift += num_transfer_bits;
    }
}
// Convert pd_frame to raw bitstream
uint32_t* typec_pretx_convert(pd_frame *pdf) {
    // Calculate payload bytes (excluding K-Code symbols)
    uint num_payload_bytes =
        2 +                                 // Header
        (((pdf->hdr >> 12) & 0x7) * 4) +    // Data Objects
        typec_pretx_unchunked_bytes(pdf) +  // Ext Header/Payload
        4;                                  // CRC32
    // Calculate the number of output bits required
    uint16_t total_bits_req =
        64 + 5 * (              // Preamble + [5 bits per symbol]
        4 +                     // Ordered Set symbols
        2 * num_payload_bytes + // Payload (2 symbols per byte)
        1);                     // EOP symbol
    // Determine number of leading zeros
    uint num_zeros = (total_bits_req % 32) ? (32 - total_bits_req % 32) : 0; // Zero if no remainder
    // Determine number of u32 objects requires
    uint num_u32 = num_zeros ? total_bits_req / 32 + 1 : total_bits_req / 32;
    // Allocate u32 objects + 1 extra
    uint32_t *out = malloc(sizeof(uint32_t) * (num_u32 + 1));
    // The first u32 object will store the allocation size
    out[0] = (uint32_t)num_u32;
    // Clear output buffer allocation
    memset(&out[1], 0, sizeof(uint32_t) * num_u32);
    // Offset for leading zeros
    uint out_bit_pos = num_zeros;
    // Add preamble (64-bits)
    typec_pretx_buf_write(0xAAAAAAAA, 32, &out[1], &out_bit_pos);
    typec_pretx_buf_write(0xAAAAAAAA, 32, &out[1], &out_bit_pos);
    // Add the Ordered Set symbols
    uint input_byte_offset = 4;
    for(uint i = 0; i < 4; i++) {
        typec_pretx_buf_write(bmc4bTo5b[pdf->raw_bytes[input_byte_offset]], (uint)NUM_BITS_SYMBOL, &out[1], &out_bit_pos);
        input_byte_offset++;
    }
    // Add the Payload (non K-Code symbols)
    for(uint i = 0; i < num_payload_bytes; i++) {
        // Skip the __padding1 field in the pd_frame structure
        if(input_byte_offset == 8) { input_byte_offset = 10; }
        // Write both the upper and lower symbols (With the exception of K-Code symbols - each byte is represented by 2 symbols)
        typec_pretx_buf_write(bmc4bTo5b[(pdf->raw_bytes)[input_byte_offset] & 0xF], (uint)NUM_BITS_SYMBOL, &out[1], &out_bit_pos);
        typec_pretx_buf_write(bmc4bTo5b[((pdf->raw_bytes)[input_byte_offset] >> 4) & 0xF], (uint)NUM_BITS_SYMBOL, &out[1], &out_bit_pos);
        input_byte_offset++;
    }
    // Add the EOP symbol
    typec_pretx_buf_write(symKcodeEop, (uint)NUM_BITS_SYMBOL, &out[1], &out_bit_pos);
    return out;
}
uint32_t* typec_tx_convert(uint32_t *in, uint num_in_obj) {
    // We should have twice are many bits at the output
    uint num_out_obj = 2 * num_in_obj;
    uint32_t *out = malloc(sizeof(uint32_t) * num_out_obj);
    memset(out, 0, sizeof(uint32_t) * num_out_obj);
    for(uint i = 0; i < num_out_obj; i += 2) {
        uint input_obj = i / 2;
        out[i] = 0xAAAABBBB;
        out[i + 1] = 0xCCCCDDDD;
        out[i] = 0xAAAAAAAA |
                ((in[input_obj] >> 15) & 1) << 30 |
                ((in[input_obj] >> 14) & 1) << 28 |
                ((in[input_obj] >> 13) & 1) << 26 |
                ((in[input_obj] >> 12) & 1) << 24 |
                ((in[input_obj] >> 11) & 1) << 22 |
                ((in[input_obj] >> 10) & 1) << 20 |
                ((in[input_obj] >> 9) & 1) << 18 |
                ((in[input_obj] >> 8) & 1) << 16 |
                ((in[input_obj] >> 7) & 1) << 14 |
                ((in[input_obj] >> 6) & 1) << 12 |
                ((in[input_obj] >> 5) & 1) << 10 |
                ((in[input_obj] >> 4) & 1) << 8 |
                ((in[input_obj] >> 3) & 1) << 6 |
                ((in[input_obj] >> 2) & 1) << 4 |
                ((in[input_obj] >> 1) & 1) << 2 |
                ((in[input_obj] >> 0) & 1) << 0;
        out[i + 1] = 0xAAAAAAAA |
                ((in[input_obj] >> 31) & 1) << 30 |
                ((in[input_obj] >> 30) & 1) << 28 |
                ((in[input_obj] >> 29) & 1) << 26 |
                ((in[input_obj] >> 28) & 1) << 24 |
                ((in[input_obj] >> 27) & 1) << 22 |
                ((in[input_obj] >> 26) & 1) << 20 |
                ((in[input_obj] >> 25) & 1) << 18 |
                ((in[input_obj] >> 24) & 1) << 16 |
                ((in[input_obj] >> 23) & 1) << 14 |
                ((in[input_obj] >> 22) & 1) << 12 |
                ((in[input_obj] >> 21) & 1) << 10 |
                ((in[input_obj] >> 20) & 1) << 8 |
                ((in[input_obj] >> 19) & 1) << 6 |
                ((in[input_obj] >> 18) & 1) << 4 |
                ((in[input_obj] >> 17) & 1) << 2 |
                ((in[input_obj] >> 16) & 1) << 0;
    }
    out[num_out_obj - 1] ^= (1 << 31);
    return out;
}
void typec_operation_test() {
    uint32_t test_data[] = { 0x5, 0x55555000, 0x55555555, 0xC718C555, 0xFD5FBD24, 0x6BAAA4DE };
    uint num_u32 = test_data[0];
    uint32_t *test_ptr = &test_data[1];
    //printf("%u | %X %X", num_u32, test_ptr[0], test_ptr[1]);
    uint32_t *test_ptr2 = typec_tx_convert(test_ptr, num_u32);
    for(uint i = 0; i < num_u32 * 2; i++) {
        printf("%X ", test_ptr2[i]);
    }
}
