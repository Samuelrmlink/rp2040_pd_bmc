#include "main_i.h"

#define MAX_BYTES_IN_PDFRAME_STRUCT 56
// Ensures that the raw bits are aligned before interpretation
//      * Does NOT modify the input data - but DOES change the input_offset variable if needed
//      Returns true if preamble is found & in alignment
bool typec_4b5b_preamble_align(uint *input_offset, uint32_t in) {
    uint offset = 0;
    bool aligned;
    switch(in) {
        case(0x55555555):
            offset = 1;
            // Falls through..
        case(0xAAAAAAAA):
            // Preamble has bee found
            aligned = true;
            break;
        default:
            aligned= false;
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
bool typec_4b5b_find_ordered_set(uint *input_offset, uint *after_scrap_offset, uint *scrap_bits, uint *output_offset, uint32_t in) {
    bool found_ordset = false;
    while(*input_offset <= 27) {
        if( ((*scrap_bits | (in >> *input_offset) << *after_scrap_offset) & 0x1F) == symKcodeS1 ||  // K-Code S1 (starting symbol for ordsetSop*) ---or---
            ((*scrap_bits | (in >> *input_offset) << *after_scrap_offset) & 0x1F) == symKcodeS1) {  // K-Code R1 (starting symbol for ordsetHardReset, ordsetCableReset)
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
void typec_4b5b_decode(pd_frame *pdf, uint32_t raw_data) {
    static uint input_offset;       // [ >> shift direction ]
    static uint after_scrap_offset; // [ << shift direction ]
    static uint scrap_bits;
    static uint output_offset;
    static bool preamble_aligned;
    assert(output_offset < MAX_BYTES_IN_PDFRAME_STRUCT);
    // Ensure preamble alignment
    if(!preamble_aligned) {
        if(typec_4b5b_preamble_align(&input_offset, raw_data)) {
            preamble_aligned = true;
        }
    }
    // Locate first symbol of the Ordered Set
    if(typec_4b5b_find_ordered_set(&input_offset, &after_scrap_offset, &scrap_bits, &output_offset, raw_data)) {
        // Ordered Set has been found
        output_offset = 4;
        printf("%X ioffset: %u\n", raw_data, input_offset);
    }

    input_offset = 0;

    // TODO: Reset all variables upon error, EOP, etc...
}
