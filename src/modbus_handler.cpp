#include "modbus_handler.h"
#include "config.h"
#include "state.h"
#include <ModbusIP_ESP8266.h>

// ============================================================
// SERVER INSTANCE
// ============================================================
ModbusIP mb; // Modbus TCP server handler

// ============================================================
// INITIALIZATION
// ============================================================
void modbus_init() {
    mb.server();
    
    // Dynamically initialize all binary points (Coils)
    for (int i = 0; i < NUM_BINARY_POINTS; i++) {
        mb.addCoil(binaryPoints[i].mb_coil);
    }

    // Dynamically initialize all analog points (Holding Registers)
    for (int i = 0; i < NUM_ANALOG_POINTS; i++) {
        mb.addHreg(analogPoints[i].mb_hreg);
    }
}

// ============================================================
// TASKS & SYNCHRONIZATION
// ============================================================
void modbus_task() {
    mb.task(); // Process incoming Modbus TCP requests
}

void modbus_sync_from_registers() {
    for (int i = 0; i < NUM_BINARY_POINTS; i++) {
        binaryPoints[i].value = mb.Coil(binaryPoints[i].mb_coil);
    }

    for (int i = 0; i < NUM_ANALOG_POINTS; i++) {
        uint16_t raw_val = mb.Hreg(analogPoints[i].mb_hreg);
        analogPoints[i].value = (float)raw_val / analogPoints[i].mb_scale;
    }
}

// ============================================================
// WRITERS (Called when BACnet changes a value)
// ============================================================
void modbus_write_binary(int index) {
    mb.Coil(binaryPoints[index].mb_coil, binaryPoints[index].value);
}

void modbus_write_analog(int index) {
    uint16_t raw_val = (uint16_t)(analogPoints[index].value * analogPoints[index].mb_scale);
    mb.Hreg(analogPoints[index].mb_hreg, raw_val);
}