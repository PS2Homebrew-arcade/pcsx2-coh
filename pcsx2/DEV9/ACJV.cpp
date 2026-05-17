#include "common/Console.h"
#include "ACJV.h"
#include <array>

enum ACJVCMD {
	UNKNOWN = -2, // unknown CMD, should fire up a warning for developer
	NONE = -1,  // Neutral state
	JVS_INIT0, // starts with 26 A3
	JVS_INIT1, // starts with 98 59
	JVS_JVS,   // starts with 6F 3E. this holds an actual JVS packet inside
};

bool ACJV::enabled = false;
#define JVS_PKTINDX_TO_ADDR(index) ((2 * (index & 0x3FFF)) + 0x12400000)
// u16 ACJV::JVBUF[ACJV_ADDR_CAP];

#define ANS(addr, what) case addr: return what
#define JVS_CHECK_ADDR_JVSPACKET(addr) ((addr & 0x00000F00) == 0x00000600) // when ACJV writes to 0x12406???.
void do_acjv_packet();

u16 RootAddr = 0x3E6F;
u16 Sent0D = 0, Sent0C = 0;

// R/W arrays are u8 because ACJV is processing (u8) arrays, but the MMIO is performed over (volatile u16*). leaving the higher byte empty
std::array<u8, ACJV_PACKETSIZE> rdbuf; // NAMCO_PCB ---> IOP
std::array<u8, ACJV_PACKETSIZE> wrbuf; // IOP --> NAMCO_PCB
inline u16* rdbuf_getu16() { // NAMCO_PCB ---> IOP
    return reinterpret_cast<u16*>(rdbuf.data());
}
inline const u16* wrbuf_getu16() { // IOP --> NAMCO_PCB
    return reinterpret_cast<const u16*>(wrbuf.data());
}

std::string BOARDS[] = {
	"namco ltd.;RAYS PCB;",
	"namco ltd.;FCA-1;Ver1.01;JPN,Multipurpose",
	"namco ltd.;FCB;Ver1.02;JPN,TouchPanel&Multipurpose",
	"namco ltd.;TSS-I/O;Ver2.11;GUN-EXTENTION",
	"namco ltd.;MIU-I/O;Ver2.05;JPN,GUN-EXTENTION",
};
enum BOARDID ACJV::CurrentBoardID = RAYS_PCB;

extern u16 ACJV::ButtonState[JVS_PLAYER_COUNT] = {0,0};

u16 dipsw = (DIPS::TESTMODE|DIPS::VIDEO_VOLTAGE|DIPS::MONITOR_SYNCFREQ|DIPS::VIDEO_SYNC_SPLIT);
u32 lastRead = 0x0;


u16 ACJV::Read16(u32 addr) {
    if (addr >= ACJV_RDBASE && addr < 0x124045FE) {
        int x = (addr - ACJV_RDBASE)/2;
		if (/*CurrentCMD == NONE &&*/ (x == 2 || x == 3 || x == 4)) return rdbuf.at(x)|1;// initial polling expects these addrs to not be zero
        return (u16)rdbuf.at(x);
    } else if ((addr == 0x124045FE)) {
		return (u16)rdbuf.at((addr - ACJV_RDBASE)/2);
	}
	return 0;
}

void ACJV::Write16(u32 addr, u16 val) {
    if (addr >= ACJV_WRBASE && addr < 0x12404BFE) { //0x124048FE
        u32 x = (addr - ACJV_WRBASE)/2;
        wrbuf[x] = val;
    } else if (addr == 0x12404BFE) {
       wrbuf[(addr -  ACJV_WRBASE)/2] = val;
		do_acjv_packet();
	}
}

#define assert(x) if (!(x)) Console.WriteLn("## ASSERT ## %s:%s:%d %s", __FILE__, __FUNCTION__, __LINE__, #x);

void do_jvs_packet(const u8* input, u8* output) {
	if (input[0] != JVS_SYNC) {
		//Console.Error("ACJV::%s: Error: input does not begin with E0, (%02X, %02X)", __FUNCTION__, input[0], input[1]);
		//return;
	}
	input++;
	u8 inDest = *input++;
	u8 inSize = *input++;
	u8 outSize = 0;
	u32 inWorkChecksum = inDest + inSize;
	inSize--;

	(*output++) = JVS_SYNC;
	(*output++) = 0x00; //Master ID?
	u8* dstSize = output++;
	(*dstSize) = 1;
	(*output++) = JVS_CMD_SUCCESS;
	while(inSize != 0) {
		u8 cmd = (*input++);
		// Console.WriteLn("jvs_cmd:0x%02X", cmd);
		inSize--;
		inWorkChecksum += cmd;
		switch(cmd) {
		case JVS::RESET: {
			assert(inSize != 0);
			u8 param = (*input++);
			assert(param == 0xD9);
			inSize--;
			inWorkChecksum += param;
		}
		break;
		case JVS::READ_ID_DATA: {
			(*output++) = JVS_CMD_SUCCESS;
			(*dstSize)++;
			const char* boardName = BOARDS[ACJV::CurrentBoardID].c_str();
			size_t length = strlen(boardName);

			for(int i = 0; i < length + 1; i++)
			{
				(*output++) = boardName[i];
				(*dstSize)++;
			}
		}
		break;
		case JVS::SET_NODE_ADDRESS: {
			assert(inSize != 0);
			u8 param = (*input++);
			inSize--;
			inWorkChecksum += param;
			(*output++) = JVS_CMD_SUCCESS;
			(*dstSize)++;
		}
		break;
		case JVS::GET_CMDFORMAT_REV: {
			(*output++) = JVS_CMD_SUCCESS;
			(*output++) = 0x13; //Revision 1.3
			(*dstSize) += 2;
		}
		break;
		case JVS::GET_REVISION: {
			(*output++) = JVS_CMD_SUCCESS;
			(*output++) = JVS_REVISION;
			(*dstSize) += 2;
		}
		break;
		case JVS::GET_SUPP_COMM_VER: {
			(*output++) = JVS_CMD_SUCCESS;
			(*output++) = JVS_VERSION;
			(*dstSize) += 2;
		}
		break;
		case JVS::GET_SLAVE_FEAT: {
			(*output++) = JVS_CMD_SUCCESS;

			(*output++) = 0x02;             //Coin input
			(*output++) = JVS_PLAYER_COUNT; //2 Coin slots
			(*output++) = 0x00;
			(*output++) = 0x00;

			(*output++) = 0x01;             //Switch input
			(*output++) = JVS_PLAYER_COUNT; //2 players
			(*output++) = 0x10;             //16 switches
			(*output++) = 0x00;
#if 0
			if(m_jvsMode == JVS_MODE::DRIVE)
			{
				(*output++) = 0x03;                  //Analog Input
				(*output++) = JVS_WHEEL_CHANNEL_MAX; //Channel Count (3 channels)
				(*output++) = 0x10;                  //Bits (16 bits)
				(*output++) = 0x00;
				(*dstSize) += 4;
			}
			else if(m_jvsMode == JVS_MODE::LIGHTGUN)
			{
				(*output++) = 0x06; //Screen Pos Input
				(*output++) = 0x10; //X pos bits
				(*output++) = 0x10; //Y pos bits
				(*output++) = 0x01; //channels

				//GPIO for recoil
				(*output++) = 0x12; //GPIO output
				(*output++) = 0x10; //slot(?) count
				(*output++) = 0x00;
				(*output++) = 0x00;

				//Time Crisis 4 reads from analog input to determine screen position
				(*output++) = 0x03; //Analog Input
				(*output++) = 0x02; //Channel Count (2 channels)
				(*output++) = 0x10; //Bits (16 bits)
				(*output++) = 0x00;

				(*dstSize) += 12;
			}
			else if(m_jvsMode == JVS_MODE::DRUM)
			{
				(*output++) = 0x03;                 //Analog Input
				(*output++) = JVS_DRUM_CHANNEL_MAX; //Channel Count (8 channels)
				(*output++) = 0x0A;                 //Bits (10 bits)
				(*output++) = 0x00;

				(*dstSize) += 4;
			}
			else if(m_jvsMode == JVS_MODE::TOUCH)
			{
				(*output++) = 0x06; //Screen Pos Input
				(*output++) = 0x10; //X pos bits
				(*output++) = 0x10; //Y pos bits
				(*output++) = 0x01; //channels

				(*dstSize) += 4;
			}
#endif
			(*output++) = 0x00; //End of features

			(*dstSize) += 10;
		}
		break;
		case JVS::CONVEY_ID_MAINBOARD:
		{
			while(1)
			{
				u8 value = (*input++);
				assert(inSize != 0);
				inSize--;
				inWorkChecksum += value;
				if(value == 0) break;
			}
		}
		break;
#if 0
		case JVS::READ_INP_SWITCH:
		{
			assert(inSize >= 2);
			u8 playerCount = (*input++);
			u8 byteCount = (*input++);
			assert(playerCount >= 1);
			assert(playerCount <= JVS_PLAYER_COUNT);
			assert(byteCount == 2);
			inWorkChecksum += playerCount;
			inWorkChecksum += byteCount;
			inSize -= 2;

			(*output++) = JVS_CMD_SUCCESS;

			m_counter++;
			if(m_testButtonState == 0 && m_jvsSystemButtonState == 0x03 && m_counter > 16)
			{
				m_testButtonState = 0x80;
				m_counter = 0;
			}
			else if(m_testButtonState == 0x80 && m_jvsSystemButtonState == 0x03 && m_counter > 16)
			{
				m_testButtonState = 0;
				m_counter = 0;
			}
			(*output++) = m_testButtonState;

			//(*output++) = (m_jvsSystemButtonState == 0x03) ? 0x80 : 0;  //Test
			(*output++) = static_cast<u8>(m_jvsButtonState[0]);      //Player 1
			(*output++) = static_cast<u8>(m_jvsButtonState[0] >> 8); //Player 1
			(*dstSize) += 4;

			if(playerCount == 2)
			{
				(*output++) = static_cast<u8>(m_jvsButtonState[1]);      //Player 2
				(*output++) = static_cast<u8>(m_jvsButtonState[1] >> 8); //Player 2
				(*dstSize) += 2;
			}
		}
#endif
		break;
#if 0
		case JVS::READ_INP_COIN:
		{
			assert(inSize != 0);
			u8 slotCount = (*input++);
			assert(slotCount >= 1);
			assert(slotCount <= 2);
			inWorkChecksum += slotCount;
			inSize--;
			u8 slot1Condition = COIN_NORMAL; // see enum COINCOND
			u8 slot2Condition = COIN_NORMAL; // see enum COINCOND

			(*output++) = JVS_CMD_SUCCESS;

			(*output++) = static_cast<u8>(((m_coin1 >> 8) & 0x3f) | (slot1Condition << 6)); //Coin 1 MSB + slot1condition
			(*output++) = static_cast<u8>(m_coin1 & 0x00ff);                                //Coin 1 LSB

			(*dstSize) += 3;

			if(slotCount == 2)
			{
				(*output++) = static_cast<u8>(((m_coin2 >> 8) & 0x3f) | (slot2Condition << 6)); //Coin 2 MSB + slot2condition
				(*output++) = static_cast<u8>(m_coin2 & 0x00ff);                                //Coin 2 LSB

				(*dstSize) += 2;
			}
		}
#endif
		break;
#if 0
		case JVS::OUTPUT_COIN_NUM: // actually never received this jvs cmd
		{
			assert(inSize != 3);
			u8 slotCount = (*input++);
			u8 amountMSB = (*input++);
			u8 amountLSB = (*input++);

			assert(slotCount >= 1);
			assert(slotCount <= 2);
			//inWorkChecksum += slotCount;
			inSize -= 3;

			if(slotCount == 1) m_coin1 += (amountMSB << 8) + amountLSB;
			if(slotCount == 2) m_coin2 += (amountMSB << 8) + amountLSB;

			(*output++) = JVS_CMD_SUCCESS;

			(*dstSize) += 1;
		}
#endif
		break;
#if 0
		case JVS__DECREASE_COIN_NUM: // actually never received this jvs cmd
		{
			assert(inSize != 3);
			u8 slotCount = (*input++);
			u8 amountMSB = (*input++);
			u8 amountLSB = (*input++);

			assert(slotCount >= 1);
			assert(slotCount <= 2);
			//inWorkChecksum += slotCount;
			inSize -= 3;

			if(slotCount == 1) m_coin1 -= (amountMSB << 8) + amountLSB;
			if(slotCount == 2) m_coin2 -= (amountMSB << 8) + amountLSB;

			(*output++) = JVS_CMD_SUCCESS;

			(*dstSize) += 1;
		}
#endif
		break;
		case JVS::READ_INP_ANALOG:
		{
			assert(inSize != 0);
			u8 channel = (*input++);
			inWorkChecksum += channel;
			inSize--;

			(*output++) = JVS_CMD_SUCCESS;

#if 0
			if(m_jvsMode == JVS_MODE::LIGHTGUN)
			{
				assert(channel == 2);
				(*output++) = static_cast<u8>(m_jvsScreenPosX >> 8); //Pos X MSB
				(*output++) = static_cast<u8>(m_jvsScreenPosX);      //Pos X LSB
				(*output++) = static_cast<u8>(m_jvsScreenPosY >> 8); //Pos Y MSB
				(*output++) = static_cast<u8>(m_jvsScreenPosY);      //Pos Y LSB
			}
			else if(m_jvsMode == JVS_MODE::DRUM)
			{
				assert(channel == JVS_DRUM_CHANNEL_MAX);
				for(int i = 0; i < JVS_DRUM_CHANNEL_MAX; i++)
				{
					(*output++) = static_cast<u8>(m_jvsDrumChannels[i] >> 8);
					(*output++) = static_cast<u8>(m_jvsDrumChannels[i]);
				}
			}
			else if(m_jvsMode == JVS_MODE::DRIVE)
			{
				for(int i = 0; i < JVS_WHEEL_CHANNEL_MAX; i++)
				{
					(*output++) = static_cast<u8>(m_jvsWheelChannels[i] >> 8);
					(*output++) = static_cast<u8>(m_jvsWheelChannels[i]);
				}
			}
			else
#endif
			{
				assert(false);
			}

			(*dstSize) += (2 * channel) + 1;
		}
		break;
#if 0
		case JVS::READ_INP_SCREENPOS:
		{
			assert(inSize != 0);
			u8 channel = (*input++);
			assert(channel == 1);
			inWorkChecksum += channel;
			inSize--;

			(*output++) = JVS_CMD_SUCCESS;

			(*output++) = static_cast<u8>(m_jvsScreenPosX >> 8); //Pos X MSB
			(*output++) = static_cast<u8>(m_jvsScreenPosX);      //Pos X LSB
			(*output++) = static_cast<u8>(m_jvsScreenPosY >> 8); //Pos Y MSB
			(*output++) = static_cast<u8>(m_jvsScreenPosY);      //Pos Y LSB

			(*dstSize) += 5;
		}
#endif
		break;
		// GPIO output
		case JVS::OUTPUT_GENERAL:
		{
			assert(inSize >= 2);

			u8 bytecount = (*input++);
			inWorkChecksum += bytecount;
			inSize--;

			for(int i = 1; i <= bytecount; i++)
			{
				u8 gpvalue = (*input++);
				inWorkChecksum += gpvalue;
				inSize--;

				if(i == 1)
				{
					// value1 0xC0 indicates P1 recoil triggered
					int p1Recoil = (gpvalue >= 0x50) ? 1 : 0;
					/*if(p1Recoil != m_p1RecoilLast)
					{
						// TODOx6
						//m_p1RecoilLast = p1Recoil;
#ifdef _WIN32
						m_mameCompatOutput->SendRecoil(p1Recoil);
#endif
					}*/
				}
			}

			(*output++) = JVS_CMD_SUCCESS;
			(*dstSize) += 1;
		}
		break;
		default:
			//Unknown command
			// Console.Error("ACJV::%s: unknown JVS CMD 0x%X", __FUNCTION__, cmd);
			break;
		}
	}
	u8 inChecksum = (*input);
	// if (inChecksum != (inWorkChecksum & 0xFF)) 
		// Console.Warning("ACJV::%s: checksum mismatch: %02X | %02X", __FUNCTION__, inChecksum, inWorkChecksum&0xFF);
}


// based on https://github.com/search?q=repo%3Ajpd002/Play-%20CSys246%3A%3AProcessJvsPacket&type=code by Jean-Philip Desjardins
void do_acjv_packet() {
	const u16* wr16 = wrbuf_getu16();
	u16* rd16 = rdbuf_getu16();
	rd16[0] = wr16[0];
	u16 RootPacketID = wr16[8];
	if(rd16[0] == 0x3E6F) {
		rd16[1]      = 0x208;        // unconfirmed: firmware version
		rd16[0x14]   = RootPacketID; // Xored with value at 0x10 in send packet, needs to be the same
		rd16[0x21]   = wr16[0x0D];
		rd16[0x30]  = dipsw;         // here the game polls the dip switch values?
		u16 PacketID = wr16[0x0C];
		if(PacketID != 0) {
			// Console.WriteLn("ACJV::JVS: Packet ID 0x%04X", PacketID);
			if(wrbuf[0x122] == JVS_SYNC) {
				Console.WriteLn("JVS::PACKET(0x122, 0x15A)");
				do_jvs_packet(&wrbuf[0x122], &rdbuf[0x15A]);
			} else {
				Console.WriteLn("JVS::PACKET(0x22, 0x5A)");
				do_jvs_packet(&wrbuf[0x22],  &rdbuf[0x5A]);
			}
			rd16[0x20] = PacketID;
		}
		rd16[0x15] = 0x5210;
		rd16[0x16] = 0x5210;
		rd16[0x17] = 0x5210;
	}
}