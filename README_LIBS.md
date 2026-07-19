# Bundled and platform libraries

The following libraries are bundled so the Arduino sketch can be built without installing them separately:

| Library | Bundled version | License |
| --- | --- | --- |
| [MCP_CAN_lib](https://github.com/coryjfowler/MCP_CAN_lib) | 1.5.1 | LGPL-2.1-or-later |
| [ArduinoJson](https://arduinojson.org/) | 7.1.0 | MIT |
| [WiFiManager](https://github.com/tzapu/WiFiManager) | 2.0.17 | MIT |
| [MultiMap](https://github.com/RobTillaart/MultiMap) | 0.2.0 | MIT |

The HTTP dashboard uses the `WebServer` library supplied by the [Arduino core for ESP32](https://github.com/espressif/arduino-esp32), licensed under LGPL-2.1-or-later. The project does not bundle or use Mongoose.

Copyright notices and license details are collected in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) and the `LICENSES` directory. The project itself remains licensed under GPLv3 as required by the upstream CarCluster project.
