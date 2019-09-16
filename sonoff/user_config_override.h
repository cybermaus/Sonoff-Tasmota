/*
  user_config_override.h - user configuration overrides my_user_config.h for Sonoff-Tasmota

  Copyright (C) 2019  Theo Arends

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _USER_CONFIG_OVERRIDE_H_
#define _USER_CONFIG_OVERRIDE_H_

// force the compiler to show a warning to confirm that this file is included
#warning **** user_config_override.h: Using Settings from this File ****

/*****************************************************************************************************\
 * USAGE:
 *   To modify the stock configuration without changing the my_user_config.h file:
 *   (1) copy this file to "user_config_override.h" (It will be ignored by Git)
 *   (2) define your own settings below
 *   (3) for platformio:
 *         define USE_CONFIG_OVERRIDE as a build flags.
 *         ie1 : export PLATFORMIO_BUILD_FLAGS='-DUSE_CONFIG_OVERRIDE'
 *         ie2 : enable in file platformio.ini "build_flags = -Wl,-Tesp8266.flash.1m0.ld -DUSE_CONFIG_OVERRIDE"
 *       for Arduino IDE:
 *         enable define USE_CONFIG_OVERRIDE in my_user_config.h
 ******************************************************************************************************
 * ATTENTION:
 *   - Changes to SECTION1 PARAMETER defines will only override flash settings if you change define CFG_HOLDER.
 *   - Expect compiler warnings when no ifdef/undef/endif sequence is used.
 *   - You still need to update my_user_config.h for major define USE_MQTT_TLS.
 *   - All parameters can be persistent changed online using commands via MQTT, WebConsole or Serial.
\*****************************************************************************************************/


// If not selected the default will be SONOFF_BASIC
//#define MODULE                 SONOFF_BASIC      // [Module] Select default model from sonoff_template.h
//#define MODULE                 SHELLY1           // [Module] All Camping/FTF devices are Shelly1


// Reduce wear on flash
#undef  SAVE_STATE
#define SAVE_STATE             0                  // [SetOption0] Save changed power state to Flash (0 = disable, 1 = enable)
#undef  APP_POWERON_STATE
#define APP_POWERON_STATE      POWER_ALL_OFF      // [PowerOnState] Power On Relay state


// -- Setup your own Wifi settings  For Camping and Home
// Note that Devices password maybe cleartext, but my Devices VLAN is isolated from internet anyway. 
// Only for untrusted chinese webcamera's and such trojan horse riddled junk.
#undef WIFI_CONFIG_TOOL                           // [WifiConfig] Default tool if wifi fails to connect
#define WIFI_CONFIG_TOOL       WIFI_WAIT          // WAIT does not reboot, RETRY does reboot
#undef  STA_SSID1
#define STA_SSID1              "Devices"          // [Ssid1] Optional alternate AP Wifi SSID
#undef  STA_PASS1
#define STA_PASS1              "#T03gang"         // [Password1] Optional alternate AP Wifi password


// Prevent OAT
#undef OTA_URL
#define OTA_URL                ""  // [OtaUrl]


// Common Camping and Home local NTP servers
// Note source also has improvements to retain time after reboot by using RTC-Memory
#undef  NTP_SERVER1
#define NTP_SERVER1            "192.168.178.1"   // [NtpServer1] Select first NTP server by name or IP address (129.250.35.250)
#undef  NTP_SERVER2
#define NTP_SERVER2            "192.168.160.1"   // [NtpServer2] Select second NTP server by name or IP address (5.39.184.5)
#undef  NTP_SERVER3
#define NTP_SERVER3            "192.168.190.1"   // [NtpServer3] Select third NTP server by name or IP address (93.94.224.67)
// Location for Sunrise/Sunset
#undef LATITUDE 
#undef LONGITUDE
//#define LATITUDE               58.7986           // Camping
//#define LONGITUDE              14.5391           // Camping
#define LATITUDE               51.63             // Home
#define LONGITUDE               4.48             // Home
// Maybe I should use CIVIL instead of +30 minutes, especially in high altitudes
// Found the best time to be the middle of Dawn, so DAWN/2
#undef  SUNRISE_DAWN_ANGLE 
#define SUNRISE_DAWN_ANGLE DAWN_CIVIL/2
// Use automatice Daylight/Standard timezones
#undef  APP_TIMEZONE
#define APP_TIMEZONE           99                // [Timezone] +1 hour (Amsterdam) (-13 .. 14 = hours from UTC, 99 = use TIME_DST/TIME_STD)


// Build in, but do not activate MQTT
#undef  MQTT_USE
#define MQTT_USE               0                 // [SetOption3] Select default MQTT use (0 = Off, 1 = On)
// Build in KNX
#define USE_KNX                                  // Enable KNX IP Protocol Support (+9.4k code, +3k7 mem)


// MAURITS_MODS next lines disable all I2C devices to save program size space
/* #undef USE_I2C                                  // I2C using library wire (+10k code, 0k2 mem, 124 iram)
#undef USE_SPI                                  // Hardware SPI using GPIO12(MISO), GPIO13(MOSI) and GPIO14(CLK) in addition to two user selectable GPIOs(CS and DC)
#undef USE_IR_REMOTE                            // Send IR remote commands using library IRremoteESP8266 and ArduinoJson (+4k3 code, 0k3 mem, 48 iram)
#undef USE_IR_HVAC                              // Support for HVAC (Toshiba, Mitsubishi and LG) system using IR (+3k5 code)
#undef USE_IR_RECEIVE                           // Support for IR receiver (+7k2 code, 264 iram)
#undef USE_WS2812                               // WS2812 Led string using library NeoPixelBus (+5k code, +1k mem, 232 iram) - Disable by //
#undef USE_ARILUX_RF                            // Add support for Arilux RF remote controller (+0k8 code, 252 iram (non 2.3.0))
#undef USE_SR04                                 // Add support for HC-SR04 ultrasonic devices (+1k code)
#undef USE_TM1638                               // Add support for TM1638 switches copying Switch1 .. Switch8 (+1k code)
#undef USE_HX711                                // Add support for HX711 load cell (+1k5 code)
#undef USE_RF_FLASH                             // Add support for flashing the EFM8BB1 chip on the Sonoff RF Bridge. C2CK must be connected to GPIO4, C2D to GPIO5 on the PCB (+3k code)
#undef USE_TX20_WIND_SENSOR                     // Add support for La Crosse TX20 anemometer (+2k code)
#undef USE_RC_SWITCH                            // Add support for RF transceiver using library RcSwitch (+2k7 code, 460 iram)
#undef USE_RF_SENSOR                            // Add support for RF sensor receiver (434MHz or 868MHz) (+0k8 code)
#undef USE_THEO_V2                              // Add support for decoding Theo V2 sensors as documented on https://sidweb.nl using 434MHz RF sensor receiver (+1k4 code)
#undef USE_ALECTO_V2                            // Add support for decoding Alecto V2 sensors like ACH2010, WS3000 and DKW2012 weather stations using 868MHz RF sensor receiver (+1k7 code)
#undef USE_SM16716                              // Add support for SM16716 RGB LED controller (+0k7 code)
#undef USE_HRE                                  // Add support for Badger HR-E Water Meter (+1k4 code)
*/

#endif  // _USER_CONFIG_OVERRIDE_H_