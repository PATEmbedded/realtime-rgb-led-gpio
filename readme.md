# PAT RGB LED Driver (for Node.js)

A high-performance **RGB LED driver** for Orange Pi running Armbian, implemented in **C++** and **Node.js**. It provides real-time LED control via JSON-based commands and supports 2-bit PWM for smooth brightness transitions. Since it uses only 2 PWM channels, it operates the RGB LEDs through standard GPIO pins rather than requiring three dedicated PWM pins for Red, Green, and Blue. The driver also integrates with system services for automatic startup.

The **Node.js bridge** is designed for high-level application developers. It allows installation and usage of the RGB driver without dealing with low-level C++ code or hardware layers, providing an easy and reliable interface for controlling the LEDs programmatically.

---

## ⚡ Features

- **Low CPU Usage:** The driver uses less than **5% CPU** while running.
- **RGB Control:** Set Red, Green, Blue values individually via `RGB_Indicate(red, green, blue)`.
- **JSON Communication:** Seamless interface between C++ driver and Node.js bridge.
- **Real-Time Scheduling:** Uses `SCHED_FIFO` to ensure precise timing for LED updates.
- **Robust Signal Handling:** Handles termination and crash signals gracefully, ensuring LEDs are turned off on exit.
- **Duplicate Process Protection:** Ensures only one instance runs at a time.
- **High-Level Node.js Interface:** Allows developers to control the driver without touching low-level hardware code.
  ⚡ Features

---

## 📂 Project Structure

```
├─ src/                # C++ source code
│   ├─ rgb_driver.cpp  # Core driver logic
│   └─ rgb_json_config.h
├─ scripts/            # Build and setup scripts
│   ├─ build_rgb_driver.sh
│   └─ setup_rgb_driver.sh
├─ config/             # JSON configuration
│   ├─ rgb_config_cpp.json
│   └─ rgb_config_nodejs.json
├─ nodejs/             # Node.js bridge
│   └─ rgb_driver.js
├─ system/             # System integration
│   └─ rgb_driver.service
├─ docs/               # Optional documentation
│   └─ readme_rgb.txt
└─ README.md           # Project overview
```

---

## ⚙️ C++ Driver Setup

1. **Build the driver (optional)**:

```bash
cd scripts
./build_rgb_driver.sh
```

2. **Install system service**:

```bash
sudo ./setup_rgb_driver.sh
```

This sets up `rgb_driver.service` to run at boot and ensures the driver runs with real-time priority.

---

## 🖥 JSON Interface

The driver reads commands from `rgb_config_nodejs.json` and writes responses to `rgb_config_cpp.json`. A sample command:

```json
{
  "command": "indicate",
  "red": 255,
  "green": 0,
  "blue": 128,
  "updated": "2025-11-12T22:30:00"
}
```

The C++ driver validates JSON before applying the LED values.

---

# 🔧Example: Node.js Bridge Usage

The Node.js bridge provides a high-level interface for controlling the RGB driver without interacting with low-level hardware code:

```javascript
const JsonIOBridge = require("./JsonIOBridge");
const rgb = new JsonIOBridge(
  "rgb",
  "./config/rgb_config_nodejs.json",
  "./config/rgb_config_cpp.json"
);

// Set LED color
rgb.indicate(255, 128, 0);

// Optional: read current driver state
// rgb.read((data) => { console.log(data); });

// Optional: watch for state changes
// rgb.watch((data) => { console.log(data); });
```

---

## 🛡 Safety & Reliability

- Signals handled: `SIGINT`, `SIGTERM`, `SIGHUP`, `SIGQUIT`, `SIGABRT`, `SIGFPE`, `SIGILL`, `SIGSEGV`, `SIGBUS`, `SIGTRAP`.
- Duplicate processes are killed automatically.
- LEDs are turned off safely on driver exit.

---

## 🔄 Runtime Behavior

- **Idle Mode:** LEDs off, awaiting command.
- **Indicate Mode:** LEDs display the requested color with optional PWM for smooth transitions.
- **Error Handling:** If JSON write fails or driver encounters an error, LEDs are turned off and an error response is logged.

---

## 📌 Notes

- Designed for **embedded developers** needing precise control of RGB hardware.
- Works best on **Orange Pi/Armbian** with `wiringPi` installed.
- JSON-based communication allows integration with higher-level software or web interfaces.
- Node.js bridge allows high-level developers to interact with the driver easily without low-level code knowledge.
