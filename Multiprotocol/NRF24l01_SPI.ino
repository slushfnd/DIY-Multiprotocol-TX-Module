/*
 This project is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Multiprotocol is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Multiprotocol.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifdef NRF24L01_INSTALLED
#include "iface_nrf24l01.h"
#include "iface_xn297.h"

//---------------------------
// NRF24L01+ SPI Specific Functions
//---------------------------

uint8_t rf_setup;

void NRF24L01_Initialize()
{
    rf_setup = 0x09;
	prev_power = 0x00;	// Make sure prev_power is inline with current power

	//Load most likely default NRF config
	NRF24L01_FlushTx();
	NRF24L01_FlushRx();
	NRF24L01_WriteReg(NRF24L01_01_EN_AA,		0x00);	// No Auto Acknowldgement on all data pipes
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR,	0x01); 	// Enable data pipe 0 only
	NRF24L01_WriteReg(NRF24L01_03_SETUP_AW,		0x03);	// 5 bytes rx/tx address
	NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR,	0x00); 	// no retransmits
	NRF24L01_SetBitrate(NRF24L01_BR_1M);             	// 1Mbps
	NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);       	// Disable dynamic payload length on all pipes
	NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x01);     	// Set feature bits off and enable the command NRF24L01_B0_TX_PYLD_NOACK
	NRF24L01_SetPower();
	NRF24L01_SetTxRxMode(TX_EN);						// Clear data ready, data sent, retransmit and enable CRC 16bits, ready for TX
}  

void NRF24L01_WriteReg(uint8_t reg, uint8_t data)
{
	NRF_CSN_off;
	SPI_Write(W_REGISTER | (REGISTER_MASK & reg));
	SPI_Write(data);
	NRF_CSN_on;
}

void NRF24L01_WriteRegisterMulti(uint8_t reg, uint8_t * data, uint8_t length)
{
	NRF_CSN_off;

	SPI_Write(W_REGISTER | ( REGISTER_MASK & reg));
	for (uint8_t i = 0; i < length; i++)
		SPI_Write(data[i]);
	NRF_CSN_on;
}

void NRF24L01_WritePayload(uint8_t * data, uint8_t length)
{
	NRF_CSN_off;
	SPI_Write(W_TX_PAYLOAD);
	for (uint8_t i = 0; i < length; i++)
		SPI_Write(data[i]);
	NRF_CSN_on;
}

uint8_t NRF24L01_ReadReg(uint8_t reg)
{
	NRF_CSN_off;
	SPI_Write(R_REGISTER | (REGISTER_MASK & reg));
	uint8_t data = SPI_Read();
	NRF_CSN_on;
	return data;
}

/*static void NRF24L01_ReadRegisterMulti(uint8_t reg, uint8_t * data, uint8_t length)
{
	NRF_CSN_off;
	SPI_Write(R_REGISTER | (REGISTER_MASK & reg));
	for(uint8_t i = 0; i < length; i++)
		data[i] = SPI_Read();
	NRF_CSN_on;
}
*/

static void NRF24L01_ReadPayload(uint8_t * data, uint8_t length)
{
	NRF_CSN_off;
	SPI_Write(R_RX_PAYLOAD);
	for(uint8_t i = 0; i < length; i++)
		data[i] = SPI_Read();
	NRF_CSN_on; 
}

static void  NRF24L01_Strobe(uint8_t state)
{
	NRF_CSN_off;
	SPI_Write(state);
	NRF_CSN_on;
}

void NRF24L01_FlushTx()
{
	NRF24L01_Strobe(FLUSH_TX);
}

void NRF24L01_FlushRx()
{
	NRF24L01_Strobe(FLUSH_RX);
}

static uint8_t __attribute__((unused)) NRF24L01_GetStatus()
{
	return SPI_Read();
}

static uint8_t NRF24L01_GetDynamicPayloadSize()
{
	NRF_CSN_off;
    SPI_Write(R_RX_PL_WID);
    uint8_t len = SPI_Read();
	NRF_CSN_on; 
    return len;
}

/*void NRF24L01_Activate(uint8_t code)
{
	NRF_CSN_off;
	SPI_Write(ACTIVATE);
	SPI_Write(code);
	NRF_CSN_on;
}*/

void NRF24L01_SetBitrate(uint8_t bitrate)
{
    // Note that bitrate 250kbps (and bit RF_DR_LOW) is valid only
    // for nRF24L01+. There is no way to programmatically tell it from
    // older version, nRF24L01, but the older is practically phased out
    // by Nordic, so we assume that we deal with modern version.

    // Bit 0 goes to RF_DR_HIGH, bit 1 - to RF_DR_LOW
    rf_setup = (rf_setup & 0xD7) | ((bitrate & 0x02) << 4) | ((bitrate & 0x01) << 3);
    prev_power=(rf_setup>>1)&0x03;	// Make sure prev_power is inline with current power
	NRF24L01_WriteReg(NRF24L01_06_RF_SETUP, rf_setup);
}

/*
	static void NRF24L01_SetPower_Value(uint8_t power)
{
    uint8_t nrf_power = 0;
    switch(power) {
        case TXPOWER_100uW: nrf_power = 0; break;
        case TXPOWER_300uW: nrf_power = 0; break;
        case TXPOWER_1mW:   nrf_power = 0; break;
        case TXPOWER_3mW:   nrf_power = 1; break;
        case TXPOWER_10mW:  nrf_power = 1; break;
        case TXPOWER_30mW:  nrf_power = 2; break;
        case TXPOWER_100mW: nrf_power = 3; break;
        case TXPOWER_150mW: nrf_power = 3; break;
        default:            nrf_power = 0; break;
    };
    // Power is in range 0..3 for nRF24L01
    rf_setup = (rf_setup & 0xF9) | ((nrf_power & 0x03) << 1);
    NRF24L01_WriteReg(NRF24L01_06_RF_SETUP, rf_setup);
}
*/
void NRF24L01_SetPower()
{
	uint8_t power=NRF_BIND_POWER;
	if(IS_BIND_DONE)
		#ifdef NRF24L01_ENABLE_LOW_POWER
			power=IS_POWER_FLAG_on?NRF_HIGH_POWER:NRF_LOW_POWER;
		#else
			power=NRF_HIGH_POWER;
		#endif
	if(IS_RANGE_FLAG_on)
		power=NRF_POWER_0;
	if(prev_power != power)
	{
		rf_setup = (rf_setup & 0xF8) | (power << 1);
		if(power==3)
			rf_setup |= 0x01;	// Si24r01 full power, unused bit for NRF
		NRF24L01_WriteReg(NRF24L01_06_RF_SETUP, rf_setup);
		prev_power=power;
	}
}

void NRF24L01_SetTxRxMode(enum TXRX_State mode)
{
	NRF24L01_WriteReg(NRF24L01_07_STATUS, (1 << NRF24L01_07_RX_DR)    //reset the flag(s)
										| (1 << NRF24L01_07_TX_DS)
										| (1 << NRF24L01_07_MAX_RT));
	if(mode == TX_EN) {
		NRF_CE_off;
		NRF24L01_WriteReg(NRF24L01_00_CONFIG, (1 << NRF24L01_00_EN_CRC)   // switch to TX mode
											| (1 << NRF24L01_00_CRCO)
											| (1 << NRF24L01_00_PWR_UP));
		delayMicroseconds(130);
		NRF_CE_on;
	}
	else
		if (mode == RX_EN)
		{
			NRF_CE_off;
			NRF24L01_WriteReg(NRF24L01_00_CONFIG, (1 << NRF24L01_00_EN_CRC)   // switch to RX mode
												| (1 << NRF24L01_00_CRCO)
												| (1 << NRF24L01_00_PWR_UP)
												| (1 << NRF24L01_00_PRIM_RX));
			delayMicroseconds(130);
			NRF_CE_on;
		}
		else
		{
			NRF24L01_WriteReg(NRF24L01_00_CONFIG, (1 << NRF24L01_00_EN_CRC)); //PowerDown
			NRF_CE_off;
		}
}

void NRF24L01_Reset()
{
	NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    NRF24L01_Strobe(0xff);			// NOP
    NRF24L01_ReadReg(NRF24L01_07_STATUS);
    NRF24L01_SetTxRxMode(TXRX_OFF);
	delayMicroseconds(100);
}

uint8_t NRF24L01_packet_ack()
{
    switch (NRF24L01_ReadReg(NRF24L01_07_STATUS) & (_BV(NRF24L01_07_TX_DS) | _BV(NRF24L01_07_MAX_RT)))
	{
		case _BV(NRF24L01_07_TX_DS):
			return PKT_ACKED;
		case _BV(NRF24L01_07_MAX_RT):
			return PKT_TIMEOUT;
    }
	return PKT_PENDING;
}

///////////////
// LT8900 emulation layer
uint8_t LT8900_buffer[64];
uint8_t LT8900_buffer_start;
uint16_t LT8900_buffer_overhead_bits;
uint8_t LT8900_addr[8];
uint8_t LT8900_addr_size;
uint8_t LT8900_Preamble_Len;
uint8_t LT8900_Tailer_Len;
uint8_t LT8900_CRC_Initial_Data;
uint8_t LT8900_Flags;
#define LT8900_CRC_ON 6
#define LT8900_SCRAMBLE_ON 5
#define LT8900_PACKET_LENGTH_EN 4
#define LT8900_DATA_PACKET_TYPE_1 3
#define LT8900_DATA_PACKET_TYPE_0 2
#define LT8900_FEC_TYPE_1 1
#define LT8900_FEC_TYPE_0 0

void LT8900_Config(uint8_t preamble_len, uint8_t trailer_len, uint8_t flags, uint8_t crc_init)
{
	//Preamble 1 to 8 bytes
	LT8900_Preamble_Len=preamble_len;
	//Trailer 4 to 18 bits
	LT8900_Tailer_Len=trailer_len;
	//Flags
	// CRC_ON: 1 on, 0 off
	// SCRAMBLE_ON: 1 on, 0 off
	// PACKET_LENGTH_EN: 1 1st byte of payload is payload size
	// DATA_PACKET_TYPE: 00 NRZ, 01 Manchester, 10 8bit/10bit line code, 11 interleave data type
	// FEC_TYPE: 00 No FEC, 01 FEC13, 10 FEC23, 11 reserved
	LT8900_Flags=flags;
	//CRC init constant
	LT8900_CRC_Initial_Data=crc_init;
}

void LT8900_SetChannel(uint8_t channel)
{
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, channel +2);	//NRF24L01 is 2400+channel but LT8900 is 2402+channel
}

void LT8900_SetTxRxMode(enum TXRX_State mode)
{
	if(mode == TX_EN)
	{
		//Switch to TX
		NRF24L01_SetTxRxMode(TXRX_OFF);
		NRF24L01_SetTxRxMode(TX_EN);
		//Disable CRC
		NRF24L01_WriteReg(NRF24L01_00_CONFIG, (1 << NRF24L01_00_PWR_UP));
	}
	else
		if (mode == RX_EN)
		{
			NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);		// Enable data pipe 0 only
			NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, 32);
			//Switch to RX
			NRF24L01_SetTxRxMode(TXRX_OFF);
			NRF24L01_FlushRx();
			NRF24L01_SetTxRxMode(RX_EN);
			// Disable CRC
			NRF24L01_WriteReg(NRF24L01_00_CONFIG, (1 << NRF24L01_00_PWR_UP) | (1 << NRF24L01_00_PRIM_RX) );
		}
		else
			NRF24L01_SetTxRxMode(TXRX_OFF);
}

void LT8900_BuildOverhead()
{
	uint8_t pos;

	//Build overhead
	//preamble
	memset(LT8900_buffer,LT8900_addr[0]&0x01?0xAA:0x55,LT8900_Preamble_Len-1);
	pos=LT8900_Preamble_Len-1;
	//address
	for(uint8_t i=0;i<LT8900_addr_size;i++)
	{
		LT8900_buffer[pos]=bit_reverse(LT8900_addr[i]);
		pos++;
	}
	//trailer
	memset(LT8900_buffer+pos,(LT8900_buffer[pos-1]&0x01)==0?0xAA:0x55,3);
	LT8900_buffer_overhead_bits=pos*8+LT8900_Tailer_Len;
	//nrf address length max is 5
	pos+=LT8900_Tailer_Len/8;
	LT8900_buffer_start=pos>5?5:pos;
}

void LT8900_SetAddress(uint8_t *address,uint8_t addr_size)
{
	uint8_t addr[5];
	
	//Address size (SyncWord) 2 to 8 bytes, 16/32/48/64 bits
	LT8900_addr_size=addr_size;
	for (uint8_t i = 0; i < addr_size; i++)
		LT8900_addr[i] = address[addr_size-1-i];

	//Build overhead
	LT8900_BuildOverhead();

	//Set NRF RX&TX address based on overhead content
	NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, LT8900_buffer_start-2);
	for(uint8_t i=0;i<LT8900_buffer_start;i++)	// reverse bytes order
		addr[i]=LT8900_buffer[LT8900_buffer_start-i-1];
	NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0,	addr,LT8900_buffer_start);
	NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR,	addr,LT8900_buffer_start);
}

uint8_t LT8900_ReadPayload(uint8_t* msg, uint8_t len)
{
	uint8_t i,pos=0,shift,end,buffer[32];
	unsigned int a;
	crc=LT8900_CRC_Initial_Data;
	pos=LT8900_buffer_overhead_bits/8-LT8900_buffer_start;
	end=pos+len+(LT8900_Flags&_BV(LT8900_PACKET_LENGTH_EN)?1:0)+(LT8900_Flags&_BV(LT8900_CRC_ON)?2:0);
	//Read payload
	NRF24L01_ReadPayload(buffer,end+1);
	//Check address + trail
	for(i=0;i<pos;i++)
		if(LT8900_buffer[LT8900_buffer_start+i]!=buffer[i])
			return 0; // wrong address...
	//Shift buffer to remove trail bits
	shift=LT8900_buffer_overhead_bits&0x7;
	for(i=pos;i<end;i++)
	{
		a=(buffer[i]<<8)+buffer[i+1];
		a<<=shift;
		buffer[i]=(a>>8)&0xFF;
	}
	//Check len
	if(LT8900_Flags&_BV(LT8900_PACKET_LENGTH_EN))
	{
		crc16_update(buffer[pos],8);
		if(bit_reverse(len)!=buffer[pos++])
			return 0; // wrong len...
	}
	//Decode message 
	for(i=0;i<len;i++)
	{
		crc16_update(buffer[pos],8);
		msg[i]=bit_reverse(buffer[pos++]);
	}
	//Check CRC
	if(LT8900_Flags&_BV(LT8900_CRC_ON))
	{
		if(buffer[pos++]!=((crc>>8)&0xFF)) return 0;	// wrong CRC...
		if(buffer[pos]!=(crc&0xFF)) return 0;			// wrong CRC...
	}
	//Everything ok
	return 1;
}

void LT8900_WritePayload(uint8_t* msg, uint8_t len)
{
	unsigned int a,mask;
	uint8_t i, pos=0,tmp, buffer[64], pos_final,shift;
	crc=LT8900_CRC_Initial_Data;
	//Add packet len
	if(LT8900_Flags&_BV(LT8900_PACKET_LENGTH_EN))
	{
		tmp=bit_reverse(len);
		buffer[pos++]=tmp;
		crc16_update(tmp,8);
	}
	//Add payload
	for(i=0;i<len;i++)
	{
		tmp=bit_reverse(msg[i]);
		buffer[pos++]=tmp;
		crc16_update(tmp,8);
	}
	//Add CRC
	if(LT8900_Flags&_BV(LT8900_CRC_ON))
	{
		buffer[pos++]=crc>>8;
		buffer[pos++]=crc;
	}
	//Shift everything to fit behind the trailer (4 to 18 bits)
	shift=LT8900_buffer_overhead_bits&0x7;
	pos_final=LT8900_buffer_overhead_bits/8;
	mask=~(0xFF<<(8-shift));
	LT8900_buffer[pos_final+pos]=0xFF;
	for(i=pos-1;i!=0xFF;i--)
	{
		a=buffer[i]<<(8-shift);
		LT8900_buffer[pos_final+i]=(LT8900_buffer[pos_final+i]&mask>>8)|a>>8;
		LT8900_buffer[pos_final+i+1]=(LT8900_buffer[pos_final+i+1]&mask)|a;
	}
	if(shift)
		pos++;
	//Send everything
	NRF24L01_WritePayload(LT8900_buffer+LT8900_buffer_start,pos_final+pos-LT8900_buffer_start);
}
// End of LT8900 emulation
#endif