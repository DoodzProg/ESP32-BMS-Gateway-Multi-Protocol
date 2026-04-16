/**
 * @file        bacnet_handler.h
 * @brief       BACnet/IP protocol handler and dynamic binding declarations.
 * @details     Exposes the global state as BACnet objects and intercepts write requests.
 *              v1.2.0 additions: COV subscriptions, ReadPropertyMultiple,
 *              dynamic device name from NVS, Vendor ID 260.
 * @author      Doodz (DoodzProg)
 * @date        2026-04-16
 * @version     1.2.0
 * @repository  https://github.com/DoodzProg/ESP32-BMS-Gateway-Multi-Protocol
 */

#pragma once
#include <Arduino.h> // Required for IPAddress class definition

// ==============================================================================
// BACNET HANDLER (Declarations)
// ==============================================================================

/**
 * @brief Initializes the BACnet stack and binds it to a specific IP address.
 * @param ip         The IPAddress to bind to (typically STA primary IP or AP fallback IP).
 * @param deviceName Human-readable device name for the BACnet Device Object Name property.
 *                   Persisted via NVS by main.cpp; defaults to BACNET_DEVICE_NAME if empty.
 */
void bacnet_init(IPAddress ip, const String& deviceName);

/**
 * @brief Synchronizes internal state variables to BACnet objects.
 * @note Called continuously in the main loop to keep BACnet Present_Values updated.
 */
void bacnet_update_objects();

/**
 * @brief Processes incoming BACnet UDP packets and handles stack routines.
 * @note Must be called frequently in the main loop.
 */
void bacnet_task();