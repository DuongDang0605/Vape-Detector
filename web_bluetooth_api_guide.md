# Web Bluetooth Integration Guide - Vape Detector

This document is for the developer designing the web dashboard. It explains how to connect to the ESP32-S3 over Bluetooth Low Energy (BLE) and parse the real-time sensor metrics.

## 1. BLE Protocol Specifications

The ESP32-S3 runs a standard BLE GATT Server with the following identifiers:

*   **BLE Device Name (Advertised):** `ESP32S3_VAPE`
*   **GATT Service UUID:** `0000ffe0-0000-1000-8000-00805f9b34fb` (16-bit short form: `FFE0`)
*   **GATT Characteristic UUID:** `0000ffe1-0000-1000-8000-00805f9b34fb` (16-bit short form: `FFE1`)
*   **Characteristic Properties:** `READ`, `NOTIFY`

---

## 2. Data Packet Format (JSON)

Every 1 second, the ESP32-S3 broadcasts notifications on characteristic `FFE1` containing a UTF-8 JSON string:

```json
{
  "temp": 28.5,
  "humid": 55.2,
  "pm25": 12,
  "voc": 28450,
  "status": 0
}
```

### Parameter Mapping:
*   `temp`: Current temperature in Celsius (float).
*   `humid`: Current relative humidity in % (float).
*   `pm25`: PM2.5 particle concentration in ug/m³ (integer).
*   `voc`: SRAW_VOC ticks from SGP41 sensor (integer).
*   `status`: Current air quality status index (integer):
    *   `0`: **CLEAN AIR** (Normal)
    *   `1`: **VAPE DETECTED** (Alert! Trigger sound & log timestamp)
    *   `2`: **STEAM EXCLUDED** (Humid steam detected, ignore PM2.5)
    *   `3`: **CHEMICAL EXCLUDED** (Cleaning agent VOC detected, ignore)

---

## 3. JavaScript Web Bluetooth Implementation

The web application should use the HTML5 **Web Bluetooth API** to interact with the device.

The implemented dashboard gateway lives in
`fschool-iot-dashboard/app/bluetooth-gateway.ts`. It keeps this firmware packet
unchanged, adds browser-side metadata (`deviceId`, `bootId`, `sequence`,
`timestamp`, `buffered`), and sends it to
`POST /api/v1/telemetry/ble`. Failed uploads are queued in IndexedDB. Backend
history continues over REST and live server decisions are delivered with SSE.

> [!WARNING]
> Web Bluetooth requires a secure context. The web application must be served over **HTTPS** (or `localhost` during development).

### Sample Connection & Parser Script:
```javascript
// Configuration
const SERVICE_UUID = '0000ffe0-0000-1000-8000-00805f9b34fb';
const CHARACTERISTIC_UUID = '0000ffe1-0000-1000-8000-00805f9b34fb';

let bleDevice = null;
let dataCharacteristic = null;

// Call this function when the user clicks a "Connect BLE" button
async function connectBluetooth() {
  try {
    console.log("Requesting Bluetooth Device...");
    bleDevice = await navigator.bluetooth.requestDevice({
      filters: [{ name: 'ESP32S3_VAPE' }],
      optionalServices: [SERVICE_UUID]
    });

    console.log("Connecting to GATT Server...");
    const server = await bleDevice.gatt.connect();

    console.log("Getting Primary Service...");
    const service = await server.getPrimaryService(SERVICE_UUID);

    console.log("Getting Characteristic...");
    dataCharacteristic = await service.getCharacteristic(CHARACTERISTIC_UUID);

    // Subscribe to notifications (real-time updates)
    await dataCharacteristic.startNotifications();
    dataCharacteristic.addEventListener('characteristicvaluechanged', handleDataNotification);

    console.log("Connected successfully!");
    alert("Connected to Vape Detector via BLE!");

  } catch (error) {
    console.error("Bluetooth connection failed:", error);
    alert("Connection failed: " + error.message);
  }
}

// Handler for incoming notifications
function handleDataNotification(event) {
  const value = event.target.value;
  
  // Convert ArrayBuffer to UTF-8 String
  const decoder = new TextDecoder('utf-8');
  const jsonString = decoder.decode(value);
  
  try {
    const data = JSON.parse(jsonString);
    console.log("Received Sensor Data:", data);

    // Update your Web UI (HTML Elements)
    document.getElementById("temp-val").innerText = data.temp.toFixed(1) + " °C";
    document.getElementById("humid-val").innerText = data.humid.toFixed(1) + " %";
    document.getElementById("pm25-val").innerText = data.pm25 + " ug/m³";
    document.getElementById("voc-val").innerText = data.voc;

    // Handle Alert Status
    handleVapeAlert(data.status);

  } catch (e) {
    console.error("Failed to parse JSON packet:", jsonString, e);
  }
}

// Warning and Timestamp Logging logic
let lastStatus = 0;
function handleVapeAlert(status) {
  const statusLabels = ["Clean Air", "VAPE DETECTED", "Steam (Excluded)", "Chemical (Excluded)"];
  
  // Update status label in UI
  const statusEl = document.getElementById("status-label");
  statusEl.innerText = statusLabels[status];

  if (status === 1) {
    // 1. Vape detected
    statusEl.className = "alert-critical";
    
    // 2. Play warning buzzer sound on the browser
    playBrowserWarningBeep();

    // 3. Log timestamp if state just transitioned to Alert
    if (lastStatus !== 1) {
      logAlertTimestamp();
    }
  } else {
    statusEl.className = "status-normal";
  }

  lastStatus = status;
}

// Log event time in a history table
function logAlertTimestamp() {
  const now = new Date();
  const timeString = now.toLocaleDateString() + " " + now.toLocaleTimeString();
  
  const logTable = document.getElementById("alert-log-table");
  const newRow = logTable.insertRow(0); // Insert at top
  const cellTime = newRow.insertCell(0);
  const cellEvent = newRow.insertCell(1);
  
  cellTime.innerText = timeString;
  cellEvent.innerText = "VAPE DETECTED (Vape smoke)";
  cellEvent.style.color = "red";
  cellEvent.style.fontWeight = "bold";
}

// Web audio API simple buzzer sound
let audioCtx = null;
function playBrowserWarningBeep() {
  if (!audioCtx) {
    audioCtx = new (window.AudioContext || window.webkitAudioContext)();
  }
  const osc = audioCtx.createOscillator();
  const gain = audioCtx.createGain();
  
  osc.connect(gain);
  gain.connect(audioCtx.destination);
  
  osc.type = "sine";
  osc.frequency.setValueAtTime(2000, audioCtx.currentTime); // 2000Hz frequency beep
  gain.gain.setValueAtTime(0.5, audioCtx.currentTime);
  
  osc.start();
  osc.stop(audioCtx.currentTime + 0.3); // beep for 300ms
}
```
