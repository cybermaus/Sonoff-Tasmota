/**
 * esp-knx-ip library for KNX/IP communication on an ESP8266
 * Author: Nico Weichbrodt <envy>
 * License: MIT
 */

#include "esp-knx-ip.h"

/**
 * Send functions
 */

void ESPKNXIP::send(address_t const &receiver, knx_command_type_t ct, uint8_t data_len, uint8_t *data)
{
	if (receiver.value == 0)
		return;

#if SEND_CHECKSUM
	uint32_t len = 6 + 2 + 8 + data_len + 1; // knx_pkt + cemi_msg + cemi_service + data + checksum
#else
	uint32_t len = 6 + 2 + 8 + data_len; // knx_pkt + cemi_msg + cemi_service + data
#endif
	DEBUG_PRINT(F("Creating packet with len "));
	DEBUG_PRINTLN(len)
	uint8_t buf[len];
	knx_ip_pkt_t *knx_pkt = (knx_ip_pkt_t *)buf;
	knx_pkt->header_len = 0x06;
	knx_pkt->protocol_version = 0x10;
	knx_pkt->service_type = __ntohs(KNX_ST_ROUTING_INDICATION);
	knx_pkt->total_len.len = __ntohs(len);
	cemi_msg_t *cemi_msg = (cemi_msg_t *)knx_pkt->pkt_data;
	cemi_msg->message_code = KNX_MT_L_DATA_IND;
	cemi_msg->additional_info_len = 0;
	cemi_service_t *cemi_data = &cemi_msg->data.service_information;
	cemi_data->control_1.bits.confirm = 0;
//cemi_data->control_1.bits.ack = 1;
	cemi_data->control_1.bits.ack = 0; // ask for ACK? 0-no 1-yes
	cemi_data->control_1.bits.priority = B11;
	cemi_data->control_1.bits.system_broadcast = 0x01;
	cemi_data->control_1.bits.repeat = 0x01; // 0 = repeated telegram, 1 = not repeated telegram
	cemi_data->control_1.bits.reserved = 0;
	cemi_data->control_1.bits.frame_type = 0x01; // 1 = standard, up to 23 octets, 0=extended up to 263 octets
	cemi_data->control_2.bits.extended_frame_format = 0x00;
	cemi_data->control_2.bits.hop_count = 0x06;
	cemi_data->control_2.bits.dest_addr_type = 0x01;
	cemi_data->source = physaddr;
	cemi_data->destination = receiver;
	//cemi_data->destination.bytes.high = (area << 3) | line;
	//cemi_data->destination.bytes.low = member;
	cemi_data->data_len = data_len;
  cemi_data->pci.apci = (ct & 0x0C) >> 2;
//cemi_data->pci.apci = KNX_COT_NCD_ACK;
	cemi_data->pci.tpci_seq_number = 0x00;
	cemi_data->pci.tpci_comm_type = KNX_COT_UDP; // Type of communication: DATA PACKAGE or CONTROL DATA
//cemi_data->pci.tpci_comm_type = KNX_COT_NCD; // Type of communication: DATA PACKAGE or CONTROL DATA
	memcpy(cemi_data->data, data, data_len);
//cemi_data->data[0] = (cemi_data->data[0] & 0x3F) | ((KNX_COT_NCD_ACK & 0x03) << 6);
	cemi_data->data[0] = (cemi_data->data[0] & 0x3F) | ((ct & 0x03) << 6);

#if SEND_CHECKSUM
	// Calculate checksum, which is just XOR of all bytes
	uint8_t cs = buf[0] ^ buf[1];
	for (uint32_t i = 2; i < len - 1; ++i)
	{
		cs ^= buf[i];
	}
	buf[len - 1] = cs;
#endif

#ifdef ESP_KNX_DEBUG
	DEBUG_PRINT(F("Sending packet:"));
	for (int i = 0; i < len; ++i)
	{
		DEBUG_PRINT(F(" 0x"));
		DEBUG_PRINT(buf[i], 16);
	}
	DEBUG_PRINTLN(F(""));
#endif

    // 2020-10-09 Maurits van Dueren : No reflected broadcast 
	// If the to be broadcasted telegram is an exact copy of one we just received, 
	// then do not send it. This is to avoid network storms by applying telegram to the switch that is
	// set to send out its state again.
	// Not sure if this is part of the KNX specification, but I find it extremely usefull for puttin several
	// switches in the same broadcast group, without worrying about which is initating an event.
    bool knx_repeat = true; // assume it is reflected (repeated)
    // Memory of previous KNX telegram
    cemi_service_t *cemi_data_prev = (cemi_service_t *)cemi_data_prev_buf;
    if (knx_repeat && (millis()-cemi_data_prev_stale)>CEMI_DATA_STALE_DURATION) { 
      DEBUG_PRINTLN(F("Dropping stale buffer."));
      knx_repeat = false; 
    }
    if (knx_repeat && (cemi_data->data_len == cemi_data_prev->data_len)) { 
      size_t cmplen = sizeof cemi_data->destination +
                      sizeof cemi_data->data_len +
                      sizeof cemi_data->pci + 
                      cemi_data->data_len;
      knx_repeat = (0==memcmp(&cemi_data->destination, &cemi_data_prev->source, cmplen));
    } else {
      knx_repeat=false;
    }
    if (!knx_repeat) { 
 
      // 2020-10-09 Maurits van Dueren : Repeat KNX telegrams
      // Rudimentary implementation of the KNX protocol repeated telegram
  	  // However, rather then waiting for a time out on ACK, we just send 3 extra copies unsolicited.
	  // The receiver implementation filters them out again.
	  // I put this here, rather then the Tasmota KNX driver, as here it is a single block for all data types
	  // NOTE: Official KNX protocol only sends repeat after no acknowledge is received, but 
	  // presumeably, it knows how to deal with missing acknowledgements, and thus unsolicited repeats as well.
	  // Ideally UDP repeats should not be immediately after each other, so the official 'wait for ACK' would be better
	  for (byte i=1+3; i>0; i--) {
  	    udp.beginPacketMulticast(MULTICAST_IP, MULTICAST_PORT, WiFi.localIP());
	    udp.write(buf, len);
 	    udp.endPacket();
		if (i==1+3) {
          cemi_data->control_1.bits.repeat = 0x00; // 0 = repeated telegram, 1 = not repeated telegram
	      #if SEND_CHECKSUM
	      buf[len - 1] ^= 0x01; // fix checksum because of above changed repeat bit
          #endif
		}
	  }
	} else {
      DEBUG_PRINTLN(F("Dropping reflected telegram."));
	}
}

void ESPKNXIP::send_1bit(address_t const &receiver, knx_command_type_t ct, uint8_t bit)
{
	uint8_t buf[] = {(uint8_t)(bit & 0b00000001)};
	send(receiver, ct, 1, buf);
}

void ESPKNXIP::send_2bit(address_t const &receiver, knx_command_type_t ct, uint8_t twobit)
{
	uint8_t buf[] = {(uint8_t)(twobit & 0b00000011)};
	send(receiver, ct, 1, buf);
}

void ESPKNXIP::send_4bit(address_t const &receiver, knx_command_type_t ct, uint8_t fourbit)
{
	uint8_t buf[] = {(uint8_t)(fourbit & 0b00001111)};
	send(receiver, ct, 1, buf);
}

void ESPKNXIP::send_1byte_int(address_t const &receiver, knx_command_type_t ct, int8_t val)
{
	uint8_t buf[] = {0x00, (uint8_t)val};
	send(receiver, ct, 2, buf);
}

void ESPKNXIP::send_1byte_uint(address_t const &receiver, knx_command_type_t ct, uint8_t val)
{
	uint8_t buf[] = {0x00, val};
	send(receiver, ct, 2, buf);
}

void ESPKNXIP::send_2byte_int(address_t const &receiver, knx_command_type_t ct, int16_t val)
{
	uint8_t buf[] = {0x00, (uint8_t)(val >> 8), (uint8_t)(val & 0x00FF)};
	send(receiver, ct, 3, buf);
}

void ESPKNXIP::send_2byte_uint(address_t const &receiver, knx_command_type_t ct, uint16_t val)
{
	uint8_t buf[] = {0x00, (uint8_t)(val >> 8), (uint8_t)(val & 0x00FF)};
	send(receiver, ct, 3, buf);
}

void ESPKNXIP::send_2byte_float(address_t const &receiver, knx_command_type_t ct, float val)
{
	float v = val * 100.0f;
	int e = 0;
	for (; v < -2048.0f; v /= 2)
	++e;
	for (; v > 2047.0f; v /= 2)
	++e;
	long m = (long)round(v) & 0x7FF;
	short msb = (short) (e << 3 | m >> 8);
	if (val < 0.0f)
	msb |= 0x80;
	uint8_t buf[] = {0x00, (uint8_t)msb, (uint8_t)m};
	send(receiver, ct, 3, buf);
}

void ESPKNXIP::send_3byte_time(address_t const &receiver, knx_command_type_t ct, uint8_t weekday, uint8_t hours, uint8_t minutes, uint8_t seconds)
{
	weekday <<= 5;
	uint8_t buf[] = {0x00, (uint8_t)(((weekday << 5) & 0xE0) | (hours & 0x1F)), (uint8_t)(minutes & 0x3F), (uint8_t)(seconds & 0x3F)};
	send(receiver, ct, 4, buf);
}

void ESPKNXIP::send_3byte_date(address_t const &receiver, knx_command_type_t ct, uint8_t day, uint8_t month, uint8_t year)
{
	uint8_t buf[] = {0x00, (uint8_t)(day & 0x1F), (uint8_t)(month & 0x0F), year};
	send(receiver, ct, 4, buf);
}

void ESPKNXIP::send_3byte_color(address_t const &receiver, knx_command_type_t ct, uint8_t red, uint8_t green, uint8_t blue)
{
	uint8_t buf[] = {0x00, red, green, blue};
	send(receiver, ct, 4, buf);
}

void ESPKNXIP::send_4byte_int(address_t const &receiver, knx_command_type_t ct, int32_t val)
{
	uint8_t buf[] = {0x00,
	                 (uint8_t)((val & 0xFF000000) >> 24),
	                 (uint8_t)((val & 0x00FF0000) >> 16),
	                 (uint8_t)((val & 0x0000FF00) >> 8),
	                 (uint8_t)((val & 0x000000FF) >> 0)};
	send(receiver, ct, 5, buf);
}

void ESPKNXIP::send_4byte_uint(address_t const &receiver, knx_command_type_t ct, uint32_t val)
{
	uint8_t buf[] = {0x00,
	                 (uint8_t)((val & 0xFF000000) >> 24),
	                 (uint8_t)((val & 0x00FF0000) >> 16),
	                 (uint8_t)((val & 0x0000FF00) >> 8),
	                 (uint8_t)((val & 0x000000FF) >> 0)};
	send(receiver, ct, 5, buf);
}

void ESPKNXIP::send_4byte_float(address_t const &receiver, knx_command_type_t ct, float val)
{
	uint8_t buf[] = {0x00, ((uint8_t *)&val)[3], ((uint8_t *)&val)[2], ((uint8_t *)&val)[1], ((uint8_t *)&val)[0]};
	send(receiver, ct, 5, buf);
}

void ESPKNXIP::send_14byte_string(address_t const &receiver, knx_command_type_t ct, const char *val)
{
	// DPT16 strings are always 14 bytes long, however the data array is one larger due to the telegram structure.
	// The first byte needs to be zero, string start after that.
	uint8_t buf[15] = {0x00};
	int len = strlen(val);
	if (len > 14)
	{
		len = 14;
	}
	memcpy(buf+1, val, len);
	send(receiver, ct, 15, buf);
}
