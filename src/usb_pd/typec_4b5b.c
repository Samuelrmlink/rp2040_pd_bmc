#include "main_i.h"


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
       // Check whether EOP symbol has been received
        if(decoded_4b == sym4bKcodeEop) {
            // End of panel symbol
            return true;
        }
        // Check whether we are past the maximum pd_frame size (this shouldn't happen)
        if(*output_offset >= MAX_BYTES_IN_PDFRAME_STRUCT) {
            // Overflow protection
            cli_log(ERROR_LOG, "OVERFLOW\n");
            pdf->timestamp_us = 0;
            return true;
        }
        assert(*output_offset < MAX_BYTES_IN_PDFRAME_STRUCT);
        // Write into the pd_frame struct
        pdf->raw_bytes[*output_offset] |= decoded_4b << (4 * (upper_symbol & 1u));
        // Increment counter & offset handling for the next run
        if(upper_symbol || decoded_4b & 0x10) {
            upper_symbol = false;
            *output_offset += 1;
        } else {
            upper_symbol = true;
        }
    }
    // Stash any leftover bits before returning
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
