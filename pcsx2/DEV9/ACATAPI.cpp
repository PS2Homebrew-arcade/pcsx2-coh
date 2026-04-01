#include "ACATAPI.h"
#include "common/Console.h"

#include "ACMACROS.h"

void ACATAPI::handle_cmd(atapi_packet_t P) {
    switch (P.pkt.opcode) {
    case ATAPICMD::READ_10: {
        u32 transf_lba = U32FU16(P.pkt.lba_high, P.pkt.lba_lo);
        Console.Warning("ACATAPI:cmd:READ_10: tlen:%X, lba:%X", P.pkt.transf_len, transf_lba);
        }
        break;
    
    default:
        Console.Error("ACATAPI: unhandled cmd %02X !", P.pkt.opcode);
        break;
    }
}