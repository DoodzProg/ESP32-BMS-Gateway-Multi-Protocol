/*
 * AHU (Air Handling Unit) Multi-Protocol Simulator
 * Target Hardware: ESP32-S3 (WROOM/N16R8)
 */

#include <Arduino.h>
#include <WiFi.h>
#include "secrets.h"
#include "config.h"
#include "state.h"

// Conditional includes based on platformio.ini flags
#ifdef ENABLE_MODBUS
  #include "modbus_handler.h"
#endif

#ifdef ENABLE_BACNET
  #include "bacnet_handler.h"
#endif

#ifdef ENABLE_WEB
  #include "web_handler.h"
#endif

// ============================================================
// MAIN ARDUINO SETUP
// ============================================================
void setup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);

    // 1. Initialize Wi-Fi
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS); 
    
    Serial.print("\nConnexion au WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\n--- WIFI CONNECTE ---");
    Serial.print("Nouvelle IP : "); 
    Serial.println(WiFi.localIP());

    // 2. Initialize Enabled Modules
#ifdef ENABLE_MODBUS
    modbus_init();
    Serial.println("Modbus TCP pret.");
#endif

#ifdef ENABLE_WEB
    web_server_init();
    Serial.println("Serveur Web pret.");
#endif

#ifdef ENABLE_BACNET
    bacnet_init();
    Serial.println("BACnet/IP pret.");
#endif
}

// ============================================================
// MAIN ARDUINO LOOP
// ============================================================
void loop() {
    // 1. Process Network Tasks
#ifdef ENABLE_MODBUS
    modbus_task();
#endif

#ifdef ENABLE_WEB
    web_server_task();
#endif

#ifdef ENABLE_BACNET
    bacnet_task();
#endif

    // 2. Synchronize States (Modbus -> State -> BACnet)
#ifdef ENABLE_MODBUS
    modbus_sync_from_registers();
#endif

#ifdef ENABLE_BACNET
    bacnet_update_objects();
#endif

    // 3. Hardware Feedback (Uses the first binary point as an example)
    if (NUM_BINARY_POINTS > 0) {
        digitalWrite(LED_PIN, binaryPoints[0].value ? LOW : HIGH);
    }
}