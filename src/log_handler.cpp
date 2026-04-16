/**
 * @file        log_handler.cpp
 * @brief       In-RAM ring-buffer logger implementation.
 *
 * @details     Implements a fixed-capacity circular buffer of string entries.
 *              Each call to log_print() / log_printf() writes a timestamped,
 *              tagged line to both Serial and the ring buffer.
 *
 *              Ring buffer layout:
 *                - g_entries[LOG_BUF_ENTRIES][LOG_ENTRY_MAX]  — flat array
 *                - g_head     — index of the OLDEST valid entry
 *                - g_count    — number of valid entries (0..LOG_BUF_ENTRIES)
 *
 *              When g_count == LOG_BUF_ENTRIES the buffer is full; the next
 *              write overwrites the oldest entry and advances g_head.
 *
 * @author      Doodz (DoodzProg)
 * @date        2026-04-16
 * @version     1.2.0
 * @repository  https://github.com/DoodzProg/ESP32-BMS-Gateway-Multi-Protocol
 */

#include "log_handler.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// ==============================================================================
// RING BUFFER STATE
// ==============================================================================

static char     g_entries[LOG_BUF_ENTRIES][LOG_ENTRY_MAX];
static int      g_head  = 0;  /**< Index of the oldest entry.             */
static int      g_count = 0;  /**< Number of valid entries.               */
static uint32_t g_total = 0;  /**< Total entries ever written (monotonic).*/

// ==============================================================================
// HELPERS
// ==============================================================================

/**
 * @brief Returns the write index for the next entry.
 * @details When the buffer is full g_head is advanced (oldest dropped).
 */
static int _next_write_index() {
    if (g_count < LOG_BUF_ENTRIES) {
        return (g_head + g_count) % LOG_BUF_ENTRIES;
    }
    // Buffer full — overwrite oldest; advance head.
    int idx  = g_head;
    g_head   = (g_head + 1) % LOG_BUF_ENTRIES;
    return idx;
}

// ==============================================================================
// PUBLIC IMPLEMENTATION
// ==============================================================================

void log_init() {
    memset(g_entries, 0, sizeof(g_entries));
    g_head  = 0;
    g_count = 0;
    g_total = 0;
}

void log_print(const char* tag, const char* msg) {
    // Format: "[+ms] [TAG] message"
    char line[LOG_ENTRY_MAX];
    snprintf(line, sizeof(line), "[+%lums] [%s] %s", millis(), tag, msg);

    // Write to Serial
    Serial.println(line);

    // Write to ring buffer
    int idx = _next_write_index();
    strncpy(g_entries[idx], line, LOG_ENTRY_MAX - 1);
    g_entries[idx][LOG_ENTRY_MAX - 1] = '\0';
    if (g_count < LOG_BUF_ENTRIES) g_count++;
    g_total++;
}

void log_printf(const char* tag, const char* fmt, ...) {
    char msg[LOG_ENTRY_MAX];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);
    log_print(tag, msg);
}

String log_get_all() {
    String out;
    out.reserve(g_count * LOG_ENTRY_MAX);
    for (int i = 0; i < g_count; i++) {
        int idx = (g_head + i) % LOG_BUF_ENTRIES;
        out += g_entries[idx];
        out += '\n';
    }
    return out;
}

uint32_t log_get_total() {
    return g_total;
}

String log_get_since(uint32_t since_idx) {
    if (since_idx >= g_total) return "";  // nothing new

    // Cap at what is actually still in the ring buffer.
    uint32_t new_count = g_total - since_idx;
    if (new_count > (uint32_t)g_count) new_count = (uint32_t)g_count;

    String out;
    out.reserve((int)new_count * LOG_ENTRY_MAX);

    // Physical index of the first entry to return:
    //   offset from oldest buffered = g_count - new_count
    //   physical = (g_head + g_count - new_count) % BUF
    int start_phys = (g_head + g_count - (int)new_count) % LOG_BUF_ENTRIES;

    for (uint32_t i = 0; i < new_count; i++) {
        int phys = (start_phys + (int)i) % LOG_BUF_ENTRIES;
        out += g_entries[phys];
        out += '\n';
    }
    return out;
}
