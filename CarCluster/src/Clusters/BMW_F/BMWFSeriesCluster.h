// ####################################################################################################################
// 
// Code part of CarCluster project by Andrej Rolih. See .ino file more details
// Modified for BMW F10/F11 6WA by WesVoj, 2026. See README.md.
// 
// ####################################################################################################################

#ifndef F_SERIES_DASH
#define F_SERIES_DASH

#include "../../Libs/MultiMap/MultiMap.h" // For fuel level calculation - supports non linear mapping found on BMW clusters ( https://github.com/RobTillaart/MultiMap )
#include "../../Libs/MCP_CAN/mcp_can.h" // CAN Bus Shield Compatibility Library ( https://github.com/coryjfowler/MCP_CAN_lib )

#include "CRC8.h"
#include "../Cluster.h"

#define lo8(x) (uint8_t)((x) & 0xFF)
#define hi8(x) (uint8_t)(((x)>>8) & 0xFF)

struct tm;

#ifndef BMW_F_SYNC_LANGUAGE_AND_UNITS
#define BMW_F_SYNC_LANGUAGE_AND_UNITS 1
#endif

#ifndef BMW_F_SYNC_DATE_TIME_CANDIDATE
#define BMW_F_SYNC_DATE_TIME_CANDIDATE 1
#endif

// Log mode — change this one number to filter Serial output:
//   0 = ALL   (verbose, every CAN frame)
//   1 = UDS   (only UDS TX/RX + errors — use when doing DID probes)
//   2 = TIME  (only 0x393/0x328 watch + 0x39E/0x3F1 TX + errors — use when debugging time display)
//   3 = ERRORS (only TX/MCP errors — quiet background run)
#ifndef BMW_F_LOG_MODE
#define BMW_F_LOG_MODE 2
#endif

#ifndef BMW_F_LOG_EVERY_DATE_TIME_TX
#define BMW_F_LOG_EVERY_DATE_TIME_TX 1
#endif

struct BMWFExperimentalOptions {
  bool languageAndUnits291;
  bool enhancedLanguage291;
  bool dateTime2F8;
  bool time39E;
  uint8_t time39ESource;
  uint8_t time39EFormat;
  bool time3F1;
  bool fastRPMRefresh;
  bool highResolutionRPM;
  bool lim287;
  bool lim289Icon;
  bool outsideTemperature2CA;
  bool autoStartStop30B;
  bool autoHold5C0;
  bool tpmsWarningFL5C0;
  bool tpmsWarningFR5C0;
  bool tpmsWarningRL5C0;
  bool tpmsWarningRR5C0;
  bool tpmsCandidate31C;
  bool tpmsCandidate31F;
  bool tpmsCandidateB68;
  bool laneAssistAlwaysOn;
  bool laneSweep;
};

struct BMWFTestOptions {
  bool laneAssist327;
  bool laneAssistScanIds;
  uint16_t laneAssistId;
  uint8_t laneAssistLeft;
  uint8_t laneAssistRight;
  uint8_t laneAssistCrcSeed;
  uint8_t frame1D6ButtonMode;
  bool frame327RawA214;
  bool frame349SplitFuel;
  uint16_t frame349FuelLeft;
  uint16_t frame349FuelRight;
  uint8_t frame349FuelDlc;
  bool frame33BRawAcc;
  bool frame21A12Dlc8;
  bool frame2E4SteeringButton;
  bool frame1EERawLeftMenu;
  bool frame393;
  bool frame1B3;
  bool frame2C5;
  bool frame381;
  bool frame2C3;
  bool frame3A0;
  bool frame581;
  bool frame0AB;
  bool frame130Icm;
  bool frame0C4Szl;
  bool frame0AAWheelSpeed;
  bool frame3D0Battery;
  bool frame368Rdc;
  uint8_t frame368RdcB1;
  uint8_t frame368RdcB2;
  uint8_t frame368RdcB3;
  uint8_t frame368RdcCrcSeed;
  bool frame369Tpms;
  uint8_t frame369TpmsB1;
  uint8_t frame369TpmsB2;
  uint8_t frame369TpmsB3;
  bool cc50TyreMonitoringFailure;
  bool cc63TyreFailure;
  bool cc144TPMSFault;
  bool cc145TPMSDeactivated;
  bool cc147TyrePressureLoss;
  bool cc149TPMSFailure;
  bool cc192TPMSInitialising;
  bool cc28OilLevel;
  bool cc36DscOff;
  bool cc39EngineOverheat;
  bool time39E6waCandidate;
  bool clearCC167ResetClock;
  bool cc299ChassisWarning;
  bool cc196SosWarning;
  bool rawCCActive;
  uint8_t rawCCId;
};

class BMWFSeriesCluster: public Cluster {

  // Do not ever send 0x380 ID - that is VIN number

  public:
  static ClusterConfiguration clusterConfig(bool isCarMini) {
    ClusterConfiguration config;
    config.minimumCoolantTemperature = 50;
    config.maximumCoolantTemperature = 150;
    config.maximumSpeedValue = 260;

    if (isCarMini) {
      config.maximumRPMValue = 7000;
    } else {
      config.maximumRPMValue = 6000;
    }

    return config;
  }

  BMWFSeriesCluster(MCP_CAN& CAN, bool isCarMini);
  void updateWithGame(GameState& game);
  void updateLanguageAndUnits();
  BMWFExperimentalOptions getExperimentalOptions() const;
  void setExperimentalOptions(const BMWFExperimentalOptions& options);
  BMWFTestOptions getTestOptions() const;
  void setTestOptions(const BMWFTestOptions& options);
  void setCruiseMarkerTest(bool enabled, uint8_t d1, uint8_t d2, uint8_t d4, uint8_t d5, uint8_t d6);
  void setRawTestFrame(bool enabled, uint16_t id, const uint8_t data[8]);
  void setFuelOverride(bool enabled, uint16_t senderValue);
  void startNeedleSweep();
  bool isNeedleSweepActive() const;

  uint8_t inFuelRange[3] = {};
  uint8_t outFuelRange[3] = {};

  private:
  MCP_CAN &CAN;
  CRC8 crc8Calculator;
  // Live cruise set-speed marker experiment (0x289 data bytes), set over serial.
  bool cruiseMarkerTestEnabled = false;
  uint8_t cruiseMarkerTestD1 = 0xE0;
  uint8_t cruiseMarkerTestD2 = 0xE0;
  uint8_t cruiseMarkerTestD4 = 0x00;
  uint8_t cruiseMarkerTestD5 = 0xEC;
  uint8_t cruiseMarkerTestD6 = 0x01;
  // Generic repeated raw test frame (serial action 14) for hunting other markers.
  bool rawTestEnabled = false;
  uint16_t rawTestId = 0;
  uint8_t rawTestData[8] = {0};
  // Fuel sender calibration override (serial action 15).
  bool fuelOverrideEnabled = false;
  uint16_t fuelOverrideValue = 0;

  unsigned long dashboardUpdateTime100 = 100;
  unsigned long dashboardUpdateTime1000 = 500;
  unsigned long dashboardUpdateTimeRPM = 10;
  unsigned long dashboardUpdateTimeSpeed = 20;
  unsigned long dashboardUpdateTimeBlinkers = 50;
  unsigned long dashboardUpdateTimeLights = 100;
  unsigned long dashboardUpdateTimeWakeUp = 100;
  unsigned long dashboardUpdateTimeDateTime = 15000;
  unsigned long dashboardUpdateTimeDateTimeRetry = 1000;
  unsigned long dashboardUpdateTimeTimeCandidates = 100; // 100ms — flood KOMBI during rapid sync window
  unsigned long dashboardUpdateTimeLanguageAndUnits = 5000;
  unsigned long lastDashboardUpdateTime = 0; // Timer for the fast updated variables
  unsigned long lastDashboardUpdateTime1000ms = 0; // Timer for slow updated variables
  unsigned long lastDashboardUpdateTimeRPM = 0; // Timer for high-rate tachometer updates
  unsigned long lastDashboardUpdateTimeSpeed = 0; // Timer for high-rate speedometer updates
  unsigned long lastDashboardUpdateTimeBlinkers = 0; // Timer for indicator status updates
  unsigned long lastDashboardUpdateTimeLights = 0; // Timer for exterior light status updates
  unsigned long lastDashboardUpdateTimeWakeUp = 0; // Timer for KOMBI wake-up/keep-awake frame
  unsigned long lastDashboardUpdateTimeDateTime = 0; // Timer for cluster clock/date sync
  unsigned long lastDashboardUpdateTimeCandidates = 0; // Timer for 0x39E/0x3F1 clock experiments
  unsigned long lastDashboardUpdateTimeLanguageAndUnits = 0; // Timer for language/unit sync
  bool hasSentDateTime = false;
  bool hasSentLanguageAndUnits = false;
  bool languageAndUnits291Enabled = false;
  bool enhancedLanguage291Enabled = false;
  bool dateTime2F8Enabled = false;
  bool time39EEnabled = true;
  uint8_t time39ESourceByte = 0x04; // GPS source — best combination
  uint8_t time39EFormat = 9;        // BCD + byte6=0x03 (Mon=0 Thu, no DST)
  bool time3F1Enabled = true; // enable CRC time frame alongside 0x39E
  bool fastRPMRefreshEnabled = true;
  bool highResolutionRPMEnabled = true;
  bool lim287Enabled = false;
  bool lim289IconEnabled = false;
  bool outsideTemperature2CAEnabled = false;
  bool autoStartStop30BEnabled = false;
  bool autoHold5C0Enabled = true;
  bool tpmsWarningFL5C0Enabled = false;
  bool tpmsWarningFR5C0Enabled = false;
  bool tpmsWarningRL5C0Enabled = false;
  bool tpmsWarningRR5C0Enabled = false;
  bool tpmsCandidate31CEnabled = false;
  bool tpmsCandidate31FEnabled = false;
  bool tpmsCandidateB68Enabled = false;
  bool testLaneAssist327Enabled = false;
  bool testLaneAssistScanIdsEnabled = false;
  uint16_t testLaneAssistId = 0x327;
  uint8_t testLaneAssistLeft = 0x03;
  uint8_t testLaneAssistRight = 0x03;
  uint8_t testLaneAssistCrcSeed = 0x27;
  // Persistent lane-departure (KAFAS 0x327) sender. Unlike the test toggles
  // above, this defaults ON and survives reboot, so the lane lines stay lit
  // without touching the /test page. It reuses the lane payload fields above
  // (id 0x327, left/right state bytes, CRC seed) and is sent every 100 ms with
  // the rolling alive counter the KOMBI expects (ST_TLC_ALIVE / ST_TLC_TIMEOUT).
  //
  // IMPORTANT: the KOMBI only DRAWS the lanes if its own coding has
  // FZG_Ausstattung -> TLC_VERBAUT = aktiv. A cluster from a car that never had
  // lane departure ignores 0x327 entirely, no matter what we send. The hidden
  // test menu lighting the lane segment proves the graphic exists, not that it
  // is CAN-driven. See the notes on the /bmw-f-controls page.
  bool laneAssistAlwaysOnEnabled = true;
  // Empirical CRC-seed sweep for 0x327. The per-message CRC "seed" (final XOR of
  // the CRC-8/J1850 poly 0x1D, init 0xFF) is message-ID specific and unknown for
  // 0x327; a wrong seed makes the KOMBI drop every frame. When enabled, the seed
  // is cycled 0x00..0xFF (~1.5 s each) and the active seed is logged to Serial,
  // so you can flash once, watch the cluster, and read off the seed that lights
  // the lanes. Left/right state bytes stay at the /test values during the sweep.
  bool laneSweepEnabled = false;
  uint8_t laneSweepSeed = 0x00;
  unsigned long laneSweepLastMs = 0;
  uint8_t testFrame1D6ButtonMode = 0;
  uint8_t testFrame1D6LastButtonMode = 0;
  bool testFrame327RawA214Enabled = false;
  bool testFrame349SplitFuelEnabled = false;
  uint16_t testFrame349FuelLeft = 0x0100;
  uint16_t testFrame349FuelRight = 0x0ABE;
  uint8_t testFrame349FuelDlc = 8;
  bool testFrame33BRawAccEnabled = false;
  bool testFrame21A12Dlc8Enabled = false;
  bool testFrame2E4SteeringButtonEnabled = false;
  bool testFrame1EERawLeftMenuEnabled = false;
  bool testFrame1EERawLeftMenuWasEnabled = false;
  bool testFrame393Enabled = false;
  bool testFrame1B3Enabled = false;
  bool testFrame2C5Enabled = false;
  bool testFrame381Enabled = false;
  bool testFrame2C3Enabled = true;
  bool testFrame3A0Enabled = false;
  bool testFrame581Enabled = false;
  bool testFrame0ABEnabled = false;
  bool testFrame130IcmEnabled = false;
  bool testFrame0C4SzlEnabled = false;
  bool testFrame0AAWheelSpeedEnabled = false;
  bool testFrame3D0BatteryEnabled = false;
  bool testFrame368RdcEnabled = true;
  uint8_t testFrame368RdcB1 = 0xA2;
  uint8_t testFrame368RdcB2 = 0xA0;
  uint8_t testFrame368RdcB3 = 0xA0;
  uint8_t testFrame368RdcCrcSeed = 0xC5;
  bool testFrame369TpmsEnabled = false;
  uint8_t testFrame369TpmsB1 = 0xA2;
  uint8_t testFrame369TpmsB2 = 0xA0;
  uint8_t testFrame369TpmsB3 = 0xA0;
  bool testCC50TyreMonitoringFailureEnabled = false;
  bool testCC63TyreFailureEnabled = false;
  bool testCC144TPMSFaultEnabled = false;
  bool testCC145TPMSDeactivatedEnabled = false;
  bool testCC147TyrePressureLossEnabled = false;
  bool testCC149TPMSFailureEnabled = false;
  bool testCC192TPMSInitialisingEnabled = false;
  bool testCC28OilLevelEnabled = false;
  bool testCC36DscOffEnabled = false;
  bool testCC39EngineOverheatEnabled = false;
  bool testTime39E6waCandidateEnabled = false;
  bool testClearCC167ResetClockEnabled = true;
  bool testCC299ChassisWarningEnabled = false;
  bool testCC196SosWarningEnabled = false;
  bool testRawCCActiveEnabled = false;
  uint8_t testRawCCId = 147;
  bool autoHold5C0Latched = false;
  uint8_t dateTimeDebugCounter = 0;
  uint8_t lastIgnitionLevel = 0;
  bool needleSweepActive = false;
  unsigned long needleSweepStartMs = 0;
  const unsigned long needleSweepTravelMs = 3000;
  const unsigned long needleSweepZeroHoldMs = 300;

  uint8_t counter4Bit = 0;
  uint8_t rpmCounter4Bit = 0;
  uint8_t speedCounter4Bit = 0;
  uint8_t accCounter = 0;
  uint8_t count = 0;
  uint16_t distanceTravelledCounter = 0;
  float distanceTravelledAccumulator = 0.0f;
  bool isCarMini = false;

  void sendWakeUp();
  void sendIgnitionStatus(uint8_t ignitionLevel);
  void sendSpeed(int speed);
  void sendRPM(int rpm, int manualGear);
  void sendAutomaticTransmission(int gear);
  void sendBasicDriveInfo(int engineTemperature);
  void sendParkBrake(bool handbrakeActive);
  void sendFuel(int fuelQuantity, uint8_t inFuelRange[], uint8_t outFuelRange[], bool isCarMini);
  void sendDistanceTravelled(GameState& game, int speed);
  void sendBlinkers(bool leftTurningIndicator, bool rightTurningIndicator);
  void sendLights(bool mainLights, bool highBeam, bool rearFogLight, bool frontFogLight);
  void sendBacklightBrightness(uint8_t brightness);
  void sendAlerts(GameState& game, bool offroad, bool handbrake, bool isCarMini);
  void sendSteeringWheelButton(int buttonEvent);
  void sendDriveMode(uint8_t driveMode);
  void sendAcc();
  void sendFixedLIM(uint8_t speedKmh);
  void sendCruiseControl(bool active, int setSpeedKmh);
  void sendOutsideTemperature(int temperature);
  void sendAutoStartStop(uint8_t ignitionLevel);
  void sendAutoHold(bool active);
  void sendTPMSWarnings(bool frontLeft, bool frontRight, bool rearLeft, bool rearRight);
  void sendTPMSCandidateFrames();
  void sendTestFrames();
  void sendLaneAssistCandidate(uint16_t canId);
  void sendTestCCAlert(uint8_t id, bool active, uint8_t byte5 = 0xFF);
  float getNeedleSweepPosition(unsigned long nowMs);
  bool sendDateTime();
  bool sendTime39E();
  bool sendTime3F1();
  bool getLocalTime(struct tm& localTime);
  void logCanFrameTx(const char *label, uint16_t id, INT8U status, const uint8_t *data, uint8_t len);

  uint8_t mapGenericGearToLocalGear(GearState inputGear);
  int mapSpeed(GameState& game);
  int mapRPM(GameState& game);
  int mapCoolantTemperature(GameState& game);
};

#endif
