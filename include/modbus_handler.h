#pragma once
#include <ModbusIP_ESP8266.h>

// ============================================================
// MODBUS HANDLER (Declarations)
// ============================================================

extern ModbusIP mb; 

void modbus_init();
void modbus_task();
void modbus_sync_from_registers();
void modbus_write_binary(int index);
void modbus_write_analog(int index);