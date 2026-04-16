/**
 * @file        bacnet_handler.cpp
 * @brief       BACnet/IP protocol handler with full BTL-conformant feature set.
 *
 * @details     Integrates the bacnet-stack v1.5.0 rc5 (BIP datalink) and exposes
 *              the gateway's dynamic point map as BACnet Analog Value and Binary
 *              Value objects.  v1.2.0 adds the services required for interoperability
 *              with professional BAS supervisors (Desigo CC, EBI, etc.):
 *
 *              | Service                  | Handler                          |
 *              |--------------------------|----------------------------------|
 *              | ReadProperty             | handler_read_property            |
 *              | WriteProperty            | handler_write_property           |
 *              | ReadPropertyMultiple     | handler_read_property_multiple   |
 *              | SubscribeCOV             | handler_cov_subscribe            |
 *              | Who-Is / I-Am            | handler_who_is                   |
 *
 *              COV notifications are driven by the bacnet-stack's built-in
 *              Analog_Value_COV_Detect / Binary_Value_Change_Of_Value_COV_Detect
 *              machinery.  The default AV COV-Increment is 1.0 (one engineering
 *              unit).  Increment can be overridden by the supervisor via
 *              WriteProperty(COV-Increment).
 *
 *              Device stubs (Device_Read_Property, Device_Write_Property, etc.)
 *              are defined here because device.c is excluded from the custom
 *              library build (see lib/bacnet/library.json srcFilter).
 *
 *              Device_COV / Device_COV_Clear / Device_Value_List_Supported /
 *              Device_Encode_Value_List / Device_Objects_Property_List are also
 *              implemented here so that h_cov.c and h_rpm.c can dispatch to the
 *              correct AV/BV implementations without the full device.c machinery.
 *
 * @author      Doodz (DoodzProg)
 * @date        2026-04-16
 * @version     1.2.0
 * @repository  https://github.com/DoodzProg/ESP32-BMS-Gateway-Multi-Protocol
 */

#include "bacnet_handler.h"
#include "config.h"
#include "state.h"
#include "log_handler.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#ifdef ENABLE_MODBUS
  #include "modbus_handler.h"
#endif

// ==============================================================================
// UDP / DATALINK WRAPPERS
// ==============================================================================

WiFiUDP bacnetUDP;
bool    udpStarted = false;
IPAddress current_bacnet_ip;

static uint32_t _get_local_ip() {
    return (uint32_t)current_bacnet_ip[0]
         | ((uint32_t)current_bacnet_ip[1] << 8)
         | ((uint32_t)current_bacnet_ip[2] << 16)
         | ((uint32_t)current_bacnet_ip[3] << 24);
}

static int _udp_send(uint32_t dest_ip, uint16_t dest_port,
                     uint8_t* data, uint16_t len) {
    if (!udpStarted) { bacnetUDP.begin(BACNET_PORT); udpStarted = true; }
    IPAddress ip(dest_ip & 0xFF, (dest_ip >> 8) & 0xFF,
                 (dest_ip >> 16) & 0xFF, (dest_ip >> 24) & 0xFF);
    bacnetUDP.beginPacket(ip, dest_port);
    bacnetUDP.write(data, len);
    return bacnetUDP.endPacket() ? len : -1;
}

static int _udp_recv(uint32_t* src_ip, uint16_t* src_port,
                     uint8_t* buf, uint16_t max_len) {
    if (!udpStarted) { bacnetUDP.begin(BACNET_PORT); udpStarted = true; }
    int len = bacnetUDP.parsePacket();
    if (len <= 0) return 0;
    *src_port = bacnetUDP.remotePort();
    IPAddress ip = bacnetUDP.remoteIP();
    *src_ip = (uint32_t)ip[0] | ((uint32_t)ip[1] << 8)
            | ((uint32_t)ip[2] << 16) | ((uint32_t)ip[3] << 24);
    if (len > max_len) len = max_len;
    return bacnetUDP.read(buf, len);
}

extern "C" {
    uint32_t esp32_get_local_ip(void)                                      { return _get_local_ip(); }
    int esp32_udp_send(uint32_t d, uint16_t p, uint8_t* b, uint16_t l)   { return _udp_send(d, p, b, l); }
    int esp32_udp_recv(uint32_t* i, uint16_t* p, uint8_t* b, uint16_t l) { return _udp_recv(i, p, b, l); }
}

// ==============================================================================
// BACNET STACK INCLUDES
// ==============================================================================

extern "C" {
    #include "bacnet/bacdef.h"
    #include "bacnet/bacstr.h"
    #include "bacnet/config.h"

    #define BACnetHostOctetString  BACnet_Octet_String
    #define BACnetHostCharacterString BACnet_Character_String

    #include "bacnet/apdu.h"
    #include "bacnet/npdu.h"
    #include "bacnet/proplist.h"
    #include "bacnet/datalink/datalink.h"
    #include "bacnet/datalink/bip.h"
    #include "bacnet/basic/object/av.h"
    #include "bacnet/basic/object/bv.h"

    #include "bacnet/basic/services.h"
    #include "bacnet/basic/service/h_whois.h"
    #include "bacnet/basic/service/h_iam.h"
    #include "bacnet/basic/service/h_rp.h"
    #include "bacnet/basic/service/h_wp.h"
    #include "bacnet/basic/service/h_rpm.h"
    #include "bacnet/basic/service/h_cov.h"
    #include "bacnet/basic/service/s_cov.h"
    #include "bacnet/bacapp.h"
}

#ifndef MAX_MPDU
#define MAX_MPDU MAX_APDU
#endif

uint8_t Handler_Transmit_Buffer[MAX_MPDU];

// ==============================================================================
// RUNTIME DEVICE NAME STORAGE
// Populated by bacnet_init() from the NVS-supplied device name.
// ==============================================================================

static char g_device_name[64] = BACNET_DEVICE_NAME;

// ==============================================================================
// DEVICE OBJECT STUBS
// device.c is excluded from the library build; we provide the minimal stubs
// required by the APDU service handlers.
// ==============================================================================

extern "C" {

    /* ---- Core identity ---- */
    uint32_t Device_Object_Instance_Number(void) { return BACNET_DEVICE_ID; }
    bool     Device_Set_Object_Instance_Number(uint32_t n) { (void)n; return true; }
    bool     Device_Valid_Object_Instance_Number(uint32_t n) { return n == BACNET_DEVICE_ID; }
    unsigned Device_Count(void) { return 1; }
    uint32_t Device_Index_To_Instance(unsigned i) { (void)i; return BACNET_DEVICE_ID; }
    int      Device_Segmentation_Supported(void) { return 0; }

    /** @brief Returns the registered BTL Vendor ID for this device.
     *  @note  Value 260 is used as a clearly non-zero test identifier.
     *         Register a production ID at https://www.bacnet.org/VendorID/. */
    uint16_t Device_Vendor_Identifier(void) { return 260; }

    bool Device_Set_Object_Name(const BACNET_CHARACTER_STRING* s) { (void)s; return true; }

    /* ---- Object-list helpers used by h_rpm.c ---- */

    /**
     * @brief Validates an object-type / instance pair against this device's
     *        known AV/BV objects.
     */
    bool Device_Valid_Object_Id(BACNET_OBJECT_TYPE object_type,
                                uint32_t object_instance) {
        if (object_type == OBJECT_DEVICE && object_instance == BACNET_DEVICE_ID) {
            return true;
        }
        if (object_type == OBJECT_ANALOG_VALUE) {
            return Analog_Value_Valid_Instance(object_instance);
        }
        if (object_type == OBJECT_BINARY_VALUE) {
            return Binary_Value_Valid_Instance(object_instance);
        }
        return false;
    }

    /* ---- COV dispatch ---- */

    /**
     * @brief Polls the COV-changed flag for a given object (called by h_cov.c).
     */
    bool Device_COV(BACNET_OBJECT_TYPE object_type, uint32_t object_instance) {
        if (object_type == OBJECT_ANALOG_VALUE)
            return Analog_Value_Change_Of_Value(object_instance);
        if (object_type == OBJECT_BINARY_VALUE)
            return Binary_Value_Change_Of_Value(object_instance);
        return false;
    }

    /**
     * @brief Clears the COV-changed flag for a given object (called by h_cov.c).
     */
    void Device_COV_Clear(BACNET_OBJECT_TYPE object_type, uint32_t object_instance) {
        if (object_type == OBJECT_ANALOG_VALUE)
            Analog_Value_Change_Of_Value_Clear(object_instance);
        else if (object_type == OBJECT_BINARY_VALUE)
            Binary_Value_Change_Of_Value_Clear(object_instance);
    }

    /**
     * @brief Encodes the value-list for a COV notification (called by h_cov.c).
     */
    bool Device_Encode_Value_List(BACNET_OBJECT_TYPE object_type,
                                  uint32_t object_instance,
                                  BACNET_PROPERTY_VALUE* value_list) {
        if (object_type == OBJECT_ANALOG_VALUE)
            return Analog_Value_Encode_Value_List(object_instance, value_list);
        if (object_type == OBJECT_BINARY_VALUE)
            return Binary_Value_Encode_Value_List(object_instance, value_list);
        return false;
    }

    /**
     * @brief Returns true if the given object type supports value-list
     *        encoding for COV notifications (called by h_cov.c).
     */
    bool Device_Value_List_Supported(BACNET_OBJECT_TYPE object_type) {
        return (object_type == OBJECT_ANALOG_VALUE ||
                object_type == OBJECT_BINARY_VALUE);
    }

    /* ---- Property-list dispatch for ReadPropertyMultiple ---- */

    /**
     * @brief Fills a special_property_list_t for the given object type/instance.
     *        Called by h_rpm.c when the client requests PROP_ALL, PROP_REQUIRED,
     *        or PROP_OPTIONAL.
     */
    void Device_Objects_Property_List(BACNET_OBJECT_TYPE object_type,
                                      uint32_t object_instance,
                                      struct special_property_list_t* pPropertyList) {
        (void)object_instance;
        if (pPropertyList == NULL) return;
        pPropertyList->Required.pList    = NULL;
        pPropertyList->Required.count    = 0;
        pPropertyList->Optional.pList    = NULL;
        pPropertyList->Optional.count    = 0;
        pPropertyList->Proprietary.pList = NULL;
        pPropertyList->Proprietary.count = 0;

        if (object_type == OBJECT_DEVICE) {
            static const int32_t device_req[] = {
                PROP_OBJECT_IDENTIFIER,
                PROP_OBJECT_NAME,
                PROP_OBJECT_TYPE,
                PROP_SYSTEM_STATUS,
                PROP_VENDOR_NAME,
                PROP_VENDOR_IDENTIFIER,
                PROP_MODEL_NAME,
                PROP_FIRMWARE_REVISION,
                PROP_APPLICATION_SOFTWARE_VERSION,
                PROP_PROTOCOL_VERSION,
                PROP_PROTOCOL_REVISION,
                PROP_PROTOCOL_SERVICES_SUPPORTED,
                PROP_PROTOCOL_OBJECT_TYPES_SUPPORTED,
                PROP_OBJECT_LIST,
                PROP_MAX_APDU_LENGTH_ACCEPTED,
                PROP_SEGMENTATION_SUPPORTED,
                PROP_APDU_TIMEOUT,
                PROP_NUMBER_OF_APDU_RETRIES,
                PROP_DEVICE_ADDRESS_BINDING,
                PROP_DATABASE_REVISION,
                -1
            };
            pPropertyList->Required.pList    = device_req;
            pPropertyList->Required.count    = property_list_count(device_req);
            pPropertyList->Optional.pList    = NULL;
            pPropertyList->Optional.count    = 0;
            pPropertyList->Proprietary.pList = NULL;
            pPropertyList->Proprietary.count = 0;
            return;
        }

        const int32_t* pReq = NULL;
        const int32_t* pOpt = NULL;
        const int32_t* pProp = NULL;

        if (object_type == OBJECT_ANALOG_VALUE) {
            Analog_Value_Property_Lists(&pReq, &pOpt, &pProp);
        } else if (object_type == OBJECT_BINARY_VALUE) {
            Binary_Value_Property_Lists(&pReq, &pOpt, &pProp);
        } else {
            return;
        }

        if (pReq)  { pPropertyList->Required.pList    = pReq;  pPropertyList->Required.count    = property_list_count(pReq);  }
        if (pOpt)  { pPropertyList->Optional.pList    = pOpt;  pPropertyList->Optional.count    = property_list_count(pOpt);  }
        if (pProp) { pPropertyList->Proprietary.pList = pProp; pPropertyList->Proprietary.count = property_list_count(pProp); }
    }

    /* ---- Property-list membership (used by Device_Read_Property) ---- */
    bool Device_Objects_Property_List_Member(BACNET_OBJECT_TYPE object_type,
                                             uint32_t object_instance,
                                             BACNET_PROPERTY_ID object_property) {
        (void)object_instance;
        if (object_type == OBJECT_ANALOG_VALUE || object_type == OBJECT_BINARY_VALUE)
            return true;
        if (object_type == OBJECT_DEVICE) {
            switch (object_property) {
                case PROP_OBJECT_IDENTIFIER:
                case PROP_OBJECT_NAME:
                case PROP_OBJECT_TYPE:
                case PROP_SYSTEM_STATUS:
                case PROP_VENDOR_NAME:
                case PROP_VENDOR_IDENTIFIER:
                case PROP_MODEL_NAME:
                case PROP_FIRMWARE_REVISION:
                case PROP_APPLICATION_SOFTWARE_VERSION:
                case PROP_PROTOCOL_VERSION:
                case PROP_PROTOCOL_REVISION:
                case PROP_PROTOCOL_SERVICES_SUPPORTED:
                case PROP_PROTOCOL_OBJECT_TYPES_SUPPORTED:
                case PROP_OBJECT_LIST:
                case PROP_MAX_APDU_LENGTH_ACCEPTED:
                case PROP_SEGMENTATION_SUPPORTED:
                case PROP_APDU_TIMEOUT:
                case PROP_NUMBER_OF_APDU_RETRIES:
                case PROP_DEVICE_ADDRESS_BINDING:
                case PROP_DATABASE_REVISION:
                    return true;
                default: return false;
            }
        }
        return false;
    }

    /* ---- Network port stub ---- */
    uint32_t Network_Port_Index_To_Instance(unsigned i) { (void)i; return 0; }

    /* ---- Device Read / Write Property ---- */

    int Device_Read_Property(BACNET_READ_PROPERTY_DATA* rpdata) {
        if (rpdata->object_type == OBJECT_ANALOG_VALUE)
            return Analog_Value_Read_Property(rpdata);
        if (rpdata->object_type == OBJECT_BINARY_VALUE)
            return Binary_Value_Read_Property(rpdata);

        /* --- Device object properties --- */
        if (rpdata->object_property == PROP_OBJECT_LIST) {
            int apdu_len = 0;
            uint8_t* apdu = rpdata->application_data;

            int active_av = 0, active_bv = 0;
            for (int i = 0; i < NUM_ANALOG_POINTS; i++)
                if (analogPoints[i].protocol & PROTO_BACNET) active_av++;
            for (int i = 0; i < NUM_BINARY_POINTS; i++)
                if (binaryPoints[i].protocol & PROTO_BACNET) active_bv++;
            int total_objects = 1 + active_av + active_bv;

            if (rpdata->array_index == 0) {
                apdu_len = encode_application_unsigned(apdu, total_objects);
            } else if (rpdata->array_index == BACNET_ARRAY_ALL) {
                apdu_len += encode_application_object_id(&apdu[apdu_len], OBJECT_DEVICE, BACNET_DEVICE_ID);
                for (int i = 0; i < NUM_ANALOG_POINTS; i++)
                    if (analogPoints[i].protocol & PROTO_BACNET)
                        apdu_len += encode_application_object_id(&apdu[apdu_len], OBJECT_ANALOG_VALUE, analogPoints[i].bacnetInstance);
                for (int i = 0; i < NUM_BINARY_POINTS; i++)
                    if (binaryPoints[i].protocol & PROTO_BACNET)
                        apdu_len += encode_application_object_id(&apdu[apdu_len], OBJECT_BINARY_VALUE, binaryPoints[i].bacnetInstance);
            } else {
                if (rpdata->array_index == 1) {
                    apdu_len = encode_application_object_id(apdu, OBJECT_DEVICE, BACNET_DEVICE_ID);
                } else if (rpdata->array_index <= (uint32_t)(1 + active_av)) {
                    int target = rpdata->array_index - 1, current = 0;
                    for (int i = 0; i < NUM_ANALOG_POINTS; i++) {
                        if (analogPoints[i].protocol & PROTO_BACNET) {
                            current++;
                            if (current == target) {
                                apdu_len = encode_application_object_id(apdu, OBJECT_ANALOG_VALUE, analogPoints[i].bacnetInstance);
                                break;
                            }
                        }
                    }
                } else if (rpdata->array_index <= (uint32_t)total_objects) {
                    int target = rpdata->array_index - 1 - active_av, current = 0;
                    for (int i = 0; i < NUM_BINARY_POINTS; i++) {
                        if (binaryPoints[i].protocol & PROTO_BACNET) {
                            current++;
                            if (current == target) {
                                apdu_len = encode_application_object_id(apdu, OBJECT_BINARY_VALUE, binaryPoints[i].bacnetInstance);
                                break;
                            }
                        }
                    }
                } else {
                    return BACNET_STATUS_ERROR;
                }
            }
            rpdata->application_data_len = apdu_len;
            return apdu_len;
        }

        if (rpdata->object_property == PROP_OBJECT_NAME) {
            static BACNET_CHARACTER_STRING name;
            characterstring_init_ansi(&name, g_device_name);
            rpdata->application_data_len =
                encode_application_character_string(rpdata->application_data, &name);
            return rpdata->application_data_len;
        }
        if (rpdata->object_property == PROP_VENDOR_IDENTIFIER) {
            rpdata->application_data_len =
                encode_application_unsigned(rpdata->application_data, Device_Vendor_Identifier());
            return rpdata->application_data_len;
        }
        if (rpdata->object_property == PROP_PROPERTY_LIST) {
            int apdu_len = 0;
            uint8_t* apdu = rpdata->application_data;
            apdu_len += encode_application_enumerated(&apdu[apdu_len], PROP_OBJECT_NAME);
            apdu_len += encode_application_enumerated(&apdu[apdu_len], PROP_OBJECT_TYPE);
            apdu_len += encode_application_enumerated(&apdu[apdu_len], PROP_PRESENT_VALUE);
            apdu_len += encode_application_enumerated(&apdu[apdu_len], PROP_STATUS_FLAGS);
            apdu_len += encode_application_enumerated(&apdu[apdu_len], PROP_VENDOR_IDENTIFIER);
            rpdata->application_data_len = apdu_len;
            return apdu_len;
        }
        if (rpdata->object_property == PROP_OBJECT_IDENTIFIER) {
            rpdata->application_data_len =
                encode_application_object_id(rpdata->application_data, OBJECT_DEVICE, BACNET_DEVICE_ID);
            return rpdata->application_data_len;
        }
        if (rpdata->object_property == PROP_OBJECT_TYPE) {
            rpdata->application_data_len =
                encode_application_enumerated(rpdata->application_data, OBJECT_DEVICE);
            return rpdata->application_data_len;
        }
        if (rpdata->object_property == PROP_SYSTEM_STATUS) {
            rpdata->application_data_len =
                encode_application_enumerated(rpdata->application_data, STATUS_OPERATIONAL);
            return rpdata->application_data_len;
        }
        if (rpdata->object_property == PROP_VENDOR_NAME) {
            static BACNET_CHARACTER_STRING vname;
            characterstring_init_ansi(&vname, "DoodzProg");
            rpdata->application_data_len =
                encode_application_character_string(rpdata->application_data, &vname);
            return rpdata->application_data_len;
        }
        if (rpdata->object_property == PROP_MODEL_NAME) {
            static BACNET_CHARACTER_STRING mname;
            characterstring_init_ansi(&mname, "ESP32-BMS-Gateway");
            rpdata->application_data_len =
                encode_application_character_string(rpdata->application_data, &mname);
            return rpdata->application_data_len;
        }
        if (rpdata->object_property == PROP_FIRMWARE_REVISION) {
            static BACNET_CHARACTER_STRING fw;
            characterstring_init_ansi(&fw, "1.2.0");
            rpdata->application_data_len =
                encode_application_character_string(rpdata->application_data, &fw);
            return rpdata->application_data_len;
        }
        if (rpdata->object_property == PROP_APPLICATION_SOFTWARE_VERSION) {
            static BACNET_CHARACTER_STRING asv;
            characterstring_init_ansi(&asv, "1.2.0");
            rpdata->application_data_len =
                encode_application_character_string(rpdata->application_data, &asv);
            return rpdata->application_data_len;
        }
        if (rpdata->object_property == PROP_PROTOCOL_VERSION) {
            rpdata->application_data_len =
                encode_application_unsigned(rpdata->application_data, BACNET_PROTOCOL_VERSION);
            return rpdata->application_data_len;
        }
        if (rpdata->object_property == PROP_PROTOCOL_REVISION) {
            rpdata->application_data_len =
                encode_application_unsigned(rpdata->application_data, BACNET_PROTOCOL_REVISION);
            return rpdata->application_data_len;
        }
        if (rpdata->object_property == PROP_MAX_APDU_LENGTH_ACCEPTED) {
            rpdata->application_data_len =
                encode_application_unsigned(rpdata->application_data, MAX_APDU);
            return rpdata->application_data_len;
        }
        if (rpdata->object_property == PROP_SEGMENTATION_SUPPORTED) {
            rpdata->application_data_len =
                encode_application_enumerated(rpdata->application_data, SEGMENTATION_NONE);
            return rpdata->application_data_len;
        }
        if (rpdata->object_property == PROP_APDU_TIMEOUT) {
            rpdata->application_data_len =
                encode_application_unsigned(rpdata->application_data, 3000);
            return rpdata->application_data_len;
        }
        if (rpdata->object_property == PROP_NUMBER_OF_APDU_RETRIES) {
            rpdata->application_data_len =
                encode_application_unsigned(rpdata->application_data, 3);
            return rpdata->application_data_len;
        }
        if (rpdata->object_property == PROP_DEVICE_ADDRESS_BINDING) {
            rpdata->application_data_len = 0;
            return 0;
        }
        if (rpdata->object_property == PROP_DATABASE_REVISION) {
            rpdata->application_data_len =
                encode_application_unsigned(rpdata->application_data, 0);
            return rpdata->application_data_len;
        }
        if (rpdata->object_property == PROP_PROTOCOL_SERVICES_SUPPORTED) {
            BACNET_BIT_STRING svc;
            bitstring_init(&svc);
            bitstring_set_bit(&svc, SERVICE_CONFIRMED_COV_NOTIFICATION,    true);
            bitstring_set_bit(&svc, SERVICE_CONFIRMED_SUBSCRIBE_COV,       true);
            bitstring_set_bit(&svc, SERVICE_CONFIRMED_READ_PROPERTY,       true);
            bitstring_set_bit(&svc, SERVICE_CONFIRMED_READ_PROP_MULTIPLE,  true);
            bitstring_set_bit(&svc, SERVICE_CONFIRMED_WRITE_PROPERTY,      true);
            rpdata->application_data_len =
                encode_application_bitstring(rpdata->application_data, &svc);
            return rpdata->application_data_len;
        }
        if (rpdata->object_property == PROP_PROTOCOL_OBJECT_TYPES_SUPPORTED) {
            BACNET_BIT_STRING obj;
            bitstring_init(&obj);
            bitstring_set_bit(&obj, OBJECT_ANALOG_VALUE, true);
            bitstring_set_bit(&obj, OBJECT_BINARY_VALUE, true);
            bitstring_set_bit(&obj, OBJECT_DEVICE,       true);
            rpdata->application_data_len =
                encode_application_bitstring(rpdata->application_data, &obj);
            return rpdata->application_data_len;
        }
        return BACNET_STATUS_ERROR;
    }

    bool Device_Write_Property(BACNET_WRITE_PROPERTY_DATA* wp_data) {
        log_print("BACNET", "WRITE REQUEST received");

        if (wp_data->object_property == PROP_PRESENT_VALUE) {
            static BACNET_APPLICATION_DATA_VALUE value;
            int decode_len = bacapp_decode_application_data(
                wp_data->application_data, wp_data->application_data_len, &value);

            if (decode_len < 0) {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code  = ERROR_CODE_INVALID_DATA_TYPE;
                return false;
            }

            if (wp_data->object_type == OBJECT_ANALOG_VALUE) {
                if (value.tag == BACNET_APPLICATION_TAG_REAL) {
                    float new_val = value.type.Real;
                    for (int i = 0; i < NUM_ANALOG_POINTS; i++) {
                        if ((analogPoints[i].protocol & PROTO_BACNET) &&
                             analogPoints[i].bacnetInstance == wp_data->object_instance) {
                            analogPoints[i].value = new_val;
                            #ifdef ENABLE_MODBUS
                                modbus_write_analog(i);
                            #endif
                            Analog_Value_Present_Value_Set(
                                wp_data->object_instance, new_val, wp_data->priority);
                            log_printf("BACNET", "AV[%u] write → %.3f (pri %u)",
                                       wp_data->object_instance, new_val, wp_data->priority);
                            return true;
                        }
                    }
                }
            } else if (wp_data->object_type == OBJECT_BINARY_VALUE) {
                if (value.tag == BACNET_APPLICATION_TAG_ENUMERATED) {
                    bool new_val = (value.type.Enumerated == BINARY_ACTIVE);
                    for (int i = 0; i < NUM_BINARY_POINTS; i++) {
                        if ((binaryPoints[i].protocol & PROTO_BACNET) &&
                             binaryPoints[i].bacnetInstance == wp_data->object_instance) {
                            binaryPoints[i].value = new_val;
                            #ifdef ENABLE_MODBUS
                                modbus_write_binary(i);
                            #endif
                            Binary_Value_Present_Value_Set(
                                wp_data->object_instance,
                                new_val ? BINARY_ACTIVE : BINARY_INACTIVE);
                            log_printf("BACNET", "BV[%u] write → %s",
                                       wp_data->object_instance, new_val ? "ACTIVE" : "INACTIVE");
                            return true;
                        }
                    }
                }
            }
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code  = ERROR_CODE_INVALID_DATA_TYPE;
            return false;
        }

        if (wp_data->object_type == OBJECT_ANALOG_VALUE)
            return Analog_Value_Write_Property(wp_data);
        if (wp_data->object_type == OBJECT_BINARY_VALUE)
            return Binary_Value_Write_Property(wp_data);
        return false;
    }

    /* ---- Forward declarations to satisfy h_rpm.c ---- */
    bool Analog_Value_Name_Set(uint32_t object_instance, const char* new_name);
    bool Binary_Value_Name_Set(uint32_t object_instance, const char* new_name);


} // extern "C"

// ==============================================================================
// PUBLIC INIT
// ==============================================================================

void bacnet_init(IPAddress ip, const String& deviceName) {
    current_bacnet_ip = ip;

    // Store device name for BACnet Device Object Name property
    if (deviceName.length() > 0) {
        strncpy(g_device_name, deviceName.c_str(), sizeof(g_device_name) - 1);
        g_device_name[sizeof(g_device_name) - 1] = '\0';
    } else {
        strncpy(g_device_name, BACNET_DEVICE_NAME, sizeof(g_device_name) - 1);
    }

    // Initialise AV / BV objects from global state
    for (int i = 0; i < NUM_ANALOG_POINTS; i++) {
        if (analogPoints[i].protocol & PROTO_BACNET) {
            Analog_Value_Create(analogPoints[i].bacnetInstance);
            Analog_Value_Name_Set(analogPoints[i].bacnetInstance, analogPoints[i].name);
        }
    }
    for (int i = 0; i < NUM_BINARY_POINTS; i++) {
        if (binaryPoints[i].protocol & PROTO_BACNET) {
            Binary_Value_Create(binaryPoints[i].bacnetInstance);
            Binary_Value_Name_Set(binaryPoints[i].bacnetInstance, binaryPoints[i].name);
        }
    }

    // Register service handlers
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_PROPERTY,          handler_read_property);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_WRITE_PROPERTY,         handler_write_property);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_PROP_MULTIPLE,     handler_read_property_multiple);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_SUBSCRIBE_COV,          handler_cov_subscribe);

    // Initialise COV subscription manager
    handler_cov_init();

    // Configure BIP datalink
    bip_set_port(BACNET_PORT);

    BACNET_IP_ADDRESS local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.address[0] = ip[0];
    local_addr.address[1] = ip[1];
    local_addr.address[2] = ip[2];
    local_addr.address[3] = ip[3];
    local_addr.port = BACNET_PORT;
    bip_set_addr(&local_addr);
    bip_set_subnet_prefix(24);

    if (!bip_init(NULL)) {
        log_print("BACNET", "ERROR: BIP stack init failed");
        return;
    }

    Send_I_Am(&Handler_Transmit_Buffer[0]);

    log_printf("BACNET", "Ready — Device ID=%u Name='%s' VendorID=260 IP=%s",
               BACNET_DEVICE_ID, g_device_name, ip.toString().c_str());
}

// ==============================================================================
// OBJECT SYNCHRONISATION
// ==============================================================================

void bacnet_update_objects() {
    for (int i = 0; i < NUM_BINARY_POINTS; i++) {
        if (binaryPoints[i].protocol & PROTO_BACNET) {
            BACNET_BINARY_PV bv = binaryPoints[i].value ? BINARY_ACTIVE : BINARY_INACTIVE;
            Binary_Value_Present_Value_Set(binaryPoints[i].bacnetInstance, bv);
        }
    }
    for (int i = 0; i < NUM_ANALOG_POINTS; i++) {
        if (analogPoints[i].protocol & PROTO_BACNET) {
            Analog_Value_Present_Value_Set(
                analogPoints[i].bacnetInstance, analogPoints[i].value, 16);
        }
    }
}

// ==============================================================================
// MAIN TASK — called every loop()
// ==============================================================================

void bacnet_task() {
    static uint8_t pdu[MAX_APDU];
    static BACNET_ADDRESS src;

    // COV second-tick tracking
    static uint32_t last_cov_tick_ms = 0;
    uint32_t now = millis();
    if (now - last_cov_tick_ms >= 1000) {
        uint32_t elapsed = (now - last_cov_tick_ms) / 1000;
        last_cov_tick_ms = now;
        handler_cov_timer_seconds(elapsed);
    }

    // Process incoming UDP packets
    uint16_t pdu_len = bip_receive(&src, pdu, MAX_APDU, 0);
    if (pdu_len > 0) {
        #ifdef ENABLE_MODBUS
        extern ModbusIP mb;
        mb.task();
        #endif
        npdu_handler(&src, pdu, pdu_len);
    }

    // Drive COV finite-state machine (sends pending notifications)
    handler_cov_task();
}
