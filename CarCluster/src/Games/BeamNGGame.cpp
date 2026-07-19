// ####################################################################################################################
// 
// Code part of CarCluster project by Andrej Rolih. See .ino file more details
// Modified for BMW F10/F11 6WA by WesVoj, 2026. See README.md.
// 
// ####################################################################################################################

#include "BeamNGGame.h"
#include <stddef.h>
#include <string.h>

namespace {
  struct BeamNGExtendedPacket {
    char magic[4];
    uint32_t version;
    uint32_t timeMs;
    float speedKmh;
    float rpm;
    float coolantTempC;
    float oilTempC;
    float fuelRatio;
    float fuelVolumeLiters;
    float fuelUsedLiters;
    float fuelRateLitersPerHour;
    float throttle;
    float brake;
    float clutch;
    float engineLoad;
    uint32_t gear;
    uint32_t ignitionLevel;
    uint32_t lowBeam;
    uint32_t highBeam;
    uint32_t leftSignal;
    uint32_t rightSignal;
    uint32_t parkingBrake;
    uint32_t absActive;
    uint32_t escActive;
    uint32_t batteryWarning;
    uint32_t oilWarning;
    uint32_t driveMode;
    float outdoorTempC;
    uint32_t doorFrontLeft;
    uint32_t doorFrontRight;
    uint32_t doorRearLeft;
    uint32_t doorRearRight;
    uint32_t doorDetailsAvailable;
    uint32_t doorAnyOpen;
    uint32_t tireWarningFrontLeft;
    uint32_t tireWarningFrontRight;
    uint32_t tireWarningRearLeft;
    uint32_t tireWarningRearRight;
    uint32_t doorBonnet;
    uint32_t doorBoot;
    uint32_t engineDamageWarning;
    // Appended fields (backward compatible: older Lua sends a shorter packet and
    // these stay zero-initialised). Keep these at the END so existing offsets hold.
    uint32_t cruiseControlActive;
    float cruiseControlSetSpeedKmh;
    uint32_t oilLevelWarning;
    uint32_t engineOverheatWarning;
  };

  const size_t kBeamNGExtendedBasePacketSize = offsetof(BeamNGExtendedPacket, tireWarningFrontLeft);

  float positiveFloat(float value) {
    return value > 0.0f ? value : 0.0f;
  }

  float clampFloat(float value, float minValue, float maxValue) {
    if (value < minValue) {
      return minValue;
    }
    if (value > maxValue) {
      return maxValue;
    }
    return value;
  }

  GearState mapBeamGear(int beamGear) {
    if (beamGear == 255) {
      return GearState_Auto_P;
    }
    if (beamGear == 0) {
      return GearState_Auto_R;
    }
    if (beamGear == 1) {
      return GearState_Auto_N;
    }
    if (beamGear == 254) {
      return GearState_Auto_S;
    }
    if (beamGear >= 10) {
      return GearState_Auto_D;
    }
    return static_cast<GearState>(beamGear - 1);
  }

  void updateFuelConsumption(GameState& gameState, float speedKmh, float fuelUsedLiters, float fuelRateLitersPerHour) {
    gameState.fuelUsedLiters = positiveFloat(fuelUsedLiters);
    gameState.fuelRateLitersPerHour = positiveFloat(fuelRateLitersPerHour);
    gameState.hasFuelConsumptionData = true;

    if (gameState.fuelRateLitersPerHour <= 0.001f) {
      gameState.fuelConsumptionLPer100Km = 0.0f;
    } else if (speedKmh > 1.0f) {
      gameState.fuelConsumptionLPer100Km = clampFloat((gameState.fuelRateLitersPerHour * 100.0f) / speedKmh, 0.0f, 99.9f);
    } else {
      gameState.fuelConsumptionLPer100Km = 99.9f;
    }
  }
}

BeamNGGame::BeamNGGame(GameState& game, int port): Game(game) {
  this->port = port;
}

void BeamNGGame::begin() {
  // Beam NG sends data as a UDP blob of data
  // Telemetry protocol described here: https://github.com/fuelsoft/out-gauge-cluster

  if (beamUdp.listen(port)) {
    beamUdp.onPacket([this](AsyncUDPPacket packet) {
      if (packet.length() >= kBeamNGExtendedBasePacketSize && memcmp(packet.data(), "CCB1", 4) == 0) {
        BeamNGExtendedPacket data = {};
        size_t copyLength = packet.length();
        if (copyLength > sizeof(data)) {
          copyLength = sizeof(data);
        }
        memcpy(&data, packet.data(), copyLength);

        if (data.version == 1) {
          lastExtendedPacketMillis = millis();
          gameState.gear = mapBeamGear((int)data.gear);
          gameState.speed = (int)(positiveFloat(data.speedKmh) + 0.5f);
          gameState.rpm = (int)(positiveFloat(data.rpm) + 0.5f);
          gameState.coolantTemperature = (int)(data.coolantTempC + 0.5f);
          gameState.oilTemperature = (int)(data.oilTempC + 0.5f);
          gameState.outdoorTemperature = (int)(clampFloat(data.outdoorTempC, -40.0f, 85.0f) + 0.5f);
          gameState.fuelQuantity = constrain((int)(clampFloat(data.fuelRatio, 0.0f, 1.0f) * 100.0f + 0.5f), 0, 100);
          gameState.fuelVolumeLiters = positiveFloat(data.fuelVolumeLiters);
          gameState.throttleInput = clampFloat(data.throttle, 0.0f, 1.0f);
          gameState.brakeInput = clampFloat(data.brake, 0.0f, 1.0f);
          gameState.engineLoad = clampFloat(data.engineLoad, 0.0f, 1.0f);
          updateFuelConsumption(gameState, positiveFloat(data.speedKmh), data.fuelUsedLiters, data.fuelRateLitersPerHour);

          gameState.ignitionLevel = constrain((int)data.ignitionLevel, 0, 3);
          gameState.ignition = (gameState.ignitionLevel > 0);
          if (data.driveMode >= 1 && data.driveMode <= 7) {
            gameState.driveMode = (uint8_t)data.driveMode;
          }
          gameState.mainLights = (data.lowBeam != 0);
          gameState.highBeam = (data.highBeam != 0);
          gameState.leftTurningIndicator = (data.leftSignal != 0);
          gameState.rightTurningIndicator = (data.rightSignal != 0);
          if (gameState.leftTurningIndicator) {
            lastLeftSignalMillis = millis();
          }
          if (gameState.rightTurningIndicator) {
            lastRightSignalMillis = millis();
          }
          gameState.handbrake = (data.parkingBrake != 0);
          gameState.doorFrontLeft = (data.doorFrontLeft != 0);
          gameState.doorFrontRight = (data.doorFrontRight != 0);
          gameState.doorRearLeft = (data.doorRearLeft != 0);
          gameState.doorRearRight = (data.doorRearRight != 0);
          gameState.doorDetailsAvailable = (data.doorDetailsAvailable != 0);
          gameState.doorOpen = (data.doorAnyOpen != 0);
          gameState.tireWarningFrontLeft = (data.tireWarningFrontLeft != 0);
          gameState.tireWarningFrontRight = (data.tireWarningFrontRight != 0);
          gameState.tireWarningRearLeft = (data.tireWarningRearLeft != 0);
          gameState.tireWarningRearRight = (data.tireWarningRearRight != 0);
          gameState.doorBonnet = (data.doorBonnet != 0);
          gameState.doorBoot = (data.doorBoot != 0);
          gameState.engineDamageWarning = (data.engineDamageWarning != 0);
          gameState.oilLevelWarning = (data.oilWarning != 0 || data.oilLevelWarning != 0);
          gameState.engineOverheatWarning = (data.engineOverheatWarning != 0);
          gameState.absLight = (data.absActive != 0);
          gameState.offroadLight = (data.escActive != 0);
          gameState.batteryLight = (data.batteryWarning != 0);
          gameState.cruiseControlActive = (data.cruiseControlActive != 0);
          gameState.cruiseControlSetSpeed = (int)(positiveFloat(data.cruiseControlSetSpeedKmh) + 0.5f);
        }
        return;
      }

      if (packet.length() >= 64) {
        if (lastExtendedPacketMillis != 0 && millis() - lastExtendedPacketMillis < 1000) {
          return;
        }

        float floatValue = 0.0f;
        uint32_t uint32Value = 0;

        // GEAR
        int beamGear = (int)(0xFF & packet.data()[10]);
        gameState.gear = mapBeamGear(beamGear);

        // SPEED
        memcpy(&floatValue, (packet.data() + 12), 4);
        gameState.speed = (int)(floatValue * 3.6f + 0.5f); // Speed is in m/s

        // CURRENT_ENGINE_RPM
        memcpy(&floatValue, (packet.data() + 16), 4);
        gameState.rpm = (int)(floatValue + 0.5f);

        // ENGINE TEMPERATURE
        memcpy(&floatValue, (packet.data() + 24), 4);
        gameState.coolantTemperature = (int)(floatValue + 0.5f);
        gameState.oilTemperature = 0;
        gameState.oilLevelWarning = false;
        gameState.engineOverheatWarning = gameState.coolantTemperature >= 120;

        // FUEL
        memcpy(&floatValue, (packet.data() + 28), 4);
        gameState.fuelQuantity = constrain((int)(floatValue * 100.0f + 0.5f), 0, 100);
        gameState.fuelVolumeLiters = 0.0f;
        gameState.fuelUsedLiters = 0.0f;
        gameState.fuelRateLitersPerHour = 0.0f;
        gameState.fuelConsumptionLPer100Km = 0.0f;
        gameState.hasFuelConsumptionData = false;
        gameState.brakeInput = 0.0f;

        // LIGHTS
        memcpy(&uint32Value, (packet.data() + 44), 4);
        uint32_t lights = uint32Value;

        unsigned long nowMs = millis();
        if ((lights & 0x0020) != 0) {
          lastLeftSignalMillis = nowMs;
        }
        if ((lights & 0x0040) != 0) {
          lastRightSignalMillis = nowMs;
        }

        gameState.leftTurningIndicator = (lastLeftSignalMillis != 0 && nowMs - lastLeftSignalMillis < 900);
        gameState.rightTurningIndicator = (lastRightSignalMillis != 0 && nowMs - lastRightSignalMillis < 900);
        gameState.highBeam = ((lights & 0x0002) != 0);
        gameState.mainLights = ((lights & 0x0800) != 0);
        gameState.batteryLight = ((lights & 0x0200) != 0);
        gameState.absLight = ((lights & 0x0400) != 0);
        gameState.handbrake = ((lights & 0x0004) != 0);
        gameState.offroadLight = ((lights & 0x0010) != 0);
        gameState.ignition = (gameState.rpm > 50 || (lights & 0x0300) != 0);
        gameState.ignitionLevel = gameState.ignition ? 3 : 0;
      }
    });
  }
}
