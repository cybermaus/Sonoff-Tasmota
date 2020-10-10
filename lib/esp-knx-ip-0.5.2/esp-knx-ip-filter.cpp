/**
 * esp-knx-ip library for KNX/IP communication on an ESP8266
 * Main Author KNX driver for ESP Nico Weichbrodt <envy>
 * This file: Maurits van Dueren <cybermaus>
 * License: MIT
 * 
 * 2020-10-09 Maurits van Dueren : Repeat filter and No Reflected broadcast
 * Rudimentary implementation/emulation of the KNX telegram repeat: 
 * https://support.knx.org/hc/en-us/articles/115003188269-LL-acknowledgement
 * 
 * This version does not timeout on its wait for ACK/NACK. Instead it just 
 * sends them 3 extra times. But the receive does filters out duplicate telegrams
 * in this link layer driver (LL) rahter then leave to the above application layer
 * Also we do set the repeat bit, so maybe works better with other KNX implementations
 * 
 * Additionally, we do not send out telegrams that are the result of, and exact copy of
 * one we just received. Not sure if this is KNX spec, but it is very useful to avoid
 * network broadcast storms
 */

#include "esp-knx-ip.h"
#ifdef KNX_REPEAT_FILTER

uint8_t cemi_data_prev_buf[23]; // Memory for previous received KNX telegram
uint32_t cemi_data_prev_stale;  // When the previous telegram turns stale 
/* low intensity KNX is assumed, memory is only one deep.

/**
 * Filter functions
 * Called from both send and receive, so made it into a functions
 */

bool KNX_filter(cemi_service_t *cemi_data, uint8_t knx_filtertype)
{
  // Memory of previous KNX telegram
  cemi_service_t *cemi_data_prev = (cemi_service_t *)cemi_data_prev_buf;

  //Because old style Tasmota KNX Enhance, we cannot trust the received repeat bit
  //bool knx_filter = (cemi_data->control_1.bits.repeat == 0x00); // 0 = repeated telegram, 1 = not repeated telegram

  bool knx_filter = (millis()-cemi_data_prev_stale)<CEMI_DATA_STALE_DURATION;
#ifdef ESP_KNX_DEBUG
  if (!knx_filter) { DEBUG_PRINTLN(F("Ignoring stale filter buffer.")); }
#endif

  // reflect filter only checks destination and payload
  // repeat filter also checks source, sequence, and routing counter
  if (knx_filter) { 
      knx_filter = cemi_data->data_len == cemi_data_prev->data_len &&
                   *(uint16_t *)&cemi_data->destination == *(uint16_t *)&cemi_data_prev->destination &&
                   (0==memcmp(&cemi_data->data, &cemi_data_prev->data, cemi_data->data_len));
  }
  if (knx_filter && knx_filtertype==KNX_FILTERTYPE_REPEAT ) { 
      knx_filter = *(uint16_t *)&cemi_data->source == *(uint16_t *)&cemi_data_prev->source &&
                   *(uint8_t *)&cemi_data->pci == *(uint8_t *)&cemi_data_prev->pci;
  }
#ifdef ESP_KNX_DEBUG
  if (knx_filter && knx_filtertype==KNX_FILTERTYPE_REPEAT) { 
    DEBUG_PRINTLN(F("Dropping receive of repeated telegram."));
  }
  if (knx_filter && knx_filtertype==KNX_FILTERTYPE_REFLECT) { 
    DEBUG_PRINTLN(F("Dropping send of reflected telegram."));
  }
#endif
  
  if (!knx_filter && knx_filtertype==KNX_FILTERTYPE_REPEAT) {
    // Remember new accepted telegram as new filter
    size_t cpylen = min(sizeof *cemi_data + cemi_data->data_len, sizeof cemi_data_prev_buf);
    memcpy(cemi_data_prev, cemi_data, cpylen );
    cemi_data_prev_stale = millis();
  }

  return knx_filter;
}
#endif // KNX_REPEAT_FILTER
