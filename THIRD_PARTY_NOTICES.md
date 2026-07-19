# Third-party notices

This repository is a GPLv3 derivative of [CarCluster](https://github.com/r00li/CarCluster) by Andrej Rolih and its contributors. Original file headers, Git history, and attribution are retained.

## Bundled source

### ArduinoJson 7.1.0

- Project: <https://arduinojson.org/>
- Copyright: 2014-2024 Benoit Blanchon
- License: MIT
- Location: `CarCluster/src/Libs/ArduinoJson`

### MCP_CAN_lib 1.5.1

- Project: <https://github.com/coryjfowler/MCP_CAN_lib>
- Copyright: 2012 Seeed Technology Inc.; 2017 Cory J. Fowler
- License: GNU Lesser General Public License 2.1 or later
- Location: `CarCluster/src/Libs/MCP_CAN`

### MultiMap 0.2.0

- Project: <https://github.com/RobTillaart/MultiMap>
- Copyright: 2011-2023 Rob Tillaart
- License: MIT
- Location: `CarCluster/src/Libs/MultiMap`

### WiFiManager 2.0.17

- Project: <https://github.com/tzapu/WiFiManager>
- Copyright: 2015 tzapu and contributors
- License: MIT
- Location: `CarCluster/src/Libs/WiFiManager`

## Build-time platform libraries

The firmware links against libraries supplied by the [Arduino core for ESP32](https://github.com/espressif/arduino-esp32), including `WebServer`, `WiFi`, `Network`, `DNSServer`, `AsyncUDP`, `SPI`, and related core components. The relevant Arduino-ESP32 components are distributed under LGPL-2.1-or-later and retain their upstream notices in the installed platform package.

The exact MIT notices are retained in dependency-specific files under `LICENSES/`. The LGPL 2.1 text is available at `LICENSES/LGPL-2.1.txt`. These notices do not replace or alter the GPLv3 license of this combined firmware.
