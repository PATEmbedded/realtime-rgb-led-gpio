# RGB Driver Project

This project provides a C++ and Node.js implementation for controlling RGB hardware on the Orange Pi using Armbian. It includes source code, configuration files, system service definitions, and utility scripts to install, manage, and test the RGB driver.

---

## 📁 Folder Structure Overview

### `src/`

C++ source files for the RGB driver.

- `rgb_driver.cpp`: Core driver logic for controlling RGB LEDs.
- `rgb_json_config.h`: Header file for reading and handling JSON configuration.

---

### `scripts/`

Helper and automation shell scripts.

- `setup_rgb_driver.sh`: Installs the systemd service and deploys the RGB driver.
- `build_rgb_driver.sh`: Compiles the C++ driver (optional usage).

---

### `config/`

Configuration files for both C++ and Node.js versions.

- `rgb_config_cpp.json`: JSON configuration used by the C++ RGB driver.
- `rgb_config_node.json`: JSON configuration used by the Node.js driver.

---

### `nodejs/`

Node.js version of the RGB driver.

- `rgb_driver.js`: JavaScript implementation of the RGB driver for runtime via Node.js.

---

### `system/`

System-level integration files.

- `rgb_driver.service`: systemd service file to autostart the RGB driver on boot.

---

### `docs/`

Optional documentation and help files.

- `readme_rgb.txt`: A simple text version of this README or extra notes.
- `rgb_help.txt`: Command list or usage help for users or maintainers.

---

### `README.md`

This file. Explains the purpose and layout of the project.

---

## 🛠 Setup Instructions (C++)

```bash
cd scripts
sudo ./setup_rgb_driver.sh
```
