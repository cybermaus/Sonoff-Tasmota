/**
 * esp-knx-ip library for KNX/IP communication on an ESP8266
 * Main Author KNX driver for ESP: Nico Weichbrodt <envy>
 * This file: Maurits van Dueren <cybermaus>
 * License: MIT
 */

#include "esp-knx-ip.h"

// 2020-10-09 Maurits van Dueren : Repeat filter and No Reflected broadcast
// Memory for previous KNX frame to aid in simple repeat & reflection filter
// This only supports normal telegrams up to 23 characters
uint8_t cemi_data_prev_buf[23]; // Memory for previous received KNX telegram
uint32_t cemi_data_prev_stale;  // When the previous telegram turns stale 
#define CEMI_DATA_STALE_DURATION 500 // in ms 
#define KNX_FILTERTYPE_REPEAT 0x00 
#define KNX_FILTERTYPE_REFLECT 0x01 

/**
 * Filter functions
 * Called from both send and receive, so made it into a functions
 */

bool KNX_filter(cemi_service_t *cemi_data, uint8_t knx_filtertype)
{
  bool knx_filter = true; // assume we have to filter
  // Memory of previous KNX telegram
  cemi_service_t *cemi_data_prev = (cemi_service_t *)cemi_data_prev_buf;

  if (knx_filter && (millis()-cemi_data_prev_stale)>CEMI_DATA_STALE_DURATION) { 
    DEBUG_PRINTLN(F("Ignoring stale buffer."));
    knx_filter = false; 
  }

  if (knx_filter && (cemi_data->data_len == cemi_data_prev->data_len)) { 
    if (knx_filtertype==KNX_FILTER_REPEAT) {
      size_t cmplen = sizeof cemi_data->source +
                      sizeof cemi_data->destination +
                      sizeof cemi_data->data_len +
                      sizeof cemi_data->pci + 
                      cemi_data->data_len;
      knx_filter = (0==memcmp(&cemi_data->source, &cemi_data_prev->source, cmplen));
    } else { // knx_filtertype==KNX_FILTER_REFLECT
      size_t cmplen = sizeof cemi_data->destination +
                      sizeof cemi_data->data_len +
                      sizeof cemi_data->pci + 
                      cemi_data->data_len;
      knx_filter = (0==memcmp(&cemi_data->destination, &cemi_data_prev->source, cmplen));
    }      
  } else {
    knx_filter=false;
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
    // Remember telegram for next compare
    size_t cpylen = min(sizeof *cemi_data + cemi_data->data_len, sizeof cemi_data_prev_buf);
    memcpy(cemi_data_prev, cemi_data, cpylen );
    cemi_data_prev_stale=millis();
  }

  return knx_filter;
}
