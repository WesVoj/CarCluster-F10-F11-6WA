// ####################################################################################################################
//
// Code part of CarCluster project by Andrej Rolih. See .ino file more details
// Modified for BMW F10/F11 6WA by WesVoj, 2026. See README.md.
//
// ####################################################################################################################

#include "BMWFSeriesCluster.h"
#include <time.h>

BMWFSeriesCluster::BMWFSeriesCluster(MCP_CAN& CAN, bool isCarMini): CAN(CAN) {
  this->isCarMini = isCarMini;

  if (isCarMini) {
    inFuelRange[0] = 0; inFuelRange[1] = 50; inFuelRange[2] = 100;
    outFuelRange[0] = 22; outFuelRange[1] = 7; outFuelRange[2] = 3;
  } else {
    inFuelRange[0] = 0; inFuelRange[1] = 50; inFuelRange[2] = 100;
    outFuelRange[0] = 37; outFuelRange[1] = 18; outFuelRange[2] = 4;
  }
  crc8Calculator.begin();
}

BMWFExperimentalOptions BMWFSeriesCluster::getExperimentalOptions() const {
  BMWFExperimentalOptions options;
  options.languageAndUnits291 = languageAndUnits291Enabled;
  options.enhancedLanguage291 = enhancedLanguage291Enabled;
  options.dateTime2F8 = dateTime2F8Enabled;
  options.time39E = time39EEnabled;
  options.time39ESource = time39ESourceByte;
  options.time39EFormat = time39EFormat;
  options.time3F1 = time3F1Enabled;
  options.fastRPMRefresh = fastRPMRefreshEnabled;
  options.highResolutionRPM = highResolutionRPMEnabled;
  options.lim287 = lim287Enabled;
  options.lim289Icon = lim289IconEnabled;
  options.outsideTemperature2CA = outsideTemperature2CAEnabled;
  options.autoStartStop30B = autoStartStop30BEnabled;
  options.autoHold5C0 = autoHold5C0Enabled;
  options.tpmsWarningFL5C0 = tpmsWarningFL5C0Enabled;
  options.tpmsWarningFR5C0 = tpmsWarningFR5C0Enabled;
  options.tpmsWarningRL5C0 = tpmsWarningRL5C0Enabled;
  options.tpmsWarningRR5C0 = tpmsWarningRR5C0Enabled;
  options.tpmsCandidate31C = tpmsCandidate31CEnabled;
  options.tpmsCandidate31F = tpmsCandidate31FEnabled;
  options.tpmsCandidateB68 = tpmsCandidateB68Enabled;
  options.laneAssistAlwaysOn = laneAssistAlwaysOnEnabled;
  options.laneSweep = laneSweepEnabled;
  return options;
}

void BMWFSeriesCluster::setExperimentalOptions(const BMWFExperimentalOptions& options) {
  bool changed =
      languageAndUnits291Enabled != options.languageAndUnits291 ||
      enhancedLanguage291Enabled != options.enhancedLanguage291 ||
      dateTime2F8Enabled != options.dateTime2F8 ||
      time39EEnabled != options.time39E ||
      time39ESourceByte != options.time39ESource ||
      time39EFormat != options.time39EFormat ||
      time3F1Enabled != options.time3F1 ||
      fastRPMRefreshEnabled != options.fastRPMRefresh ||
      highResolutionRPMEnabled != options.highResolutionRPM ||
      lim287Enabled != options.lim287 ||
      lim289IconEnabled != options.lim289Icon ||
      outsideTemperature2CAEnabled != options.outsideTemperature2CA ||
      autoStartStop30BEnabled != options.autoStartStop30B ||
      autoHold5C0Enabled != options.autoHold5C0 ||
      tpmsWarningFL5C0Enabled != options.tpmsWarningFL5C0 ||
      tpmsWarningFR5C0Enabled != options.tpmsWarningFR5C0 ||
      tpmsWarningRL5C0Enabled != options.tpmsWarningRL5C0 ||
      tpmsWarningRR5C0Enabled != options.tpmsWarningRR5C0 ||
      tpmsCandidate31CEnabled != options.tpmsCandidate31C ||
      tpmsCandidate31FEnabled != options.tpmsCandidate31F ||
      tpmsCandidateB68Enabled != options.tpmsCandidateB68;

  languageAndUnits291Enabled = options.languageAndUnits291;
  enhancedLanguage291Enabled = false;
  dateTime2F8Enabled = options.dateTime2F8;
  time39EEnabled = options.time39E;
  time39ESourceByte = options.time39ESource;
  time39EFormat = options.time39EFormat <= 4 ? options.time39EFormat : 0;
  time3F1Enabled = options.time3F1;
  fastRPMRefreshEnabled = options.fastRPMRefresh;
  highResolutionRPMEnabled = true;
  lim287Enabled = options.lim287;
  lim289IconEnabled = options.lim289Icon;
  outsideTemperature2CAEnabled = options.outsideTemperature2CA;
  autoStartStop30BEnabled = options.autoStartStop30B;
  autoHold5C0Enabled = options.autoHold5C0;
  tpmsWarningFL5C0Enabled = options.tpmsWarningFL5C0;
  tpmsWarningFR5C0Enabled = options.tpmsWarningFR5C0;
  tpmsWarningRL5C0Enabled = options.tpmsWarningRL5C0;
  tpmsWarningRR5C0Enabled = options.tpmsWarningRR5C0;
  tpmsCandidate31CEnabled = options.tpmsCandidate31C;
  tpmsCandidate31FEnabled = options.tpmsCandidate31F;
  tpmsCandidateB68Enabled = options.tpmsCandidateB68;
  // Lane-assist toggle is independent of the date/time/language state, so it is
  // intentionally not part of the `changed` recompute above.
  laneAssistAlwaysOnEnabled = options.laneAssistAlwaysOn;
  // Restart the seed sweep from 0x00 whenever it is freshly switched on.
  if (options.laneSweep && !laneSweepEnabled) {
    laneSweepSeed = 0x00;
    laneSweepLastMs = 0;
  }
  laneSweepEnabled = options.laneSweep;

  if (changed) {
    hasSentDateTime = false;
    hasSentLanguageAndUnits = false;
    lastDashboardUpdateTimeRPM = 0;
    lastDashboardUpdateTimeSpeed = 0;
    lastDashboardUpdateTimeBlinkers = 0;
    lastDashboardUpdateTimeLights = 0;
    lastDashboardUpdateTimeWakeUp = 0;
    lastDashboardUpdateTimeDateTime = 0;
    lastDashboardUpdateTimeCandidates = 0;
    lastDashboardUpdateTimeLanguageAndUnits = 0;
  }
}

BMWFTestOptions BMWFSeriesCluster::getTestOptions() const {
  BMWFTestOptions options;
  options.laneAssist327 = testLaneAssist327Enabled;
  options.laneAssistScanIds = testLaneAssistScanIdsEnabled;
  options.laneAssistId = testLaneAssistId;
  options.laneAssistLeft = testLaneAssistLeft;
  options.laneAssistRight = testLaneAssistRight;
  options.laneAssistCrcSeed = testLaneAssistCrcSeed;
  options.frame1D6ButtonMode = testFrame1D6ButtonMode;
  options.frame327RawA214 = testFrame327RawA214Enabled;
  options.frame349SplitFuel = testFrame349SplitFuelEnabled;
  options.frame349FuelLeft = testFrame349FuelLeft;
  options.frame349FuelRight = testFrame349FuelRight;
  options.frame349FuelDlc = testFrame349FuelDlc;
  options.frame33BRawAcc = testFrame33BRawAccEnabled;
  options.frame21A12Dlc8 = testFrame21A12Dlc8Enabled;
  options.frame2E4SteeringButton = testFrame2E4SteeringButtonEnabled;
  options.frame1EERawLeftMenu = testFrame1EERawLeftMenuEnabled;
  options.frame393 = testFrame393Enabled;
  options.frame1B3 = testFrame1B3Enabled;
  options.frame2C5 = testFrame2C5Enabled;
  options.frame381 = testFrame381Enabled;
  options.frame2C3 = testFrame2C3Enabled;
  options.frame3A0 = testFrame3A0Enabled;
  options.frame581 = testFrame581Enabled;
  options.frame0AB = testFrame0ABEnabled;
  options.frame130Icm = testFrame130IcmEnabled;
  options.frame0C4Szl = testFrame0C4SzlEnabled;
  options.frame0AAWheelSpeed = testFrame0AAWheelSpeedEnabled;
  options.frame3D0Battery = testFrame3D0BatteryEnabled;
  options.frame368Rdc = testFrame368RdcEnabled;
  options.frame368RdcB1 = testFrame368RdcB1;
  options.frame368RdcB2 = testFrame368RdcB2;
  options.frame368RdcB3 = testFrame368RdcB3;
  options.frame368RdcCrcSeed = testFrame368RdcCrcSeed;
  options.frame369Tpms = testFrame369TpmsEnabled;
  options.frame369TpmsB1 = testFrame369TpmsB1;
  options.frame369TpmsB2 = testFrame369TpmsB2;
  options.frame369TpmsB3 = testFrame369TpmsB3;
  options.cc50TyreMonitoringFailure = testCC50TyreMonitoringFailureEnabled;
  options.cc63TyreFailure = testCC63TyreFailureEnabled;
  options.cc144TPMSFault = testCC144TPMSFaultEnabled;
  options.cc145TPMSDeactivated = testCC145TPMSDeactivatedEnabled;
  options.cc147TyrePressureLoss = testCC147TyrePressureLossEnabled;
  options.cc149TPMSFailure = testCC149TPMSFailureEnabled;
  options.cc192TPMSInitialising = testCC192TPMSInitialisingEnabled;
  options.cc28OilLevel = testCC28OilLevelEnabled;
  options.cc36DscOff = testCC36DscOffEnabled;
  options.cc39EngineOverheat = testCC39EngineOverheatEnabled;
  options.time39E6waCandidate = testTime39E6waCandidateEnabled;
  options.clearCC167ResetClock = testClearCC167ResetClockEnabled;
  options.cc299ChassisWarning = testCC299ChassisWarningEnabled;
  options.cc196SosWarning = testCC196SosWarningEnabled;
  options.rawCCActive = testRawCCActiveEnabled;
  options.rawCCId = testRawCCId;
  return options;
}

void BMWFSeriesCluster::setTestOptions(const BMWFTestOptions& options) {
  testLaneAssist327Enabled = options.laneAssist327;
  testLaneAssistScanIdsEnabled = options.laneAssistScanIds;
  testLaneAssistId = options.laneAssistId <= 0x7FF ? options.laneAssistId : 0x327;
  testLaneAssistLeft = options.laneAssistLeft;
  testLaneAssistRight = options.laneAssistRight;
  testLaneAssistCrcSeed = options.laneAssistCrcSeed;
  testFrame1D6ButtonMode = options.frame1D6ButtonMode <= 2 ? options.frame1D6ButtonMode : 0;
  testFrame327RawA214Enabled = options.frame327RawA214;
  testFrame349SplitFuelEnabled = options.frame349SplitFuel;
  testFrame349FuelLeft = options.frame349FuelLeft;
  testFrame349FuelRight = options.frame349FuelRight;
  testFrame349FuelDlc = options.frame349FuelDlc == 4 || options.frame349FuelDlc == 5 || options.frame349FuelDlc == 8
      ? options.frame349FuelDlc
      : 8;
  testFrame33BRawAccEnabled = options.frame33BRawAcc;
  testFrame21A12Dlc8Enabled = options.frame21A12Dlc8;
  testFrame2E4SteeringButtonEnabled = options.frame2E4SteeringButton;
  testFrame1EERawLeftMenuEnabled = options.frame1EERawLeftMenu;
  testFrame393Enabled = options.frame393;
  testFrame1B3Enabled = options.frame1B3;
  testFrame2C5Enabled = options.frame2C5;
  testFrame381Enabled = options.frame381;
  testFrame2C3Enabled = options.frame2C3;
  testFrame3A0Enabled = options.frame3A0;
  testFrame581Enabled = options.frame581;
  testFrame0ABEnabled = options.frame0AB;
  testFrame130IcmEnabled = options.frame130Icm;
  testFrame0C4SzlEnabled = options.frame0C4Szl;
  testFrame0AAWheelSpeedEnabled = options.frame0AAWheelSpeed;
  testFrame3D0BatteryEnabled = options.frame3D0Battery;
  testFrame368RdcEnabled = options.frame368Rdc;
  testFrame368RdcB1 = options.frame368RdcB1;
  testFrame368RdcB2 = options.frame368RdcB2;
  testFrame368RdcB3 = options.frame368RdcB3;
  testFrame368RdcCrcSeed = options.frame368RdcCrcSeed;
  testFrame369TpmsEnabled = options.frame369Tpms;
  testFrame369TpmsB1 = options.frame369TpmsB1;
  testFrame369TpmsB2 = options.frame369TpmsB2;
  testFrame369TpmsB3 = options.frame369TpmsB3;
  testCC50TyreMonitoringFailureEnabled = options.cc50TyreMonitoringFailure;
  testCC63TyreFailureEnabled = options.cc63TyreFailure;
  testCC144TPMSFaultEnabled = options.cc144TPMSFault;
  testCC145TPMSDeactivatedEnabled = options.cc145TPMSDeactivated;
  testCC147TyrePressureLossEnabled = options.cc147TyrePressureLoss;
  testCC149TPMSFailureEnabled = options.cc149TPMSFailure;
  testCC192TPMSInitialisingEnabled = options.cc192TPMSInitialising;
  testCC28OilLevelEnabled = options.cc28OilLevel;
  testCC36DscOffEnabled = options.cc36DscOff;
  testCC39EngineOverheatEnabled = options.cc39EngineOverheat;
  testTime39E6waCandidateEnabled = options.time39E6waCandidate;
  testClearCC167ResetClockEnabled = options.clearCC167ResetClock;
  testCC299ChassisWarningEnabled = options.cc299ChassisWarning;
  testCC196SosWarningEnabled = options.cc196SosWarning;
  testRawCCActiveEnabled = options.rawCCActive;
  testRawCCId = options.rawCCId;
}

uint8_t BMWFSeriesCluster::mapGenericGearToLocalGear(GearState inputGear) {
  // The gear that the car is in: 0 = clear, 1-9 = M1-M9, 10 = P, 11 = R, 12 = N, 13 = D

  switch(inputGear) {
    case GearState_Manual_1: return 1; break;
    case GearState_Manual_2: return 2; break;
    case GearState_Manual_3: return 3; break;
    case GearState_Manual_4: return 4; break;
    case GearState_Manual_5: return 5; break;
    case GearState_Manual_6: return 6; break;
    case GearState_Manual_7: return 7; break;
    case GearState_Manual_8: return 8; break;
    case GearState_Manual_9: return 9; break;
    case GearState_Manual_10: return 13; break;
    case GearState_Auto_P: return 10; break;
    case GearState_Auto_R: return 11; break;
    case GearState_Auto_N: return 12; break;
    case GearState_Auto_D: return 13; break;
    case GearState_Auto_S: return 13; break;
  }
  return 0;
}

int BMWFSeriesCluster::mapSpeed(GameState& game) {
  int scaledSpeed = game.speed * game.configuration.speedCorrectionFactor;
  if (scaledSpeed < 0) {
    return 0;
  } else if (scaledSpeed > game.configuration.maximumSpeedValue) {
    return game.configuration.maximumSpeedValue;
  } else {
    return scaledSpeed;
  }
}

int BMWFSeriesCluster::mapRPM(GameState& game) {
  int scaledRPM = game.rpm * game.configuration.rpmCorrectionFactor;
  if (scaledRPM < 0) {
    return 0;
  } else if (scaledRPM > game.configuration.maximumRPMValue) {
    return game.configuration.maximumRPMValue;
  } else {
    return scaledRPM;
  }
}

int BMWFSeriesCluster::mapCoolantTemperature(GameState& game) {
  if (game.coolantTemperature < game.configuration.minimumCoolantTemperature) { return game.configuration.minimumCoolantTemperature; }
  if (game.coolantTemperature > game.configuration.maximumCoolantTemperature) { return game.configuration.maximumCoolantTemperature; }
  return game.coolantTemperature;
}

void BMWFSeriesCluster::updateWithGame(GameState& game) {
  unsigned long nowMs = millis();
  uint8_t ignitionLevel = game.ignition ? game.ignitionLevel : 0;
  if (game.ignition && ignitionLevel == 0) {
    ignitionLevel = 3;
  } else if (ignitionLevel > 3) {
    ignitionLevel = 3;
  }

  bool wakeImmediately = lastIgnitionLevel == 0 && ignitionLevel > 0;
  if (wakeImmediately || lastDashboardUpdateTimeWakeUp == 0 || nowMs - lastDashboardUpdateTimeWakeUp >= dashboardUpdateTimeWakeUp) {
    sendWakeUp();
    lastDashboardUpdateTimeWakeUp = nowMs;
  }

  if (ignitionLevel == 0) {
    if (lastDashboardUpdateTime == 0 || nowMs - lastDashboardUpdateTime >= dashboardUpdateTime100) {
      sendIgnitionStatus(0);
      if (autoStartStop30BEnabled) {
        sendAutoStartStop(0);
      }
      if (autoHold5C0Latched) {
        sendAutoHold(false);
      }
      counter4Bit++;
      if (counter4Bit >= 14) { counter4Bit = 0; }
      lastDashboardUpdateTime = nowMs;
    }
    lastIgnitionLevel = ignitionLevel;
    return;
  }

  float needlePosition = getNeedleSweepPosition(nowMs);
  bool sweepingNeedles = needlePosition >= 0.0f;
  int displaySpeed = sweepingNeedles
      ? (int)((float)game.configuration.maximumSpeedValue * needlePosition + 0.5f)
      : mapSpeed(game);
  int displayRPM = sweepingNeedles
      ? (int)((float)game.configuration.maximumRPMValue * needlePosition + 0.5f)
      : mapRPM(game);
  int displayCoolantTemperature = mapCoolantTemperature(game);
  int displayFuelQuantity = game.fuelQuantity;
  if (sweepingNeedles) {
    int temperatureRange = game.configuration.maximumCoolantTemperature - game.configuration.minimumCoolantTemperature;
    displayCoolantTemperature = game.configuration.minimumCoolantTemperature + (int)((float)temperatureRange * needlePosition + 0.5f);
    displayFuelQuantity = (int)(100.0f * needlePosition + 0.5f);
  }

  if (fastRPMRefreshEnabled && (lastDashboardUpdateTimeRPM == 0 || nowMs - lastDashboardUpdateTimeRPM >= dashboardUpdateTimeRPM)) {
    sendRPM(displayRPM, mapGenericGearToLocalGear(game.gear));
    lastDashboardUpdateTimeRPM = nowMs;
  }

  if (lastDashboardUpdateTimeSpeed == 0 || nowMs - lastDashboardUpdateTimeSpeed >= dashboardUpdateTimeSpeed) {
    sendSpeed(displaySpeed);
    if (testClearCC167ResetClockEnabled) {
      sendTestCCAlert(167, false);
    }
    lastDashboardUpdateTimeSpeed = nowMs;
  }

  if (lastDashboardUpdateTimeBlinkers == 0 || nowMs - lastDashboardUpdateTimeBlinkers >= dashboardUpdateTimeBlinkers) {
    sendBlinkers(game.leftTurningIndicator, game.rightTurningIndicator);
    lastDashboardUpdateTimeBlinkers = nowMs;
  }

  if (lastDashboardUpdateTimeLights == 0 || nowMs - lastDashboardUpdateTimeLights >= dashboardUpdateTimeLights) {
    sendLights(game.mainLights, game.highBeam, game.rearFogLight, game.frontFogLight);
    lastDashboardUpdateTimeLights = nowMs;
  }

  // Date/time broadcasts (0x2F8 / 0x39E / 0x3F1) were removed: this 6WA/MOST
  // cluster gets its time from the head unit over MOST, not from K-CAN, so these
  // frames never set the clock and are no longer sent.

  if (nowMs - lastDashboardUpdateTime >= dashboardUpdateTime100) {
    // This should probably be done using a more sophisticated method like a
    // scheduler, but for now this seems to work.

    sendIgnitionStatus(ignitionLevel);
    if (!fastRPMRefreshEnabled) {
      sendRPM(displayRPM, mapGenericGearToLocalGear(game.gear));
    }
    sendBasicDriveInfo(displayCoolantTemperature);
    sendAutomaticTransmission(mapGenericGearToLocalGear(game.gear));
    sendFuel(displayFuelQuantity, inFuelRange, outFuelRange, isCarMini);
    sendParkBrake(game.handbrake);
    sendDistanceTravelled(game, mapSpeed(game));
    sendAlerts(game, game.offroadLight, game.handbrake, isCarMini);
    sendDriveMode(game.driveMode);
    if (!testFrame33BRawAccEnabled) {
      sendAcc();
    }
    sendTPMSWarnings(
      tpmsWarningFL5C0Enabled || game.tireWarningFrontLeft,
      tpmsWarningFR5C0Enabled || game.tireWarningFrontRight,
      tpmsWarningRL5C0Enabled || game.tireWarningRearLeft,
      tpmsWarningRR5C0Enabled || game.tireWarningRearRight
    );
    sendTPMSCandidateFrames();
    sendTestFrames();

    // Persistent lane-departure lines: send KAFAS 0x327 every 100 ms with the
    // rolling alive counter so the KOMBI keeps the lane graphic lit (provided
    // its coding has TLC_VERBAUT = aktiv). Independent of the /test toggles.
    if ((laneAssistAlwaysOnEnabled || laneSweepEnabled) && !testFrame327RawA214Enabled) {
      sendLaneAssistCandidate(testLaneAssistId);
    }
    // CRC-seed sweep: hold each candidate seed for ~1.5 s and log the active one
    // so the winning seed can be read off the Serial Monitor when the lane lines
    // appear on the cluster. Wraps 0xFF -> 0x00 and keeps going.
    if (laneSweepEnabled) {
      const unsigned long laneSweepIntervalMs = 1500;
      bool announce = false;
      if (laneSweepLastMs == 0) {
        laneSweepLastMs = nowMs;
        announce = true;
      } else if (nowMs - laneSweepLastMs >= laneSweepIntervalMs) {
        laneSweepSeed++;  // uint8_t wraps 0xFF -> 0x00
        laneSweepLastMs = nowMs;
        announce = true;
      }
      if (announce) {
        Serial.print("BMW_F LANE SWEEP 0x");
        Serial.print(testLaneAssistId, HEX);
        Serial.print(" seed=0x");
        if (laneSweepSeed < 0x10) { Serial.print('0'); }
        Serial.print(laneSweepSeed, HEX);
        Serial.print(" left=0x");
        if (testLaneAssistLeft < 0x10) { Serial.print('0'); }
        Serial.print(testLaneAssistLeft, HEX);
        Serial.print(" right=0x");
        if (testLaneAssistRight < 0x10) { Serial.print('0'); }
        Serial.print(testLaneAssistRight, HEX);
        Serial.println();
      }
    }

    // Cruise control: green set-speed icon + speedo-ring marker from BeamNG.
    // Takes priority over the fixed-LIM test (both drive the 0x287 ring frame).
    sendCruiseControl(game.cruiseControlActive, game.cruiseControlSetSpeed);
    if (!game.cruiseControlActive && lim287Enabled) {
      sendFixedLIM(110);
    }
    // Generic repeated raw test frame (serial action 14) for marker hunting.
    if (rawTestEnabled) {
      CAN.sendMsgBuf(rawTestId, 0, 8, rawTestData);
    }
    if (autoStartStop30BEnabled) {
      sendAutoStartStop(ignitionLevel);
    }
    if (autoHold5C0Enabled || autoHold5C0Latched) {
      bool autoHoldActive = autoHold5C0Enabled && mapSpeed(game) <= 1 && game.brakeInput > 0.08f;
      sendAutoHold(autoHoldActive);
    }

    counter4Bit++;
    if (counter4Bit >= 14) { counter4Bit = 0; }

    count++;
    if (count >= 254) { count = 0; } // Needs to be reset at 254 not 255

    lastDashboardUpdateTime = nowMs;
  }

  if (nowMs - lastDashboardUpdateTime1000ms >= dashboardUpdateTime1000) {
    sendBacklightBrightness(game.backlightBrightness);
    if (outsideTemperature2CAEnabled) {
      sendOutsideTemperature(game.outdoorTemperature);
    }

    if (!testFrame1EERawLeftMenuEnabled) {
      sendSteeringWheelButton(game.buttonEventToProcess);
    }
    if (game.buttonEventToProcess != 0) {
      game.buttonEventToProcess = 0;
    }

    lastDashboardUpdateTime1000ms = nowMs;
  }

#if BMW_F_SYNC_LANGUAGE_AND_UNITS
  if (languageAndUnits291Enabled && (!hasSentLanguageAndUnits || nowMs - lastDashboardUpdateTimeLanguageAndUnits >= dashboardUpdateTimeLanguageAndUnits)) {
    updateLanguageAndUnits();
    hasSentLanguageAndUnits = true;
    lastDashboardUpdateTimeLanguageAndUnits = nowMs;
  }
#endif

  lastIgnitionLevel = ignitionLevel;
}

void BMWFSeriesCluster::sendWakeUp() {
  // ZGW/SGM NM alive (node 0x10)
  unsigned char wakeUp[] = { 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x10 };
  CAN.sendMsgBuf(0x510, 0, 8, wakeUp);

  // CIC/HU NM alive — KOMBI requires this to enter C8 "searching for time" state.
  // Tested: 0x540 (node 0x40) and 0x568 (node 0x68) trigger C8; 0x53B/0x53C do not.
  // Currently using 0x540. If still failing, try 0x568 (change both CAN ID and byte 7).
  // CAN ID 0x540 with byte7=0x3B triggers KOMBI's C8 "searching for time" state.
  // Do NOT change byte7 to 0x40 — that breaks C8 appearance.
  unsigned char cicNM[] = { 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x3B };
  CAN.sendMsgBuf(0x540, 0, 8, cicNM);
}

void BMWFSeriesCluster::sendFixedLIM(uint8_t speedKmh) {
  uint8_t limValue = speedKmh / 5;
  if (limValue < 1 || limValue > 30) {
    return;
  }

  unsigned char limFrame[] = {
    0x05,
    limValue,
    0x10,
    0x10,
    0x00,
    0xFF,
    0xFF,
    0xFF
  };

  CAN.sendMsgBuf(0x287, 0, 8, limFrame);
}

// Cruise control telltale + speedo-ring set-speed marker, both carried on 0x289
// (CRC seed 0x82). This frame must be sent every cycle (the cluster needs 0x289
// alive), so when cruise is off we send the idle keep-alive payload.
//
// Active-marker byte meanings come from F10 CarCluster reverse-engineering notes and
// MAY need tuning against the cluster's hidden test menu:
//   d0 = 0xF0|counter  alive counter
//   d1 = marker colour     0x26 green, 0x20 orange
//   d3 = cruise telltale   0xE3 cruise on (0xE1 idle)
//   d4 = marker type       0x86 cruise (0x88 limiter)
//   d5 = marker mode       0x0C cruise (0x0A limiter, 0x02 flashing)
//   d6 = set-speed marker position (resolution unconfirmed - tune vs test menu)
void BMWFSeriesCluster::sendCruiseControl(bool active, int setSpeedKmh) {
  unsigned char d[7];
  d[0] = 0xF0 | counter4Bit;

  if (cruiseMarkerTestEnabled) {
    // Live marker experiment (set over serial via action 13). Forced on regardless
    // of `active` so BeamNG telemetry can't reset it mid-test. Lets us reverse-
    // engineer the speedo-ring set-speed marker byte-by-byte without reflashing.
    // WARNING: wrong values here can re-trigger "Cruise control failure" - that's
    // expected while sweeping; we're looking for the combination that shows the
    // green marker WITHOUT faulting.
    d[1] = cruiseMarkerTestD1;
    d[2] = cruiseMarkerTestD2;
    d[3] = 0xE3;
    d[4] = cruiseMarkerTestD4;
    d[5] = cruiseMarkerTestD5;
    d[6] = cruiseMarkerTestD6;
  } else if (active) {
    // Cruise engaged. Empirically (serial action-13 testing on this cluster):
    //   d4=0x86 + d5=0x0C is what actually lights the cruise icon (NOT d3 alone),
    //   and it is stable with no "cruise control failure" as long as d1 stays 0xE0
    //   (the fault earlier came from d1=0x26, not these bytes).
    //   d6 carries the set-speed marker position (scale still being calibrated).
    int marker = setSpeedKmh;
    if (marker < 0) marker = 0;
    if (marker > 255) marker = 255;
    d[1] = 0xE0;
    d[2] = 0xE0;
    d[3] = 0xE3;
    d[4] = 0x86;
    d[5] = 0x0C;
    d[6] = (uint8_t)marker;
  } else if (lim289IconEnabled) {
    // Manual Speed-Limiter (LIM) telltale - dashboard toggle. d4=0x88/d5=0x0E is the
    // limiter variant (vs cruise 0x86/0x0C). BeamNG has no limiter, so this is a
    // display-only on/off rather than a live state.
    d[1] = 0xE0;
    d[2] = 0xE0;
    d[3] = 0xE3;
    d[4] = 0x88;
    d[5] = 0x0E;
    d[6] = 0x14;
  } else {
    // Idle keep-alive (cruise available, no telltale/marker).
    d[1] = 0xE0;
    d[2] = 0xE0;
    d[3] = 0xE1;
    d[4] = 0x00;
    d[5] = 0xEC;
    d[6] = 0x01;
  }

  unsigned char frame[8] = {
    crc8Calculator.get_crc8(d, 7, 0x82),
    d[0], d[1], d[2], d[3], d[4], d[5], d[6]
  };
  CAN.sendMsgBuf(0x289, 0, 8, frame);
}

void BMWFSeriesCluster::setCruiseMarkerTest(bool enabled, uint8_t d1, uint8_t d2, uint8_t d4, uint8_t d5, uint8_t d6) {
  cruiseMarkerTestEnabled = enabled;
  cruiseMarkerTestD1 = d1;
  cruiseMarkerTestD2 = d2;
  cruiseMarkerTestD4 = d4;
  cruiseMarkerTestD5 = d5;
  cruiseMarkerTestD6 = d6;
}

void BMWFSeriesCluster::setRawTestFrame(bool enabled, uint16_t id, const uint8_t data[8]) {
  rawTestEnabled = enabled;
  rawTestId = id;
  for (uint8_t i = 0; i < 8; i++) {
    rawTestData[i] = data[i];
  }
}

void BMWFSeriesCluster::setFuelOverride(bool enabled, uint16_t senderValue) {
  fuelOverrideEnabled = enabled;
  fuelOverrideValue = senderValue;
}

void BMWFSeriesCluster::startNeedleSweep() {
  needleSweepActive = true;
  needleSweepStartMs = millis();
  lastDashboardUpdateTimeRPM = 0;
  lastDashboardUpdateTimeSpeed = 0;
  lastDashboardUpdateTime = 0;
}

bool BMWFSeriesCluster::isNeedleSweepActive() const {
  return needleSweepActive;
}

float BMWFSeriesCluster::getNeedleSweepPosition(unsigned long nowMs) {
  if (!needleSweepActive) {
    return -1.0f;
  }

  unsigned long elapsed = nowMs - needleSweepStartMs;
  if (elapsed < needleSweepTravelMs) {
    return (float)elapsed / (float)needleSweepTravelMs;
  }
  if (elapsed < needleSweepTravelMs * 2UL) {
    return 1.0f - ((float)(elapsed - needleSweepTravelMs) / (float)needleSweepTravelMs);
  }
  if (elapsed < needleSweepTravelMs * 2UL + needleSweepZeroHoldMs) {
    return 0.0f;
  }

  needleSweepActive = false;
  return -1.0f;
}

void BMWFSeriesCluster::sendOutsideTemperature(int temperature) {
  temperature = constrain(temperature, -40, 85);
  uint8_t tempByte = (uint8_t)((temperature * 2) + 80);
  unsigned char tempFrame[] = { tempByte, 0xFF };
  CAN.sendMsgBuf(0x2CA, 0, 2, tempFrame);
}

void BMWFSeriesCluster::sendAutoStartStop(uint8_t ignitionLevel) {
  uint8_t autoStartStopValue = ignitionLevel > 0 ? 0x1A : 0xE6;
  unsigned char autoStartStopFrame[] = {
    autoStartStopValue,
    autoStartStopValue,
    autoStartStopValue,
    autoStartStopValue,
    autoStartStopValue,
    autoStartStopValue,
    autoStartStopValue,
    autoStartStopValue
  };

  CAN.sendMsgBuf(0x30B, 0, 8, autoStartStopFrame);
}

void BMWFSeriesCluster::sendAutoHold(bool active) {
  uint8_t autoHoldFrame[] = { 0x40, 58, 0x00, (uint8_t)(active ? 0x29 : 0x28), 0xFF, 0xFF, 0xFF, 0xFF };
  CAN.sendMsgBuf(0x5C0, 0, 8, autoHoldFrame);
  autoHold5C0Latched = active;
}

void BMWFSeriesCluster::sendTPMSWarnings(bool frontLeft, bool frontRight, bool rearLeft, bool rearRight) {
  static bool initialized = false;
  static bool lastFrontLeft = false;
  static bool lastFrontRight = false;
  static bool lastRearLeft = false;
  static bool lastRearRight = false;
  static bool lastGlobal = false;

  if (!initialized || frontLeft != lastFrontLeft) {
    uint8_t message[] = { 0x40, 139, 0x00, (uint8_t)(frontLeft ? 0x29 : 0x28), 0xFF, 0xFF, 0xFF, 0xFF };
    CAN.sendMsgBuf(0x5C0, 0, 8, message);
    lastFrontLeft = frontLeft;
  }
  if (!initialized || frontRight != lastFrontRight) {
    uint8_t message[] = { 0x40, 143, 0x00, (uint8_t)(frontRight ? 0x29 : 0x28), 0xFF, 0xFF, 0xFF, 0xFF };
    CAN.sendMsgBuf(0x5C0, 0, 8, message);
    lastFrontRight = frontRight;
  }
  if (!initialized || rearLeft != lastRearLeft) {
    uint8_t message[] = { 0x40, 141, 0x00, (uint8_t)(rearLeft ? 0x29 : 0x28), 0xFF, 0xFF, 0xFF, 0xFF };
    CAN.sendMsgBuf(0x5C0, 0, 8, message);
    lastRearLeft = rearLeft;
  }
  if (!initialized || rearRight != lastRearRight) {
    uint8_t message[] = { 0x40, 140, 0x00, (uint8_t)(rearRight ? 0x29 : 0x28), 0xFF, 0xFF, 0xFF, 0xFF };
    CAN.sendMsgBuf(0x5C0, 0, 8, message);
    lastRearRight = rearRight;
  }

  bool globalWarning = frontLeft || frontRight || rearLeft || rearRight;
  if (!initialized || globalWarning != lastGlobal) {
    uint8_t message[] = { 0x40, 142, 0x00, (uint8_t)(globalWarning ? 0x29 : 0x28), 0xFF, 0xFF, 0xFF, 0xFF };
    CAN.sendMsgBuf(0x5C0, 0, 8, message);
    lastGlobal = globalWarning;
  }

  initialized = true;
}

void BMWFSeriesCluster::sendTPMSCandidateFrames() {
  if (!tpmsCandidate31CEnabled && !tpmsCandidate31FEnabled && !tpmsCandidateB68Enabled) {
    return;
  }

  unsigned char tpmsWithoutCRC[] = { (uint8_t)(0xF0 | counter4Bit), 0xA2, 0xA0, 0xA0 };
  unsigned char tpmsWithCRC[] = {
    crc8Calculator.get_crc8(tpmsWithoutCRC, 4, 0xC5),
    tpmsWithoutCRC[0],
    tpmsWithoutCRC[1],
    tpmsWithoutCRC[2],
    tpmsWithoutCRC[3]
  };

  if (tpmsCandidate31CEnabled) {
    CAN.sendMsgBuf(0x31C, 0, 5, tpmsWithCRC);
  }
  if (tpmsCandidate31FEnabled) {
    CAN.sendMsgBuf(0x31F, 0, 5, tpmsWithCRC);
  }
  if (tpmsCandidateB68Enabled) {
    CAN.sendMsgBuf(0xB68, 1, 5, tpmsWithCRC);
  }
}

void BMWFSeriesCluster::sendLaneAssistCandidate(uint16_t canId) {
  if (canId > 0x7FF) {
    return;
  }

  // During a sweep the CRC seed is the cycling candidate; otherwise the fixed
  // /test value is used.
  uint8_t crcSeed = laneSweepEnabled ? laneSweepSeed : testLaneAssistCrcSeed;
  unsigned char laneWithoutCRC[] = {
    (uint8_t)(0x50 | counter4Bit),
    testLaneAssistLeft,
    testLaneAssistRight
  };
  unsigned char laneWithCRC[] = {
    crc8Calculator.get_crc8(laneWithoutCRC, 3, crcSeed),
    laneWithoutCRC[0],
    laneWithoutCRC[1],
    laneWithoutCRC[2]
  };
  CAN.sendMsgBuf(canId, 0, 4, laneWithCRC);
}

void BMWFSeriesCluster::sendTestCCAlert(uint8_t id, bool active, uint8_t byte5) {
  uint8_t message[] = { 0x40, id, 0x00, (uint8_t)(active ? 0x29 : 0x28), 0xFF, byte5, 0xFF, 0xFF };
  CAN.sendMsgBuf(0x5C0, 0, 8, message);
}

void BMWFSeriesCluster::sendTestFrames() {
  static const uint16_t laneScanIds[] = { 0x327, 0x345, 0x18A, 0x239, 0x1A6, 0x31B, 0x317, 0x337, 0x347 };
  static bool lastCC50TyreMonitoringFailure = false;
  static bool lastCC63TyreFailure = false;
  static bool lastCC144TPMSFault = false;
  static bool lastCC145TPMSDeactivated = false;
  static bool lastCC147TyrePressureLoss = false;
  static bool lastCC149TPMSFailure = false;
  static bool lastCC192TPMSInitialising = false;
  static bool lastCC28OilLevel = false;
  static bool lastCC36DscOff = false;
  static bool lastCC39EngineOverheat = false;
  static bool lastCC299ChassisWarning = false;
  static bool lastCC196SosWarning = false;
  static bool lastRawCCActive = false;
  static uint8_t lastRawCCId = 147;

  if (testFrame327RawA214Enabled) {
    unsigned char frame[] = { 0xA2, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    CAN.sendMsgBuf(0x327, 0, 8, frame);
  } else {
    if (testLaneAssist327Enabled) {
      sendLaneAssistCandidate(testLaneAssistId);
    }
    if (testLaneAssistScanIdsEnabled) {
      for (uint8_t i = 0; i < sizeof(laneScanIds) / sizeof(laneScanIds[0]); i++) {
        sendLaneAssistCandidate(laneScanIds[i]);
      }
    }
  }

  if (testFrame1D6ButtonMode != testFrame1D6LastButtonMode && testFrame1D6LastButtonMode != 0) {
    unsigned char releaseFrame[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    CAN.sendMsgBuf(0x1D6, 0, 8, releaseFrame);
  }
  if (testFrame1D6ButtonMode != 0) {
    uint8_t buttonValue = testFrame1D6ButtonMode == 1 ? 0xC8 : 0xC4;
    unsigned char pressFrame[] = { buttonValue, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    CAN.sendMsgBuf(0x1D6, 0, 8, pressFrame);
  }
  testFrame1D6LastButtonMode = testFrame1D6ButtonMode;

  if (testFrame2E4SteeringButtonEnabled) {
    unsigned char frame[] = { (uint8_t)(0xF0 | counter4Bit), 0xFF, 0x08, 0xFF };
    CAN.sendMsgBuf(0x2E4, 0, 4, frame);
  }

  if (testFrame1EERawLeftMenuEnabled) {
    unsigned char pressFrame[] = { 0xC4, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    CAN.sendMsgBuf(0x1EE, 0, 8, pressFrame);
  } else if (testFrame1EERawLeftMenuWasEnabled) {
    unsigned char releaseFrame[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    CAN.sendMsgBuf(0x1EE, 0, 8, releaseFrame);
  }
  testFrame1EERawLeftMenuWasEnabled = testFrame1EERawLeftMenuEnabled;

  if (testFrame33BRawAccEnabled) {
    unsigned char frame[] = { 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00 };
    CAN.sendMsgBuf(0x33B, 0, 8, frame);
  }

  if (testFrame393Enabled) {
    unsigned char frame[] = { 0x3E, 0x32, 0x05, 0xFE };
    CAN.sendMsgBuf(0x393, 0, 4, frame);
  }
  if (testFrame1B3Enabled) {
    unsigned char withoutCRC[] = { 0x1E, 0x01, (uint8_t)(0xF0 | counter4Bit), 0x00, 0xFF, 0x80, 0x50 };
    unsigned char withCRC[] = { crc8Calculator.get_crc8(withoutCRC, 7, 0xFF), withoutCRC[0], withoutCRC[1], withoutCRC[2], withoutCRC[3], withoutCRC[4], withoutCRC[5], withoutCRC[6] };
    CAN.sendMsgBuf(0x1B3, 0, 8, withCRC);
  }
  if (testFrame2C5Enabled) {
    unsigned char withoutCRC[] = { (uint8_t)(0xF0 | counter4Bit), 0x00, 0xF0, 0x04, 0x70, 0x08 };
    unsigned char withCRC[] = { crc8Calculator.get_crc8(withoutCRC, 6, 0xD8), withoutCRC[0], withoutCRC[1], withoutCRC[2], withoutCRC[3], withoutCRC[4], withoutCRC[5] };
    CAN.sendMsgBuf(0x2C5, 0, 7, withCRC);
  }
  if (testFrame381Enabled) {
    unsigned char frame[] = { 0x79, 0x20 };
    CAN.sendMsgBuf(0x381, 0, 2, frame);
  }
  if (testFrame2C3Enabled) {
    unsigned char frame[] = { (uint8_t)(counter4Bit & 0x0F), 0x15, 0x0F, 0x00, 0x00, 0x70, 0xFF, 0xFF };
    CAN.sendMsgBuf(0x2C3, 0, 8, frame);
  }
  if (testFrame3A0Enabled) {
    unsigned char frame[] = { 0xFF, 0xFF, 0xC0, 0xFF, 0xFF, 0xFF, 0xF0, 0xFC };
    CAN.sendMsgBuf(0x3A0, 0, 8, frame);
  }
  if (testFrame581Enabled) {
    unsigned char frame[] = { 0x40, 0x4D, 0x00, 0x28, 0xFF, 0xFF, 0xFF, 0xFF };
    CAN.sendMsgBuf(0x581, 0, 8, frame);
  }
  if (testFrame0ABEnabled) {
    unsigned char withoutCRC[] = { (uint8_t)(0x40 | counter4Bit), 0x40, 0x55, 0xFD, 0xFF, 0xFF, 0xFF };
    unsigned char withCRC[] = { crc8Calculator.get_crc8(withoutCRC, 7, 0xFF), withoutCRC[0], withoutCRC[1], withoutCRC[2], withoutCRC[3], withoutCRC[4], withoutCRC[5], withoutCRC[6] };
    CAN.sendMsgBuf(0x0AB, 0, 8, withCRC);
  }
  if (testFrame130IcmEnabled) {
    unsigned char frame[] = { (uint8_t)(0xF0 | counter4Bit), 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00 };
    CAN.sendMsgBuf(0x130, 0, 8, frame);
  }
  if (testFrame0C4SzlEnabled) {
    unsigned char frame[] = { (uint8_t)(0xF0 | counter4Bit), 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    CAN.sendMsgBuf(0x0C4, 0, 8, frame);
  }
  if (testFrame0AAWheelSpeedEnabled) {
    unsigned char frame[] = { (uint8_t)(0xF0 | counter4Bit), 0x10, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00 };
    CAN.sendMsgBuf(0x0AA, 0, 8, frame);
  }
  if (testFrame3D0BatteryEnabled) {
    unsigned char frame[] = { (uint8_t)(0xF0 | counter4Bit), 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64 };
    CAN.sendMsgBuf(0x3D0, 0, 8, frame);
  }

  auto sendPresetCC = [this](uint8_t id, bool active, bool& last, uint8_t byte5 = 0xFF) {
    if (active || last) {
      sendTestCCAlert(id, active, byte5);
      last = active;
    }
  };

  sendPresetCC(50, testCC50TyreMonitoringFailureEnabled, lastCC50TyreMonitoringFailure);
  sendPresetCC(63, testCC63TyreFailureEnabled, lastCC63TyreFailure);
  sendPresetCC(144, testCC144TPMSFaultEnabled, lastCC144TPMSFault);
  sendPresetCC(145, testCC145TPMSDeactivatedEnabled, lastCC145TPMSDeactivated);
  sendPresetCC(147, testCC147TyrePressureLossEnabled, lastCC147TyrePressureLoss);
  sendPresetCC(149, testCC149TPMSFailureEnabled, lastCC149TPMSFailure);
  sendPresetCC(192, testCC192TPMSInitialisingEnabled, lastCC192TPMSInitialising);
  sendPresetCC(28, testCC28OilLevelEnabled, lastCC28OilLevel);
  sendPresetCC(36, testCC36DscOffEnabled, lastCC36DscOff);
  sendPresetCC(39, testCC39EngineOverheatEnabled, lastCC39EngineOverheat);
  if (testClearCC167ResetClockEnabled) {
    sendTestCCAlert(167, false);
  }
  sendPresetCC((uint8_t)(299 & 0xFF), testCC299ChassisWarningEnabled, lastCC299ChassisWarning, 0xF7);
  sendPresetCC(196, testCC196SosWarningEnabled, lastCC196SosWarning, 0xF7);

  if (testRawCCActiveEnabled) {
    if (lastRawCCActive && lastRawCCId != testRawCCId) {
      sendTestCCAlert(lastRawCCId, false);
    }
    sendTestCCAlert(testRawCCId, true);
    lastRawCCActive = true;
    lastRawCCId = testRawCCId;
  } else if (lastRawCCActive) {
    sendTestCCAlert(lastRawCCId, false);
    lastRawCCActive = false;
  }
}

void BMWFSeriesCluster::sendIgnitionStatus(uint8_t ignitionLevel) {
  uint8_t ignitionStatus = 0x86;
  if (ignitionLevel == 1) {
    ignitionStatus = 0x88;
  } else if (ignitionLevel >= 2) {
    ignitionStatus = 0x8A;
  }

  unsigned char ignitionWithoutCRC[] = { 0x80|counter4Bit, ignitionStatus, 0xDD, 0xF1, 0x01, 0x30, 0x06 };
  unsigned char ignitionWithCRC[] = { crc8Calculator.get_crc8(ignitionWithoutCRC, 7, 0x44), ignitionWithoutCRC[0], ignitionWithoutCRC[1], ignitionWithoutCRC[2], ignitionWithoutCRC[3], ignitionWithoutCRC[4], ignitionWithoutCRC[5], ignitionWithoutCRC[6] };
  CAN.sendMsgBuf(0x12F, 0, 8, ignitionWithCRC);
}

void BMWFSeriesCluster::sendSpeed(int speed) {
  uint16_t calculatedSpeed = (double)speed * 64.01;
  unsigned char speedWithoutCRC[] = { (uint8_t)(0xC0|speedCounter4Bit), lo8(calculatedSpeed), hi8(calculatedSpeed), (speed == 0 ? 0x81 : 0x91) };
  unsigned char speedWithCRC[] = { crc8Calculator.get_crc8(speedWithoutCRC, 4, 0xA9), speedWithoutCRC[0], speedWithoutCRC[1], speedWithoutCRC[2], speedWithoutCRC[3] };
  CAN.sendMsgBuf(0x1A1, 0, 5, speedWithCRC);

  speedCounter4Bit++;
  if (speedCounter4Bit >= 14) {
    speedCounter4Bit = 0;
  }
}

void BMWFSeriesCluster::sendRPM(int rpm, int manualGear) {
  // The gear that the car is in: 0 = clear, 1-9 = M1-M9, 10 = P, 11 = R, 12 = N, 13 = D
  if (rpm < 0) {
    rpm = 0;
  }

  int calculatedGear = 0;
  switch (manualGear) {
    case 0: calculatedGear = 0; break; // Empty
    case 1 ... 9: calculatedGear = manualGear + 4; break; // 1-9
    case 11: calculatedGear = 2; break; // Reverse
    case 12: calculatedGear = 1; break; // Neutral
  }

  unsigned char rpmWithoutCRC[7];
  if (highResolutionRPMEnabled) {
    uint16_t rpmScaled = (uint16_t)constrain((int)((float)rpm * 0.1f), 0, 0x0FFF);
    uint16_t rpmShifted = rpmScaled << 4;
    rpmWithoutCRC[0] = lo8(rpmShifted) | (rpmCounter4Bit & 0x0F);
    rpmWithoutCRC[1] = hi8(rpmShifted);
    rpmWithoutCRC[2] = 0xC0;
    rpmWithoutCRC[3] = 0xF0;
    rpmWithoutCRC[4] = 0xC4;
    rpmWithoutCRC[5] = 0xFF;
    rpmWithoutCRC[6] = 0xFF;
  } else {
    int rpmValue = map(rpm, 0, 6900, 0x00, 0x2B);
    rpmValue = constrain(rpmValue, 0x00, 0x2B);
    rpmWithoutCRC[0] = 0x60 | rpmCounter4Bit;
    rpmWithoutCRC[1] = rpmValue;
    rpmWithoutCRC[2] = 0xC0;
    rpmWithoutCRC[3] = 0xF0;
    rpmWithoutCRC[4] = calculatedGear;
    rpmWithoutCRC[5] = 0xFF;
    rpmWithoutCRC[6] = 0xFF;
  }

  unsigned char rpmWithCRC[] = { crc8Calculator.get_crc8(rpmWithoutCRC, 7, 0x7A), rpmWithoutCRC[0], rpmWithoutCRC[1], rpmWithoutCRC[2], rpmWithoutCRC[3], rpmWithoutCRC[4], rpmWithoutCRC[5], rpmWithoutCRC[6] };
  CAN.sendMsgBuf(0x0F3, 0, 8, rpmWithCRC);

  rpmCounter4Bit++;
  if (rpmCounter4Bit >= 14) {
    rpmCounter4Bit = 0;
  }
}

void BMWFSeriesCluster::sendAutomaticTransmission(int gear) {
  // The gear that the car is in: 0 = clear, 1-9 = M1-M9, 10 = P, 11 = R, 12 = N, 13 = D
  uint8_t selectedGear = 0;
  switch (gear) {
    case 1 ... 9: selectedGear = 0x81; break; // DS
    case 10: selectedGear = 0x20; break; // P
    case 11: selectedGear = 0x40; break; // R
    case 12: selectedGear = 0x60; break; // N
    case 13: selectedGear = 0x80; break; // D
  }
  unsigned char transmissionWithoutCRC[] = { counter4Bit, selectedGear, 0xFC, 0xFF }; //0x20= P, 0x40= R, 0x60= N, 0x80= D, 0x81= DS
  unsigned char transmissionWithCRC[] = { crc8Calculator.get_crc8(transmissionWithoutCRC, 4, 0xD6), transmissionWithoutCRC[0], transmissionWithoutCRC[1], transmissionWithoutCRC[2], transmissionWithoutCRC[3] };
  CAN.sendMsgBuf(0x3FD, 0, 5, transmissionWithCRC);
}

void BMWFSeriesCluster::sendBasicDriveInfo(int engineTemperature) {
  // ABS
  unsigned char abs1WithoutCRC[] = { 0xF0|counter4Bit, 0xFE, 0xFF, 0x14 };
  unsigned char abs1WithCRC[] = { crc8Calculator.get_crc8(abs1WithoutCRC, 4, 0xD8), abs1WithoutCRC[0], abs1WithoutCRC[1], abs1WithoutCRC[2], abs1WithoutCRC[3] };
  CAN.sendMsgBuf(0x36E, 0, 5, abs1WithCRC);

  // ABS secondary keep-alive from the enhanced F-series fork.
  unsigned char absSecondary[] = { counter4Bit, counter4Bit, counter4Bit, counter4Bit, counter4Bit, counter4Bit, counter4Bit, counter4Bit };
  CAN.sendMsgBuf(0xB6E, 0, 8, absSecondary);

  //Alive counter safety
  unsigned char aliveCounterSafetyWithoutCRC[] = { count, 0xFF };
  CAN.sendMsgBuf(0xD7, 0, 2, aliveCounterSafetyWithoutCRC);

  //Power Steering
  unsigned char steeringColumnWithoutCRC[] = { 0xF0|counter4Bit, 0xFE, 0xFF, 0x14 };
  unsigned char steeringColumnWithCRC[] = { crc8Calculator.get_crc8(steeringColumnWithoutCRC, 4, 0x9E), steeringColumnWithoutCRC[0], steeringColumnWithoutCRC[1], steeringColumnWithoutCRC[2], steeringColumnWithoutCRC[3] };
  CAN.sendMsgBuf(0x2A7, 0, 5, steeringColumnWithCRC);

  // Cruise control 0x289 is now sent by sendCruiseControl() (called every cycle),
  // so it can show the live cruise telltale + set-speed marker instead of a static
  // keep-alive. Do not also send a static 0x289 here (it would override the state).

  //Restraint system (airbag?)
  unsigned char restraintWithoutCRC[] = { 0x40|counter4Bit, 0x40, 0x55, 0xFD, 0xFF, 0xFF, 0xFF };
  unsigned char restraintWithCRC[] = { crc8Calculator.get_crc8(restraintWithoutCRC, 7, 0xFF), restraintWithoutCRC[0], restraintWithoutCRC[1], restraintWithoutCRC[2], restraintWithoutCRC[3], restraintWithoutCRC[4], restraintWithoutCRC[5], restraintWithoutCRC[6] };
  CAN.sendMsgBuf(0x19B, 0, 8, restraintWithCRC);

  // EHC keep-alive from the enhanced F-series fork.
  unsigned char EHCWithoutCRC[] = { 0x40|counter4Bit, 0x40, 0x55, 0xFD, 0xFF, 0xFF, 0xFF };
  unsigned char EHCWithCRC[] = { crc8Calculator.get_crc8(EHCWithoutCRC, 7, 0xFF), EHCWithoutCRC[0], EHCWithoutCRC[1], EHCWithoutCRC[2], EHCWithoutCRC[3], EHCWithoutCRC[4], EHCWithoutCRC[5], EHCWithoutCRC[6] };
  CAN.sendMsgBuf(0x26A, 0, 8, EHCWithCRC);

  //Restraint system (seatbelt?)
  unsigned char restraint2WithoutCRC[] = { 0xE0|counter4Bit, 0xF1, 0xF0, 0xF2, 0xF2, 0xFE };
  unsigned char restraint2WithCRC[] = { crc8Calculator.get_crc8(restraint2WithoutCRC, 6, 0x28), restraint2WithoutCRC[0], restraint2WithoutCRC[1], restraint2WithoutCRC[2], restraint2WithoutCRC[3], restraint2WithoutCRC[4], restraint2WithoutCRC[5] };
  CAN.sendMsgBuf(0x297, 0, 7, restraint2WithCRC);

  //TPMS
  unsigned char TPMSWithoutCRC[] = {
    (uint8_t)(0xF0|counter4Bit),
    (uint8_t)(testFrame369TpmsEnabled ? testFrame369TpmsB1 : 0xA2),
    (uint8_t)(testFrame369TpmsEnabled ? testFrame369TpmsB2 : 0xA0),
    (uint8_t)(testFrame369TpmsEnabled ? testFrame369TpmsB3 : 0xA0)
  };
  unsigned char TPMSWithCRC[] = { crc8Calculator.get_crc8(TPMSWithoutCRC, 4, 0xC5), TPMSWithoutCRC[0], TPMSWithoutCRC[1], TPMSWithoutCRC[2], TPMSWithoutCRC[3] };
  CAN.sendMsgBuf(0x369, 0, 5, TPMSWithCRC);

  if (testFrame368RdcEnabled) {
    unsigned char RDCWithoutCRC[] = {
      (uint8_t)(0xF0|counter4Bit),
      testFrame368RdcB1,
      testFrame368RdcB2,
      testFrame368RdcB3
    };
    unsigned char RDCWithCRC[] = { crc8Calculator.get_crc8(RDCWithoutCRC, 4, testFrame368RdcCrcSeed), RDCWithoutCRC[0], RDCWithoutCRC[1], RDCWithoutCRC[2], RDCWithoutCRC[3] };
    CAN.sendMsgBuf(0x368, 0, 5, RDCWithCRC);
  }

  // Unknown (makes RPM steady)
  // Also engine temp on diesel? Range 100 - 200
  unsigned char oilWithoutCRC[] = { 0x10|counter4Bit, 0x82, 0x4E, 0x7E, engineTemperature + 50, 0x05, 0x89 };
  unsigned char oilWithCRC[] = { crc8Calculator.get_crc8(oilWithoutCRC, 7, 0xF1), oilWithoutCRC[0], oilWithoutCRC[1], oilWithoutCRC[2], oilWithoutCRC[3], oilWithoutCRC[4], oilWithoutCRC[5], oilWithoutCRC[6] };
  CAN.sendMsgBuf(0x3F9, 0, 8, oilWithCRC);

  // Engine temperature
  // range: 0 - 200
  // CRC calculation for this one is weird... there is no counter present but scans show something like CRC
  unsigned char engineTempWithoutCRC[] = { 0x3e, engineTemperature, 0x64, 0x64, 0x64, 0x01, 0xF1 };
  unsigned char engineTempWithCRC[] = { crc8Calculator.get_crc8(engineTempWithoutCRC, 7, 0xB2), engineTempWithoutCRC[0], engineTempWithoutCRC[1], engineTempWithoutCRC[2], engineTempWithoutCRC[3], engineTempWithoutCRC[4], engineTempWithoutCRC[5], engineTempWithoutCRC[6] };
  CAN.sendMsgBuf(0x2C4, 0, 8, engineTempWithCRC);
}

void BMWFSeriesCluster::sendParkBrake(bool handbrakeActive) {
  unsigned char abs3WithoutCRC[] = { 0xF0|counter4Bit, 0x38, 0, handbrakeActive ? 0x15 : 0x14 };
  unsigned char abs3WithCRC[] = { crc8Calculator.get_crc8(abs3WithoutCRC, 4, 0x17), abs3WithoutCRC[0], abs3WithoutCRC[1], abs3WithoutCRC[2], abs3WithoutCRC[3] };
  CAN.sendMsgBuf(0x36F, 0, 5, abs3WithCRC);
}

void BMWFSeriesCluster::sendFuel(int fuelQuantity, uint8_t inFuelRange[], uint8_t outFuelRange[], bool isCarMini) {
  //Fuel
  fuelQuantity = constrain(fuelQuantity, 0, 100);

  if (!isCarMini) {
    if (testFrame349SplitFuelEnabled && !needleSweepActive) {
      unsigned char splitFuelFrame[] = {
        lo8(testFrame349FuelLeft), hi8(testFrame349FuelLeft),
        lo8(testFrame349FuelRight), hi8(testFrame349FuelRight),
        0x00, 0x00, 0x00, 0x00
      };
      CAN.sendMsgBuf(0x349, 0, testFrame349FuelDlc, splitFuelFrame);
      return;
    }

    uint16_t fuelSensorValue = fuelOverrideEnabled && !needleSweepActive
      ? fuelOverrideValue
      : map(fuelQuantity, 0, 100, 9500, 750);
    unsigned char fuelWithoutCRC[] = { lo8(fuelSensorValue), hi8(fuelSensorValue), lo8(fuelSensorValue), hi8(fuelSensorValue), 0x00 };
    CAN.sendMsgBuf(0x349, 0, 5, fuelWithoutCRC);
    return;
  }

  uint8_t fuelQuantityLiters = multiMap<uint8_t>(fuelQuantity, inFuelRange, outFuelRange, 3);
  unsigned char fuelWithoutCRC[] = { 0, 0, hi8(fuelQuantityLiters), lo8(fuelQuantityLiters), 0x00 };
  CAN.sendMsgBuf(0x349, 0, 5, fuelWithoutCRC);
}

void BMWFSeriesCluster::sendDistanceTravelled(GameState& game, int speed) {
  const float baseDistanceStep = (float)speed * 2.9f;
  float distanceStep = baseDistanceStep;

  if (game.hasFuelConsumptionData) {
    float consumption = game.fuelConsumptionLPer100Km;

    if (speed <= 1 && game.fuelRateLitersPerHour > 0.02f) {
      consumption = 35.0f;
    }

    if (speed > 80 && game.throttleInput > 0.85f) {
      float fullThrottleFloor = 8.0f + ((float)speed * 0.06f);
      if (game.engineLoad > 0.2f) {
        fullThrottleFloor += game.engineLoad * 3.0f;
      }
      if (fullThrottleFloor > 40.0f) {
        fullThrottleFloor = 40.0f;
      }
      if (consumption < fullThrottleFloor) {
        consumption = fullThrottleFloor;
      }
    }

    if (consumption < 0.1f) {
      consumption = 0.1f;
    } else if (consumption > 60.0f) {
      consumption = 60.0f;
    }

    // The cluster derives the economy bar from fuel-related 0x2C4 data and
    // the distance counter. Lower counter growth means higher l/100km.
    const float referenceConsumptionLPer100Km = 10.0f;
    float consumptionFactor = consumption / referenceConsumptionLPer100Km;
    bool ecoProCoasting = game.driveMode == 7 && speed > 3 && game.throttleInput < 0.03f;
    float minimumConsumptionFactor = ecoProCoasting ? 0.65f : 0.15f;
    if (consumptionFactor < minimumConsumptionFactor) {
      consumptionFactor = minimumConsumptionFactor;
    } else if (consumptionFactor > 6.0f) {
      consumptionFactor = 6.0f;
    }

    distanceStep = baseDistanceStep / consumptionFactor;
    if (speed <= 1 && game.fuelRateLitersPerHour > 0.02f) {
      distanceStep = 0.2f;
    }
  }

  distanceTravelledAccumulator += distanceStep;
  if (distanceTravelledAccumulator > 65535.0f) {
    distanceTravelledAccumulator -= 65535.0f;
  } else if (distanceTravelledAccumulator < 0.0f) {
    distanceTravelledAccumulator = 0.0f;
  }
  distanceTravelledCounter = (uint16_t)distanceTravelledAccumulator;

  // MPG bar
  unsigned char mpgWithoutCRC[] = { count, 0xFF, 0x64, 0x64, 0x64, 0x01, 0xF1 };
  unsigned char mpgWithCRC[] = { crc8Calculator.get_crc8(mpgWithoutCRC, 7, 0xC6), mpgWithoutCRC[0], mpgWithoutCRC[1], mpgWithoutCRC[2], mpgWithoutCRC[3], mpgWithoutCRC[4], mpgWithoutCRC[5], mpgWithoutCRC[6] };
  CAN.sendMsgBuf(0x2C4, 0, 8, mpgWithCRC);

  // MPG bar 2 (this one actually moves the bar)
  // The distance travelled counter is used to calculate the travelled distance shown in the cluster.
  unsigned char mpg2WithoutCRC[] = { 0xF0|counter4Bit, lo8(distanceTravelledCounter), hi8(distanceTravelledCounter), 0xF2 };
  unsigned char mpg2WithCRC[] = { crc8Calculator.get_crc8(mpg2WithoutCRC, 4, 0xde), mpg2WithoutCRC[0], mpg2WithoutCRC[1], mpg2WithoutCRC[2], mpg2WithoutCRC[3], mpg2WithoutCRC[4] };
  CAN.sendMsgBuf(0x2BB, 0, 5, mpg2WithCRC);
}

void BMWFSeriesCluster::sendBlinkers(bool leftTurningIndicator, bool rightTurningIndicator) {
  //Blinkers
  uint8_t blinkerStatus = 0x80;
  uint8_t blinkerMode = 0xF0;

  if (leftTurningIndicator && rightTurningIndicator) {
    blinkerStatus = 0xB1;
    blinkerMode = 0xF2;
  } else if (leftTurningIndicator) {
    blinkerStatus = 0x91;
    blinkerMode = 0xF2;
  } else if (rightTurningIndicator) {
    blinkerStatus = 0xA1;
    blinkerMode = 0xF2;
  }

  unsigned char blinkersWithoutCRC[] = { blinkerStatus, blinkerMode };
  CAN.sendMsgBuf(0x1F6, 0, 2, blinkersWithoutCRC);
}

void BMWFSeriesCluster::sendLights(bool mainLights, bool highBeam, bool rearFogLight, bool frontFogLight) {
  //Lights
  if (testFrame21A12Dlc8Enabled) {
    uint8_t lightStatus = highBeam ? 0x07 : (mainLights ? 0x04 : 0x00);
    unsigned char sourceFrame[] = { lightStatus, 0x12, 0xF7, 0x00, 0x00, 0x00, 0x00, 0x00 };
    CAN.sendMsgBuf(0x21A, 0, 8, sourceFrame);
    return;
  }

  //32 = front fog light, 64 = rear fog light, 2 = high beam, 4 = main lights
  uint8_t lightStatus = highBeam << 1 | (mainLights || highBeam) << 2 | frontFogLight << 5 | rearFogLight << 6;
  unsigned char lightsWithoutCRC[] = { lightStatus, 0x32, 0xF7 };
  CAN.sendMsgBuf(0x21A, 0, 3, lightsWithoutCRC);
}

void BMWFSeriesCluster::sendBacklightBrightness(uint8_t brightness) {
  // Backlight brightness
  uint8_t mappedBrightness = map(brightness, 0, 100, 0, 253);
  unsigned char backlightBrightnessWithoutCRC[] = { mappedBrightness, 0xFF };
  CAN.sendMsgBuf(0x202, 0, 2, backlightBrightnessWithoutCRC);
}

void BMWFSeriesCluster::sendAlerts(GameState& game, bool offroad, bool handbrake, bool isCarMini) {
  // Check control messages
  // These are complicated since the same CAN ID is used to show variety of messages
  // Sending 0x29 on byte 4 sets the alert for the ID in byte 2, while sending 0x28 clears that message
  // Known IDs:
  // 28: add engine oil / low oil level
  // 34: check engine
  // 35, 215: DSC
  // 36: DSC OFF
  // 39: engine overheated, stop carefully
  // 24: Park brake error (yellow)
  // 71: Park brake error (red)
  // 77 Seat belt indicator
  static bool doorStatesInitialized = false;
  static bool lastDoorFrontRight = false;
  static bool lastDoorFrontLeft = false;
  static bool lastDoorRearLeft = false;
  static bool lastDoorRearRight = false;
  static bool lastDoorBonnet = false;
  static bool lastDoorBoot = false;
  static bool lastEngineDamageWarning = false;
  static bool lastOilLevelWarning = false;
  static bool lastDscOffWarning = false;
  static bool lastEngineOverheatWarning = false;

  bool doorFrontLeft = game.doorDetailsAvailable ? game.doorFrontLeft : game.doorOpen;
  bool doorFrontRight = game.doorDetailsAvailable ? game.doorFrontRight : false;
  bool doorRearLeft = game.doorDetailsAvailable ? game.doorRearLeft : false;
  bool doorRearRight = game.doorDetailsAvailable ? game.doorRearRight : false;
  bool doorBonnet = game.doorDetailsAvailable ? game.doorBonnet : false;
  bool doorBoot = game.doorDetailsAvailable ? game.doorBoot : false;
  bool engineDamageWarning = game.engineDamageWarning;
  bool oilLevelWarning = game.oilLevelWarning;
  bool dscOffWarning = game.driveMode == 6;
  bool engineOverheatWarning = game.engineOverheatWarning || game.coolantTemperature >= 120 || game.oilTemperature >= 145;

  if (!doorStatesInitialized || engineDamageWarning != lastEngineDamageWarning) {
    uint8_t message[] = { 0x40, 34, 0x00, (uint8_t)(engineDamageWarning ? 0x29 : 0x28), 0xFF, 0xFF, 0xFF, 0xFF };
    CAN.sendMsgBuf(0x5c0, 0, 8, message);
    lastEngineDamageWarning = engineDamageWarning;
  }

  auto sendRuntimeCC = [this](uint8_t id, bool active, bool& last) {
    if (active || active != last) {
      uint8_t message[] = { 0x40, id, 0x00, (uint8_t)(active ? 0x29 : 0x28), 0xFF, 0xFF, 0xFF, 0xFF };
      CAN.sendMsgBuf(0x5C0, 0, 8, message);
    }
    last = active;
  };
  sendRuntimeCC(28, oilLevelWarning, lastOilLevelWarning);
  sendRuntimeCC(36, dscOffWarning, lastDscOffWarning);
  sendRuntimeCC(39, engineOverheatWarning, lastEngineOverheatWarning);

  if (!doorStatesInitialized || doorFrontRight != lastDoorFrontRight) {
    uint8_t message[] = { 0x40, 14, 0x00, (uint8_t)(doorFrontRight ? 0x29 : 0x28), 0xFF, 0xFF, 0xFF, 0xFF };
    CAN.sendMsgBuf(0x5c0, 0, 8, message);
    lastDoorFrontRight = doorFrontRight;
  }
  if (!doorStatesInitialized || doorFrontLeft != lastDoorFrontLeft) {
    uint8_t message[] = { 0x40, 15, 0x00, (uint8_t)(doorFrontLeft ? 0x29 : 0x28), 0xFF, 0xFF, 0xFF, 0xFF };
    CAN.sendMsgBuf(0x5c0, 0, 8, message);
    lastDoorFrontLeft = doorFrontLeft;
  }
  if (!doorStatesInitialized || doorRearLeft != lastDoorRearLeft) {
    uint8_t message[] = { 0x40, 16, 0x00, (uint8_t)(doorRearLeft ? 0x29 : 0x28), 0xFF, 0xFF, 0xFF, 0xFF };
    CAN.sendMsgBuf(0x5c0, 0, 8, message);
    lastDoorRearLeft = doorRearLeft;
  }
  if (!doorStatesInitialized || doorRearRight != lastDoorRearRight) {
    uint8_t message[] = { 0x40, 17, 0x00, (uint8_t)(doorRearRight ? 0x29 : 0x28), 0xFF, 0xFF, 0xFF, 0xFF };
    CAN.sendMsgBuf(0x5c0, 0, 8, message);
    lastDoorRearRight = doorRearRight;
  }
  if (!doorStatesInitialized || doorBonnet != lastDoorBonnet) {
    uint8_t message[] = { 0x40, 18, 0x00, (uint8_t)(doorBonnet ? 0x29 : 0x28), 0xFF, 0xFF, 0xFF, 0xFF };
    CAN.sendMsgBuf(0x5c0, 0, 8, message);
    lastDoorBonnet = doorBonnet;
  }
  if (!doorStatesInitialized || doorBoot != lastDoorBoot) {
    uint8_t message[] = { 0x40, 19, 0x00, (uint8_t)(doorBoot ? 0x29 : 0x28), 0xFF, 0xFF, 0xFF, 0xFF };
    CAN.sendMsgBuf(0x5c0, 0, 8, message);
    lastDoorBoot = doorBoot;
  }
  doorStatesInitialized = true;

  if (offroad) {
    uint8_t message[] = { 0x40, 215, 0x00, 0x29, 0xFF, 0xFF, 0xFF, 0xFF };
    CAN.sendMsgBuf(0x5c0, 0, 8, message);
  } else {
    uint8_t message[] = { 0x40, 215, 0x00, 0x28, 0xFF, 0xFF, 0xFF, 0xFF };
    CAN.sendMsgBuf(0x5c0, 0, 8, message);
  }

  if (isCarMini) {
    if (handbrake) {
      uint8_t message[] = { 0x40, 71, 0x00, 0x29, 0xFF, 0xFF, 0xFF, 0xFF };
      CAN.sendMsgBuf(0x5c0, 0, 8, message);
    } else {
      uint8_t message[] = { 0x40, 71, 0x00, 0x28, 0xFF, 0xFF, 0xFF, 0xFF };
      CAN.sendMsgBuf(0x5c0, 0, 8, message);
    }
  }
}

void BMWFSeriesCluster::sendSteeringWheelButton(int buttonEvent) {
  // BMW F-series BC/menu button on 0x1EE needs a press + release edge.
  // buttonEvent == 1 -> BC / menu cycle
  uint8_t pressedValue = 0x00;

  switch (buttonEvent) {
    case 1:
      pressedValue = 0x4C;
      break;
    case 2:
      pressedValue = 0x44;
      break;
    case 3:
      pressedValue = 0x48;
      break;
    case 4:
      pressedValue = 0x40;
      break;
    case 5:
      pressedValue = 0x50;
      break;
    default:
      return;
  }

  uint8_t pressFrame[] = { pressedValue, 0xFF };
  CAN.sendMsgBuf(0x1EE, 0, 2, pressFrame);
  delay(40);
  uint8_t releaseFrame[] = { 0x00, 0xFF };
  CAN.sendMsgBuf(0x1EE, 0, 2, releaseFrame);
}

void BMWFSeriesCluster::updateLanguageAndUnits() {
  //language and units
  // Byte 1 language, byte 2 clock/temp layout, byte 3 units.
  // Default is the original English + metric/l/100km candidate. Enhanced is from CarCluster-F10-Enhanced.
  uint8_t languageAndUnits[8] = {};
  if (enhancedLanguage291Enabled) {
    languageAndUnits[0] = 0x01;
    languageAndUnits[1] = 0x12;
    languageAndUnits[2] = 0x59;
  } else {
    languageAndUnits[0] = 0x02;
    languageAndUnits[1] = 0x12;
    languageAndUnits[2] = 0x01;
  }

  INT8U sendStatus = CAN.sendMsgBuf(0x291, 0, 8, languageAndUnits);
  if (sendStatus != CAN_OK) {
    Serial.print("BMW_F TX 0x291 failed status=");
    Serial.println(sendStatus);
  }
}

bool BMWFSeriesCluster::getLocalTime(struct tm& localTime) {
  time_t now;
  time(&now);

  if (now < 1609459200) {
    return false;
  }

  return localtime_r(&now, &localTime) != nullptr;
}

void BMWFSeriesCluster::logCanFrameTx(const char *label, uint16_t id, INT8U status, const uint8_t *data, uint8_t len) {
#if BMW_F_LOG_MODE == 2 || BMW_F_LOG_MODE == 3
  if (status == CAN_OK) return; // suppress successful TX in TIME/ERRORS mode
#endif
  Serial.print("BMW_F TX ");
  Serial.print(label);
  Serial.print(" 0x");
  if (id < 0x100) {
    Serial.print('0');
  }
  if (id < 0x10) {
    Serial.print('0');
  }
  Serial.print(id, HEX);
  Serial.print(" status=");
  Serial.print(status);
  Serial.print(" data=");
  for (uint8_t i = 0; i < len; i++) {
    if (i > 0) {
      Serial.print(' ');
    }
    if (data[i] < 0x10) {
      Serial.print('0');
    }
    Serial.print(data[i], HEX);
  }
  Serial.println();
}

bool BMWFSeriesCluster::sendDateTime() {
  struct tm localTime;
  if (!getLocalTime(localTime)) {
    return false;
  }

  uint16_t year = localTime.tm_year + 1900;
  uint8_t month = localTime.tm_mon + 1;

  // BMW date/time frame candidate seen on F/E-series captures:
  // hour, minute, second, day, month in high nibble, year LSB/MSB.
  uint8_t dateTime[] = {
    (uint8_t)localTime.tm_hour,
    (uint8_t)localTime.tm_min,
    (uint8_t)localTime.tm_sec,
    (uint8_t)localTime.tm_mday,
    (uint8_t)((month << 4) | 0x0F),
    lo8(year),
    hi8(year),
    0xF5
  };

  INT8U sendStatus = CAN.sendMsgBuf(0x2F8, 0, 8, dateTime);

  if (sendStatus != CAN_OK || BMW_F_LOG_EVERY_DATE_TIME_TX || dateTimeDebugCounter == 0) {
    logCanFrameTx("DATE_TIME", 0x2F8, sendStatus, dateTime, 8);
    if (sendStatus != CAN_OK) {
      Serial.print("BMW_F MCP error=0x");
      uint8_t errorFlags = CAN.getError();
      if (errorFlags < 0x10) {
        Serial.print('0');
      }
      Serial.print(errorFlags, HEX);
      Serial.print(" txErr=");
      Serial.print(CAN.errorCountTX());
      Serial.print(" rxErr=");
      Serial.println(CAN.errorCountRX());
    }
  }

  dateTimeDebugCounter++;
  if (dateTimeDebugCounter >= 15) {
    dateTimeDebugCounter = 0;
  }

  return sendStatus == CAN_OK;
}

bool BMWFSeriesCluster::sendTime39E() {
  struct tm localTime;
  if (!getLocalTime(localTime)) {
    return false;
  }

  uint16_t year = localTime.tm_year + 1900;
  uint8_t month = localTime.tm_mon + 1;
  uint8_t yearOffset = year >= 2000 ? (uint8_t)(year - 2000) : (uint8_t)(year % 100);
  // ISO 8601 weekday: Monday=1 ... Sunday=7 (tm_wday: 0=Sun, 1=Mon, ..., 6=Sat)
  uint8_t isoWeekday = (localTime.tm_wday == 0) ? 7 : (uint8_t)localTime.tm_wday;
  uint8_t bcdHour = (uint8_t)(((localTime.tm_hour / 10) << 4) | (localTime.tm_hour % 10));
  uint8_t bcdMinute = (uint8_t)(((localTime.tm_min / 10) << 4) | (localTime.tm_min % 10));
  uint8_t bcdSecond = (uint8_t)(((localTime.tm_sec / 10) << 4) | (localTime.tm_sec % 10));
  uint8_t bcdDay = (uint8_t)(((localTime.tm_mday / 10) << 4) | (localTime.tm_mday % 10));
  uint8_t bcdMonth = (uint8_t)(((month / 10) << 4) | (month % 10));
  uint8_t bcdYear = (uint8_t)(((yearOffset / 10) << 4) | (yearOffset % 10));
  uint8_t bcdWeekday = isoWeekday; // weekday is single digit, BCD = plain value

  uint8_t timeFrame[8] = {0};
  timeFrame[0] = (uint8_t)localTime.tm_hour;
  timeFrame[1] = (uint8_t)localTime.tm_min;
  timeFrame[2] = (uint8_t)localTime.tm_sec;
  timeFrame[3] = (uint8_t)localTime.tm_mday;

  switch (time39EFormat) {
    case 1:
      timeFrame[4] = (uint8_t)((month << 4) | 0x0F);
      timeFrame[5] = lo8(year);
      timeFrame[6] = hi8(year);
      break;
    case 2:
      timeFrame[4] = month;
      timeFrame[5] = yearOffset;
      timeFrame[6] = 0x00;
      break;
    case 3:
      timeFrame[4] = (uint8_t)((month << 4) | 0x0F);
      timeFrame[5] = yearOffset;
      timeFrame[6] = 0x00;
      break;
    case 4:
      timeFrame[0] = bcdHour;
      timeFrame[1] = bcdMinute;
      timeFrame[2] = bcdSecond;
      timeFrame[3] = bcdDay;
      timeFrame[4] = bcdMonth;
      timeFrame[5] = bcdYear;
      timeFrame[6] = 0x00;
      break;
    case 5:
      // Format 5: HH MM SS DD month yearOffset isoWeekday source
      // Hypothesis: byte6 = ISO weekday (Mon=1..Sun=7), NOT year MSB.
      timeFrame[4] = month;
      timeFrame[5] = yearOffset;    // 26 for 2026
      timeFrame[6] = isoWeekday;   // 1=Mon..7=Sun
      break;
    case 6:
      // Format 6: BCD, ISO weekday in byte6 (Mon=1..Sun=7)
      timeFrame[0] = bcdHour;
      timeFrame[1] = bcdMinute;
      timeFrame[2] = bcdSecond;
      timeFrame[3] = bcdDay;
      timeFrame[4] = bcdMonth;
      timeFrame[5] = bcdYear;
      timeFrame[6] = bcdWeekday;       // Thu=4 (ISO 8601)
      break;
    case 7:
      // Format 7: BCD, Mon=0 weekday + DST flag in bit3 (0x0B = Thu CEST)
      timeFrame[0] = bcdHour;
      timeFrame[1] = bcdMinute;
      timeFrame[2] = bcdSecond;
      timeFrame[3] = bcdDay;
      timeFrame[4] = bcdMonth;
      timeFrame[5] = bcdYear;
      {
        uint8_t wday = (isoWeekday == 7) ? 6 : (isoWeekday - 1);
        uint8_t dst  = (localTime.tm_isdst > 0) ? 0x08 : 0x00;
        timeFrame[6] = wday | dst;
      }
      break;
    case 8:
      // Format 8: BCD + byte6=0x26 hypothesis
      timeFrame[0] = bcdHour;
      timeFrame[1] = bcdMinute;
      timeFrame[2] = bcdSecond;
      timeFrame[3] = bcdDay;
      timeFrame[4] = bcdMonth;
      timeFrame[5] = bcdYear;
      timeFrame[6] = 0x26;
      break;
    case 9:
      // Format 9: BCD, Mon=0 weekday WITHOUT DST — same as original format 7 before DST was added
      // byte6=0x03 for Thursday → best performer (byte2 peaked at 0x57)
      timeFrame[0] = bcdHour;
      timeFrame[1] = bcdMinute;
      timeFrame[2] = bcdSecond;
      timeFrame[3] = bcdDay;
      timeFrame[4] = bcdMonth;
      timeFrame[5] = bcdYear;
      timeFrame[6] = (isoWeekday == 7) ? 6 : (isoWeekday - 1); // Thu=3, no DST bit
      break;
    default:
      timeFrame[4] = month;
      timeFrame[5] = lo8(year);
      timeFrame[6] = hi8(year);
      break;
  }

  // F-series source/status byte: 0x00 is no signal; 0x06 is manual valid time.
  timeFrame[7] = time39ESourceByte;

  INT8U sendStatus = CAN.sendMsgBuf(0x39E, 0, 8, timeFrame);
  logCanFrameTx("TIME_39E", 0x39E, sendStatus, timeFrame, 8);

  return sendStatus == CAN_OK;
}

bool BMWFSeriesCluster::sendTime3F1() {
  struct tm localTime;
  if (!getLocalTime(localTime)) {
    return false;
  }

  uint8_t timeWithoutCRC[] = {
    (uint8_t)(0xF0 | counter4Bit),
    (uint8_t)localTime.tm_hour,
    (uint8_t)localTime.tm_min,
    0x00,
    0x00
  };
  uint8_t timeWithCRC[] = {
    crc8Calculator.get_crc8(timeWithoutCRC, 5, 0xA5),
    timeWithoutCRC[0],
    timeWithoutCRC[1],
    timeWithoutCRC[2],
    timeWithoutCRC[3],
    timeWithoutCRC[4]
  };

  INT8U sendStatus = CAN.sendMsgBuf(0x3F1, 0, 6, timeWithCRC);
  logCanFrameTx("TIME_3F1", 0x3F1, sendStatus, timeWithCRC, 6);
  return sendStatus == CAN_OK;
}

void BMWFSeriesCluster::sendDriveMode(uint8_t driveMode) {
  //1= Traction, 2= Comfort, 4= Sport, 5= Sport+, 6= DSC off, 7= Eco pro
  unsigned char modeWithoutCRC[] = { 0xF0|counter4Bit, 0, 0, driveMode, 0x11, 0xC0 };
  unsigned char modeWithCRC[] = { crc8Calculator.get_crc8(modeWithoutCRC, 6, 0x4a), modeWithoutCRC[0], modeWithoutCRC[1], modeWithoutCRC[2], modeWithoutCRC[3], modeWithoutCRC[4], modeWithoutCRC[5] };
  CAN.sendMsgBuf(0x3A7, 0, 7, modeWithCRC);
}

void BMWFSeriesCluster::sendAcc() {
  unsigned char accWithoutCrc[] = { 0xF0|accCounter, 0x5C, 0x70, 0x00, 0x00 };
  unsigned char accWithCrc[] = { crc8Calculator.get_crc8(accWithoutCrc, 5, 0x6b), accWithoutCrc[0], accWithoutCrc[1], accWithoutCrc[2], accWithoutCrc[3], accWithoutCrc[4] };
  CAN.sendMsgBuf(0x33b, 0, 6, accWithCrc);

  accCounter += 4;
  if (accCounter > 0x0E) {
    accCounter = accCounter - 0x0F;
  }
}
