/**
 * @file        sc_stubs.c
 * @brief       BACnet/SC stub implementations (BACnet Secure Connect excluded).
 *
 * @details     secure_connect.c is excluded from the library build
 *              (see lib/bacnet/library.json srcFilter: -<secure_connect.c>).
 *              bacapp.c references these four SC-specific encode functions.
 *              Since this gateway never generates SC data types, all stubs
 *              safely return 0 (zero bytes encoded).
 *
 *              Kept in a pure-C file to avoid C++ name-mangling and to
 *              match the C linkage expected by bacapp.c.
 *
 * @author      Doodz (DoodzProg)
 * @date        2026-04-16
 * @version     1.2.0
 * @repository  https://github.com/DoodzProg/ESP32-BMS-Gateway-Multi-Protocol
 */

#include <stdint.h>
#include "bacnet/secure_connect.h"

int bacapp_encode_SCFailedConnectionRequest(
    uint8_t *apdu, const BACNET_SC_FAILED_CONNECTION_REQUEST *value) {
    (void)apdu; (void)value; return 0;
}

int bacapp_encode_SCHubConnection(
    uint8_t *apdu, const BACNET_SC_HUB_CONNECTION_STATUS *value) {
    (void)apdu; (void)value; return 0;
}

int bacapp_encode_SCHubFunctionConnection(
    uint8_t *apdu, const BACNET_SC_HUB_FUNCTION_CONNECTION_STATUS *value) {
    (void)apdu; (void)value; return 0;
}

int bacapp_encode_SCDirectConnection(
    uint8_t *apdu, const BACNET_SC_DIRECT_CONNECTION_STATUS *value) {
    (void)apdu; (void)value; return 0;
}
