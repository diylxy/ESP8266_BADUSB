/*
   This software is licensed under the MIT License. See the license file for details.
   Source: https://github.com/spacehuhntech/WiFiDuck
 */

#pragma once

#define VERSION "1.0.4"

/*! ===== DEBUG Settings ===== */
 //#define ENABLE_DEBUG
 //#define DEBUG_PORT Serial
 //#define DEBUG_BAUD 115200

/*! ===== Communication Settings ===== */

// #define ENABLE_I2C
#define I2C_ADDR 0x31
// #define I2C_SDA 4
// #define I2C_SCL 5
#define I2C_CLOCK_SPEED 100000L

#define BUFFER_SIZE 384
#define PACKET_SIZE 32

#define MSG_CONNECTED "LED 0 0 25\n"
#define MSG_STARTED "LED 0 25 0\n"

/*! ======EEPROM Settings ===== */
#define EEPROM_SIZE       4095
#define EEPROM_BOOT_ADDR  3210
#define BOOT_MAGIC_NUM    1234567890

/*! ===== WiFi Settings ===== */
#define WIFI_SSID "wifiduck"
#define WIFI_PASSWORD "wifiduck"
#define WIFI_CHANNEL "1"

#define HOSTNAME "wifiduck"
#define URL "wifi.duck"

/*! ===== Parser Settings ===== */
#define CASE_SENSETIVE false
#define DEFAULT_SLEEP 5
