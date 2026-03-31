#include "web_handler.h"
#include "state.h"
#include <WebServer.h>

// ============================================================
// SERVER INSTANCE
// ============================================================
WebServer server(80); // HTTP server for the dashboard

// ============================================================
// WEB INTERFACE (Embedded HTML/JS)
// ============================================================
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en" data-theme="dark">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>BMS Gateway</title>
  <style>
    /* Industrial Theme System */
    :root[data-theme="light"] {
        --bg-body: #e4e5e6;
        --bg-panel: #f4f5f6;
        --bg-card: #ffffff;
        --bg-screen: #e9ecef;
        --text-main: #2b2b2b;
        --text-dim: #6c757d;
        --color-run: #2e7d32;
        --color-stop: #c62828;
        --color-bar: #00838f;
        --color-temp: #ef6c00;
        --border-color: #ced4da;
        --screen-border-top: #adb5bd;
        --screen-border-bot: #ffffff;
    }

    :root[data-theme="dark"] {
        --bg-body: #121212;
        --bg-panel: #1e1e1e;
        --bg-card: #2a2a2a;
        --bg-screen: #151515; 
        --text-main: #e0e0e0;
        --text-dim: #888888;
        --color-run: #4caf50;
        --color-stop: #f44336;
        --color-bar: #00bcd4;   
        --color-temp: #ff9800;  
        --border-color: #3e3e3e;
        --screen-border-top: #0a0a0a;
        --screen-border-bot: #333333;
    }

    body {
        background-color: var(--bg-body);
        color: var(--text-main);
        font-family: 'Segoe UI', Arial, sans-serif;
        margin: 0;
        padding: 0;
        transition: background-color 0.3s ease, color 0.3s ease;
    }

    /* Top utility bar */
    .top-bar {
        border-bottom: 2px solid var(--border-color);
        background-color: var(--bg-panel);
        padding: 12px 24px;
        display: flex;
        justify-content: space-between;
        align-items: center;
        flex-wrap: wrap;
        gap: 12px;
        transition: background-color 0.3s ease, border-color 0.3s ease;
    }

    .top-bar-title {
        font-size: 16px;
        font-weight: 700;
        color: var(--text-main);
        letter-spacing: 1px;
        text-transform: uppercase;
    }

    .top-right {
        display: flex;
        align-items: center;
        gap: 16px;
    }

    /* Clickable Protocol Badges */
    .protocols {
        font-size: 11px;
        color: var(--text-dim);
        display: flex;
        gap: 12px;
    }

    .protocols span {
        background: var(--bg-body);
        padding: 4px 8px;
        border: 1px solid var(--border-color);
        letter-spacing: 0.5px;
        transition: all 0.2s ease;
        cursor: pointer;
        user-select: none;
        display: flex;
        align-items: center;
        justify-content: center;
        min-width: 110px;
    }

    .protocols span:hover {
        border-color: var(--color-bar);
        color: var(--text-main);
    }

    .protocols span:active {
        transform: scale(0.98);
    }

    /* SVG Theme Toggle Button */
    .theme-btn {
        background: var(--bg-card);
        color: var(--text-main);
        border: 1px solid var(--border-color);
        padding: 6px;
        border-radius: 2px;
        cursor: pointer;
        display: flex;
        align-items: center;
        justify-content: center;
        transition: all 0.2s ease;
    }
    .theme-btn:hover {
        background: var(--border-color);
    }

    /* Main Content Area */
    .main-content {
        padding: 24px;
        max-width: 1200px;
        margin: 0 auto;
    }

    /* Container for Read-Only objects */
    .telemetry-panel {
        background-color: var(--bg-panel);
        border: 1px solid var(--border-color);
        padding: 20px;
        border-radius: 4px;
        transition: all 0.3s ease;
    }

    .panel-header {
        font-size: 14px;
        font-weight: 700;
        color: var(--text-dim);
        margin-bottom: 20px;
        padding-bottom: 8px;
        border-bottom: 1px solid var(--border-color);
        letter-spacing: 1.5px;
    }

    .dashboard {
        display: grid;
        grid-template-columns: repeat(auto-fill, minmax(220px, 1fr));
        gap: 20px;
    }

    /* Flat Card Design */
    .card {
        background-color: var(--bg-card);
        border: 1px solid var(--border-color);
        padding: 16px;
        display: flex;
        flex-direction: column;
        transition: all 0.3s ease;
    }

    .card-header {
        display: flex;
        justify-content: space-between;
        align-items: center;
        margin-bottom: 12px;
    }

    .card-label {
        font-size: 12px;
        color: var(--text-main);
        text-transform: uppercase;
        letter-spacing: 0.5px;
        font-weight: 600;
    }

    .badge-ro {
        font-size: 9px;
        color: var(--text-dim);
        border: 1px solid var(--text-dim);
        padding: 2px 4px;
        border-radius: 2px;
        letter-spacing: 1px;
    }

    /* Sunken screen effect */
    .value-display {
        background-color: var(--bg-screen);
        border-top: 2px solid var(--screen-border-top);
        border-left: 2px solid var(--screen-border-top);
        border-bottom: 1px solid var(--screen-border-bot);
        border-right: 1px solid var(--screen-border-bot);
        padding: 12px 16px;
        display: flex;
        flex-direction: column;
        justify-content: center;
        min-height: 50px;
        transition: all 0.3s ease;
    }

    .value-wrapper {
        display: flex;
        justify-content: space-between;
        align-items: center;
    }

    .value-text {
        font-size: 28px;
        font-weight: 300;
        display: flex;
        align-items: center;
    }

    .unit {
        font-size: 14px;
        color: var(--text-dim);
        margin-left: 6px;
        font-weight: 600;
    }

    /* Square Binary Indicators */
    .indicator-square {
        width: 14px;
        height: 14px;
        margin-right: 12px;
        border-radius: 2px;
    }

    .state-run { background-color: var(--color-run); }
    .state-stop { background-color: var(--color-stop); }

    /* Analog Triangle Gauge */
    .gauge-container {
        width: 100%;
        height: 10px;
        background-color: var(--screen-border-top);
        margin-top: 12px;
        clip-path: polygon(0 100%, 100% 100%, 100% 0);
    }

    .gauge-fill {
        height: 100%;
        background-color: var(--color-bar);
        width: 0%; 
        transition: width 0.3s ease-in-out;
    }

    /* Vertical Thermometer */
    .thermo-vertical {
        display: flex;
        flex-direction: column;
        align-items: center;
        justify-content: flex-end;
        height: 38px;
        margin-left: 10px;
    }

    .thermo-tube-v {
        width: 8px;
        height: 100%;
        background-color: var(--screen-border-top);
        border-radius: 4px 4px 0 0;
        display: flex;
        flex-direction: column;
        justify-content: flex-end;
        z-index: 1;
    }

    .thermo-fill-v {
        width: 100%;
        background-color: var(--color-temp);
        border-radius: 4px 4px 0 0;
        height: 0%;
        transition: height 0.3s ease-in-out;
    }

    .thermo-bulb-v {
        width: 16px;
        height: 16px;
        background-color: var(--color-temp);
        border-radius: 50%;
        margin-top: -3px;
        z-index: 2;
    }
  </style>
</head>
<body>

  <div class="top-bar">
      <div class="top-bar-title">Web View - BMS Gateway Supervision</div>
      <div class="top-right">
          <div class="protocols">
              <span onclick="copyPort(this, '502', 'Modbus TCP: 502')" title="Click to copy port">Modbus TCP: 502</span>
              <span onclick="copyPort(this, '47808', 'BACnet/IP: 47808')" title="Click to copy port">BACnet/IP: 47808</span>
          </div>
          <button id="themeToggle" class="theme-btn" title="Toggle Theme">
              </button>
      </div>
  </div>

  <div class="main-content">
      <div class="telemetry-panel">
          <div class="panel-header">PROCESS TELEMETRY (READ-ONLY)</div>
          <div class="dashboard" id="dashboard">
              </div>
      </div>
  </div>

  <script>
    // =================================================================
    // COPY TO CLIPBOARD FUNCTIONALITY
    // =================================================================
    function copyPort(element, portNumber, originalText) {
        navigator.clipboard.writeText(portNumber).then(() => {
            element.innerText = "COPIED!";
            element.style.color = "var(--color-run)";
            element.style.borderColor = "var(--color-run)";
            
            setTimeout(() => {
                element.innerText = originalText;
                element.style.color = "";
                element.style.borderColor = "";
            }, 1500);
        }).catch(err => {
            console.error('Failed to copy text: ', err);
        });
    }

    // =================================================================
    // THEME MANAGEMENT
    // =================================================================
    const htmlElement = document.documentElement;
    const themeBtn = document.getElementById('themeToggle');
    const prefersDarkScheme = window.matchMedia("(prefers-color-scheme: dark)");

    const sunSvg = `<svg viewBox="0 0 24 24" width="16" height="16" stroke="currentColor" stroke-width="2" fill="none" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="5"></circle><line x1="12" y1="1" x2="12" y2="3"></line><line x1="12" y1="21" x2="12" y2="23"></line><line x1="4.22" y1="4.22" x2="5.64" y2="5.64"></line><line x1="18.36" y1="18.36" x2="19.78" y2="19.78"></line><line x1="1" y1="12" x2="3" y2="12"></line><line x1="21" y1="12" x2="23" y2="12"></line><line x1="4.22" y1="19.78" x2="5.64" y2="18.36"></line><line x1="18.36" y1="5.64" x2="19.78" y2="4.22"></line></svg>`;
    const moonSvg = `<svg viewBox="0 0 24 24" width="16" height="16" stroke="currentColor" stroke-width="2" fill="none" stroke-linecap="round" stroke-linejoin="round"><path d="M21 12.79A9 9 0 1 1 11.21 3 7 7 0 0 0 21 12.79z"></path></svg>`;

    function applyTheme(isDark) {
        htmlElement.setAttribute('data-theme', isDark ? 'dark' : 'light');
        themeBtn.innerHTML = isDark ? sunSvg : moonSvg;
    }

    applyTheme(prefersDarkScheme.matches);

    themeBtn.addEventListener('click', () => {
        let currentTheme = htmlElement.getAttribute('data-theme');
        applyTheme(currentTheme === 'light'); 
    });

    prefersDarkScheme.addEventListener('change', (e) => {
        applyTheme(e.matches);
    });

    // =================================================================
    // DASHBOARD RENDERING
    // =================================================================
    function createCard(name, value, isBinary) {
        let valueWrapperContent = "";
        let extraContent = "";
        let isTemp = name.toLowerCase().includes('temp');

        if (isBinary) {
            let stateClass = value ? "state-run" : "state-stop";
            let stateText = value ? "RUN" : "STOP";
            valueWrapperContent = `
                <div class="value-text">
                    <div class="indicator-square ${stateClass}"></div>
                    ${stateText}
                </div>`;
        } else {
            let floatValue = parseFloat(value).toFixed(1);
            
            if (isTemp) {
                let percent = Math.max(0, Math.min(100, (floatValue / 50) * 100)); 
                valueWrapperContent = `
                    <div class="value-text">${floatValue}<span class="unit">°C</span></div>
                    <div class="thermo-vertical">
                        <div class="thermo-tube-v">
                            <div class="thermo-fill-v" style="height: ${percent}%;"></div>
                        </div>
                        <div class="thermo-bulb-v"></div>
                    </div>`;
            } else {
                let percent = Math.max(0, Math.min(100, floatValue)); 
                valueWrapperContent = `<div class="value-text">${floatValue}<span class="unit">%</span></div>`;
                extraContent = `
                    <div class="gauge-container">
                        <div class="gauge-fill" style="width: ${percent}%;"></div>
                    </div>`;
            }
        }

        return `
            <div class="card">
                <div class="card-header">
                    <div class="card-label">${name}</div>
                    <div class="badge-ro">MONITOR</div>
                </div>
                <div class="value-display">
                    <div class="value-wrapper">
                        ${valueWrapperContent}
                    </div>
                    ${extraContent}
                </div>
            </div>`;
    }

    function renderDashboard(data) {
        let html = "";
        data.binary.forEach(pt => {
            html += createCard(pt.name, pt.value, true);
        });
        data.analog.forEach(pt => {
            html += createCard(pt.name, pt.value, false);
        });
        document.getElementById("dashboard").innerHTML = html;
    }

    // =================================================================
    // ESP32 REAL-TIME FETCH LOOP
    // =================================================================
    setInterval(() => {
      fetch('/api/data')
        .then(r => r.json())
        .then(data => renderDashboard(data))
        .catch(e => console.log("Connection lost", e));
    }, 1000);

  </script>
</body>
</html>
)rawliteral";

// ============================================================
// API ENDPOINTS
// ============================================================
/**
 * HTTP API Endpoint callback.
 * Dynamically serializes the global state arrays into a JSON response.
 */
void handleData() {
    String json = "{";
    
    // Serialize Binary Points
    json += "\"binary\":[";
    for (int i = 0; i < NUM_BINARY_POINTS; i++) {
        json += "{\"name\":\"" + String(binaryPoints[i].name) + "\",";
        json += "\"value\":" + String(binaryPoints[i].value ? "true" : "false") + "}";
        if (i < NUM_BINARY_POINTS - 1) json += ",";
    }
    json += "],";

    // Serialize Analog Points
    json += "\"analog\":[";
    for (int i = 0; i < NUM_ANALOG_POINTS; i++) {
        json += "{\"name\":\"" + String(analogPoints[i].name) + "\",";
        json += "\"value\":" + String(analogPoints[i].value) + "}";
        if (i < NUM_ANALOG_POINTS - 1) json += ",";
    }
    json += "]";
    
    json += "}";
    server.send(200, "application/json", json);
}

// ============================================================
// INITIALIZATION & TASK
// ============================================================
void web_server_init() {
    server.on("/", []() { server.send(200, "text/html", INDEX_HTML); });
    server.on("/api/data", handleData);
    server.begin();
}

void web_server_task() {
    server.handleClient();
}