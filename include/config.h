#pragma once

// ============================================================
// HARDWARE CONFIGURATION
// ============================================================
// Built-in LED pin. Set to 16 for the YD-ESP32-S3 N16R8 board used in this project!
// If you use a different ESP32-S3 board, update this to match your specific hardware ;)
#define LED_PIN 16

// ============================================================
// NETWORK & BACNET CONFIGURATION
// ============================================================
#define BACNET_PORT        47808
#define BACNET_DEVICE_ID   1234
#define BACNET_DEVICE_NAME "ESP32-AHU-Sim" // Note: Changed CTA (FR) to AHU (EN)

// ============================================================
// MODBUS TCP REGISTERS MAP
// ============================================================
#define MB_COIL_FAN_STATUS 10
#define MB_HREG_FAN_SPEED  100
#define MB_HREG_TEMP_SET   101

// ============================================================
// BACNET OBJECT INSTANCES MAP
// ============================================================
#define BAC_BV_FAN_STATUS  0
#define BAC_AV_FAN_SPEED   0
#define BAC_AV_TEMP_SET    1