#pragma once

/**
 * @file ACATAPI.h
 * ATAPI related code for the ACATA interface
 */

#include "MemoryTypes.h"
#include "common/Pcsx2Types.h"
#include "common/Pcsx2Defs.h"

typedef union {
    u16 raw[6];
    struct {
        u8 opcode;
        u8 control0;
        u16 lba_high;
        u16 lba_lo;
        u8 resv;
        u16 transf_len;
        u8 control1;
        u8 control2;
        u8 unknown;
    }pkt;
}atapi_packet_t;

namespace ACATAPI
{
    void handle_cmd(atapi_packet_t P);
}


enum ATAPICMD {
    TEST_UNIT_READY = 0x00,
    REQUEST_SENSE = 0x03,
    FORMAT_UNIT = 0x04,
    INQUIRY = 0x12,
    START_STOP_UNIT_EJECT_DEVICE = 0x1B,
    PREVENT_ALLOW_MEDIUM_REMOVAL = 0x1E,
    READ_FORMAT_CAPACITIES = 0x23,
    READ_CAPACITY = 0x25,
    READ_10 = 0x28, // Standard read of logical blocks (512 bytes or 2048 bytes).
    WRITE_10 = 0x2A,
    SEEK_10 = 0x2B,
    WRITE_AND_VERIFY_10 = 0x2E,
    VERIFY_10 = 0x2F,
    SYNCHRONIZE_CACHE = 0x35,
    WRITE_BUFFER = 0x3B,
    READ_BUFFER = 0x3C,
    READ_TOC__PMA__ATIP = 0x43,
    GET_CONFIGURATION = 0x46,
    GET_EVENT_STATUS_NOTIFICATION = 0x4A,
    READ_DISC_INFORMATION = 0x51,
    READ_TRACK_INFORMATION = 0x52,
    RESERVE_TRACK = 0x53,
    SEND_OPC_INFORMATION = 0x54,
    MODE_SELECT = 0x55,
    REPAIR_TRACK = 0x58,
    MODE_SENSE = 0x5A,
    CLOSE_TRACK_SESSION = 0x5B,
    READ_BUFFER_CAPACITY = 0x5C,
    SEND_CUE_SHEET = 0x5D,
    REPORT_LUNS = 0xA0,
    BLANK = 0xA1,
    SECURITY_PROTOCOL_IN = 0xA2,
    SEND_KEY = 0xA3,
    REPORT_KEY = 0xA4,
    LOAD_UNLOAD_MEDIUM = 0xA6,
    SET_READ_AHEAD = 0xA7,
    READ_12 = 0xA8, // Extended read for larger capacities.
    WRITE_12 = 0xAA,
    //READ MEDIA SERIAL NUMBER / SERVICE ACTION IN (12) =	0xAB / 0x01,
    GET_PERFORMANCE = 0xAC,
    READ_DISC_STRUCTURE = 0xAD,
    SECURITY_PROTOCOL_OUT = 0xB5,
    SET_STREAMING = 0xB6,
    READ_CD_MSF = 0xB9,
    SET_CD_SPEED = 0xBB,
    MECHANISM_STATUS = 0xBD,
    READ_CD = 0xBE,
    SEND_DISC_STRUCTURE = 0xBF,
};
