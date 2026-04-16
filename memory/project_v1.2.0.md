---
name: v1.2.0 release state
description: What was implemented in v1.2.0 and key architectural decisions
type: project
---

v1.2.0 shipped 2026-04-16. BACnet conformance (COV, RPM, VendorID 260, dynamic device name) + Log Viewer SSE.

**Why:** DesigoCC tests cancelled but BACnet services (RPM, COV) are BTL-required regardless.

**Key files added:**
- src/log_handler.cpp + include/log_handler.h — 64-entry×80b ring buffer
- src/sc_stubs.c — 4 BACnet/SC stub functions (secure_connect.c excluded from build, bacapp.c references them)

**Key architectural note:**
device.c is NOT compiled (library.json srcFilter). All Device_* dispatch functions live in bacnet_handler.cpp. Any new BACnet service handler that calls Device_* functions needs a stub here.

**How to apply:** Before adding new BACnet services, check if Device_* functions are needed and add stubs in bacnet_handler.cpp extern "C" block.
