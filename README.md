# RFID-SoftSPI-Driver
[![GitHub Stars](https://img.shields.io/github/stars/Leeri1y/RFID-SoftSPI-Driver?style=flat-square&color=yellow)](https://github.com/Leeri1y/RFID-SoftSPI-Driver/stargazers)
[![GitHub Forks](https://img.shields.io/github/forks/Leeri1y/RFID-SoftSPI-Driver?style=flat-square&color=blue)](https://github.com/Leeri1y/RFID-SoftSPI-Driver/fork)
[![License](https://img.shields.io/github/license/Leeri1y/RFID-SoftSPI-Driver?style=flat-square&color=green)](LICENSE)

A **software SPI-based driver** for RFID RC522 modules, designed to solve SPI pin conflicts with hardware SPI devices (e.g., TFT displays). No external libraries required—pure Arduino native code.

## 🌟 Core Features
- **Pin Flexibility**: Customize SCK/MOSI/MISO pins freely (no dependency on fixed hardware SPI pins).
- **Conflict Resolution**: Works with TFT displays using hardware SPI (no pin overlap).
- **Full Compatibility**: Supports Mifare 1K/4K cards, retains all RC522 functions (read/write/auth/halt).
- **Easy Integration**: Drop-in replacement for standard RFID libraries (same API for quick migration).

## 🛠️ Hardware Preparation
| Component       | Model/Spec                |
|-----------------|----------------------------|
| Microcontroller | Arduino UNO/Nano/Mega      |
| RFID Module     | RC522 (13.56MHz)            |
| Display (Optional) | TFT ST7789/ILI9341       |
| Power Supply    | 3.3V (RFID module, avoid 5V) |

### Pin Connection (Software SPI)
| RFID RC522 | Arduino Pin | Function Description       |
|------------|-------------|----------------------------|
| 3.3V       | 3.3V        | Power (**DO NOT USE 5V**)  |
| GND        | GND         | Ground                     |
| RST        | 5           | Reset Pin (Customizable)    |
| SCK        | 6           | Software SPI Clock         |
| MOSI       | 4           | Software SPI Data Output   |
| MISO       | 3           | Software SPI Data Input    |
| SDA (CS)   | 9           | Chip Select Pin            |
| IRQ        | -           | Not Used                   |

> **Note**: Modify pin definitions in `RFID.cpp` if you need different pins.

## 📦 Installation
1. Download the repository as ZIP: [RFID-SoftSPI-Driver.zip](https://github.com/Leeri1y/RFID-SoftSPI-Driver/archive/refs/heads/main.zip).
2. Extract the ZIP file to Arduino libraries folder:
   - Windows: `C:\Users\[Your Name]\Documents\Arduino\libraries`
   - Mac: `~/Documents/Arduino/libraries`
3. Restart Arduino IDE, the library will appear in **Sketch > Include Library > RFID-SoftSPI-Driver**.

## 🚀 Quick Start Example
```cpp
#include <Arduino.h>
#include <RFID.h>

// Initialize Software SPI RFID (CS, RST, SCK, MOSI, MISO)
RFID rfid(9, 5, 6, 4, 3);

void setup() {
  Serial.begin(115200);
  rfid.init();  // No need for SPI.begin() (software SPI)
  Serial.println("RFID Soft SPI Ready. Waiting for card...");
}

void loop() {
  if (rfid.isCard()) {
    Serial.println("\n[+] Card Detected");
    
    // Read card serial number
    if (rfid.readCardSerial()) {
      Serial.print("Card UID (HEX): ");
      for (int i = 0; i < 5; i++) {
        if (rfid.serNum[i] < 0x10) Serial.print("0");
        Serial.print(rfid.serNum[i], HEX);
        Serial.print(" ");
      }
    }
    
    rfid.halt();  // Put card into sleep mode
    Serial.println("\n[-] Operation Completed");
  }
  delay(1000);
}
