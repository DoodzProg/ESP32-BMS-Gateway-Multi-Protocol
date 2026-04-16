/**
 * @file        log_handler.h
 * @brief       In-RAM ring-buffer logger with dual output (Serial + RAM).
 *
 * @details     Provides a lightweight circular log buffer that captures
 *              critical system and protocol events. Each entry is written
 *              simultaneously to the hardware Serial port and to the
 *              in-RAM ring buffer so that:
 *              - Developers can observe logs in real-time on Serial.
 *              - The Web UI can retrieve recent log entries via SSE without
 *                requiring persistent storage on LittleFS.
 *
 *              Buffer capacity: LOG_BUF_ENTRIES × LOG_ENTRY_MAX bytes.
 *              Oldest entries are silently overwritten when the buffer is full.
 *
 * @note        Not thread-safe by design — the project runs a single Arduino
 *              loop, so no mutex is required.
 *
 * @author      Doodz (DoodzProg)
 * @date        2026-04-16
 * @version     1.2.0
 * @repository  https://github.com/DoodzProg/ESP32-BMS-Gateway-Multi-Protocol
 */

#pragma once

#include <Arduino.h>

// ==============================================================================
// CONFIGURATION
// ==============================================================================

/** @brief Number of entries the ring buffer can hold before overwriting. */
#define LOG_BUF_ENTRIES  64

/** @brief Maximum length of a single formatted log entry (incl. null). */
#define LOG_ENTRY_MAX    80

// ==============================================================================
// PUBLIC API
// ==============================================================================

/**
 * @brief Initialises the log ring buffer (clears all entries).
 * @note  Call once from setup() before any log_print() call.
 */
void log_init();

/**
 * @brief Appends a tagged message to the ring buffer and prints to Serial.
 * @param tag   Short identifier, e.g. "SYSTEM", "BACNET", "WEB".
 * @param msg   Human-readable message string.
 */
void log_print(const char* tag, const char* msg);

/**
 * @brief Appends a printf-formatted message to the ring buffer and Serial.
 * @param tag   Short identifier.
 * @param fmt   printf-style format string.
 * @param ...   Variadic arguments.
 */
void log_printf(const char* tag, const char* fmt, ...);

/**
 * @brief Returns all current log entries as a single newline-delimited string.
 * @details Entries are ordered oldest-first.
 * @return Arduino String containing all entries, each terminated by '\n'.
 */
String log_get_all();

/**
 * @brief Returns the total number of log entries ever written (monotonic).
 * @details Used as a cursor: pass the returned value as `since_idx` on the
 *          next call to log_get_since() to receive only new entries.
 */
uint32_t log_get_total();

/**
 * @brief Returns entries written since the given cursor, oldest-first.
 * @param since_idx  Cursor from a previous log_get_total() call. Pass 0 for
 *                   the full ring-buffer snapshot.
 * @return Newline-delimited string of new entries. Empty if nothing new.
 */
String log_get_since(uint32_t since_idx);
