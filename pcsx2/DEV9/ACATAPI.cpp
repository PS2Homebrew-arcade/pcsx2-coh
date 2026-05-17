#include "ACATAPI.h"
#include "ACATA.h"
#include "common/Console.h"

#include "ACMACROS.h"

void ACATAPI::handle_cmd(atapi_packet_t P) {
    u32 transf_lba = ATAPI_PKT_GETLBA(P);
    u32 nsec = ATAPI_PKT_GETLEN(P);
    switch (P.pkt.opcode) {
    case ATAPICMD::READ_10: {
        Console.Warning("ACATAPI:READ_10: tlen:%X, lba:%X", P.pkt.transf_len, transf_lba);
        ACATA::TH::nsector = nsec;
        ACATA::TH::LBA = transf_lba;
        if (ACATA_ISDMA) ACATA::TH::PendTrasnfType = ACATA::TH::PTRNSF::ATAPI;
        }
        break;
    
    default:
        Console.Error("ACATAPI: UNK_CMD %02X, lba:%08X, nsec:%04X", P.raw8[0], transf_lba, nsec);
        break;
    }
}

u16 ACATAPI::Read10(u32 lba, u16 tlen) {

}

void ACATAPI::Setup() {
    u32 SectorSizes[3] = {/*TODO: CHECK CD SECTOR SIZE*/0, ACATAPI::CONSTANTS::DVD_SECTORSIZE, 512};
    ACATA::TH::sectorsize = SectorSizes[ACATA::MediaType];
}