# BeamNG.drive CCB1 protocol mod

This directory contains the optional BeamNG.drive sender for the extended CarCluster BMW protocol.

## Why this mod exists

BeamNG's standard OutGauge packet is enough for basic speed, RPM, pedals, gear, and a few warning lamps. It does not expose all the information used by this F10/F11 firmware. `carcluster_bmw.lua` sends a versioned UDP packet beginning with the `CCB1` marker and adds:

- coolant and oil temperature;
- fuel ratio, fuel volume, fuel used, fuel flow, and calculated consumption inputs;
- throttle, brake, clutch, and engine load;
- ignition level and mapped BMW drive mode;
- separate low beam, high beam, left indicator, and right indicator states;
- parking brake, ABS/ESC, battery, oil, engine-damage, and overheat warnings;
- individual doors, bonnet, boot, and tire-deflation warnings when the vehicle exposes them;
- cruise-control state and set speed.

Some values depend on the selected BeamNG vehicle. If a vehicle does not expose a particular electric or controller value, that feature may remain inactive.

The ESP32 still accepts standard OutGauge packets as a fallback. Install this mod when you want the extended warnings, fuel/consumption behavior, door/tire states, drive modes, or cruise-control marker.

## Recommended installation

1. Download [`carcluster_bmw_protocol.zip`](carcluster_bmw_protocol.zip). Keep it as a ZIP; do not extract it.
2. Open the BeamNG launcher.
3. Select **Manage User Folder** and then **Open in Explorer**.
4. Open the `mods` directory in that user folder.
5. Copy `carcluster_bmw_protocol.zip` directly into `mods`.
6. Start BeamNG.drive, or press `Ctrl+R` to reload the current vehicle if the game is already running.
7. Open **Options > Other > Protocols**.
8. Set the OutGauge destination address to the ESP32 IP address and the port to `1102`.
9. Enable **Other protocols** or the equivalent custom-protocol option shown by your BeamNG version.
10. Put CarCluster into BeamNG mode and turn on the cluster ignition.

Example destination when the ESP32 is running its own default access point:

```text
Address: 192.168.4.1
Port:    1102
```

The protocol reads the configured OutGauge address and port. If those settings are empty, it falls back to `192.168.4.1:1102`.

The supported user-folder installation is preferable to copying files into the BeamNG game installation. It survives file verification and avoids modifying official game files.

## Unpacked development installation

To inspect or edit the Lua sender, copy the source to:

```text
<BeamNG user folder>\mods\unpacked\carcluster_bmw_protocol\lua\vehicle\protocols\carcluster_bmw.lua
```

After each edit, press `Ctrl+R` in BeamNG to reload the vehicle and protocol.

The source included in the ZIP is:

```text
lua\vehicle\protocols\carcluster_bmw.lua
```

## Troubleshooting

- Confirm that the ESP32 and the PC are on networks that can reach each other.
- Confirm UDP port `1102` is not blocked by Windows Firewall.
- Confirm the configured address is the ESP32 address, not the PC address.
- Reload the vehicle with `Ctrl+R` after installing or changing the protocol.
- If extended features do not react but speed and RPM do, the ESP32 is probably receiving fallback OutGauge rather than `CCB1`.
- If only a particular warning or door state is missing, test another vehicle; BeamNG vehicles expose different controller and electric names.

The custom-protocol system requires BeamNG.drive 0.32 or newer. BeamNG's official protocol documentation describes the same `mods/unpacked/<mod>/lua/vehicle/protocols/` structure.
