// ####################################################################################################################
//
// CarCluster
// https://github.com/r00li/CarCluster
//
// By Andrej Rolih
// https://www.r00li.com
// BMW F10/F11 6WA modifications by WesVoj, 2026.
// See the repository README for the complete change and attribution notice.
//
// ####################################################################################################################

// --------------------------------------------------------------------
// ----------------------- BEGIN USER CONFIGURATION -------------------
// --------------------------------------------------------------------

// To which Arduino/ESP pin have you connected the CS (chip select) pin of your CAN interface?
#define SPI_CS_PIN 5

// To which Arduino/ESP pin have you connected the INT (interrupt) pin of your CAN interface?
#define CAN_INT 27


// This firmware targets the verified BMW F10/F11 6WA diesel cluster profile only.
// All other cluster implementations have been removed from this fork, so there
// is no longer a cluster selection to make here.

// Configure the maximum RPM value shown on the cluster
// Leave at 0 for using the defaults based on the cluster. Change this is your cluster has different limits
#define MAXIMUM_RPM 0

// A correction factor for the RPM value. RPM will be multiplied by this value.
// This enables you to fix displayed values that are slightly off, though you might not be able
// to ever fully get them calibrated correctly - depending on the cluster.
#define RPM_CORRECTION_FACTOR 1.0

// Configure the maximum speed shown on the cluster (in km/h)
// Leave at 0 for using the defaults based on the cluster. Change this is your cluster has different limits
#define MAXIMUM_SPEED 0

// A correction factor for the speed. Speed will be multiplied by this value.
// This enables you to fix displayed values that are slightly off, though you might not be able
// to ever fully get them calibrated correctly - depending on the cluster.
#define SPEED_CORRECTION_FACTOR 1.0

// Define the minimum and maximum coolant temperature your cluster can display
// Leave alone if your cluster has no such display
// Leave at 0 for using the defaults based on the cluster. Change this is your cluster has different limits.
#define MINIMUM_COOLANT_TEMPERATURE 0
#define MAXIMUM_COOLANT_TEMPERATURE 0

// Select if you want Wifi to be enabled or not.
// Wifi gives you a web dashboard that you can use for testing
// and the ability to connect to certain games directly, but will only work on an ESP32.
// If using a different board select 0.
//
// In order to connect to wifi the ESP will on first boot create a wifi access point called CarCluster. Connect to it (password is "carcluster"),
// then open your web browser and navigate to 192.168.4.1 and use the UI there to connect your wifi network (if configuration popup doesn't open automatically).
// If wifi is not connected after 3 minutes the ESP will continue normal operation and you can use it in Simhub/serial mode
//
// 1 for enabled
// 0 for disabled
#define WIFI_ENABLED 1

// Analog fuel configuration is unused by BMW F CAN fuel, but the shared
// configuration helper still carries these values.
#define ANALOG_FUEL_POT_MINIMUM_VALUE -1
#define ANALOG_FUEL_POT_MAXIMUM_VALUE -1
#define ANALOG_FUEL_POT_MINIMUM_VALUE2 -1
#define ANALOG_FUEL_POT_MAXIMUM_VALUE2 -1

// --------------------------------------------------------------------
// ------------------------ END USER CONFIGURATION --------------------
// --------------------------------------------------------------------

// ------------------------ BEGIN OTHER CONFIGURATION -----------------
// Other configurable variables that usually don't need changing

// How often is the web dashboard updated
#define WIFI_WEB_DASHBOARD_UPDATE_INTERVAL 3000

// Name of the access point created
#define WIFI_CONFIG_PORTAL_ACCESS_POINT_NAME "CarCluster"

// Password for the configuration access point
#define WIFI_CONFIG_PORTAL_ACCESS_POINT_PASSWORD "carcluster"

// Timeout of the config portal after which it will continue in wifi-less mode
#define WIFI_CONFIG_PORTAL_TIMEOUT 180

// Time sync used for the BMW F-series cluster clock frame.
// Prague/Czech Republic POSIX TZ: CET in winter, CEST in summer.
#define TIME_ZONE "CET-1CEST,M3.5.0/2,M10.5.0/3"
#define NTP_SERVER_1 "pool.ntp.org"
#define NTP_SERVER_2 "time.cloudflare.com"
#define NTP_SERVER_3 "time.google.com"

// What is the maximum character length of the serial message
#define MAX_SERIAL_MESSAGE_LENGTH 250

// Baud rate of the USB serial connection (if using Simhub set this to the same value)
#define SERIAL_BAUD_RATE 921600

// Serial Monitor debug output.
// CAN RX all = every CAN frame received by the MCP2515 is printed.
// Serial RX messages = every complete JSON line received over USB serial is printed.
#define SERIAL_LOG_CAN_RX_ALL 1
#define SERIAL_LOG_SERIAL_RX_MESSAGES 1

// UDP/TCP ports used for various games and other stuff.
// Only applicable if wifi is enabled.
#define WIFI_FORZA_UDP_PORT 1101
#define WIFI_BEAM_UDP_PORT 1102
#define WIFI_WEB_DASHBOARD_PORT 80

// ------------------------ END OTHER CONFIGURATION -------------------


// Libraries
#include <SPI.h>  // CAN Bus Shield SPI Pin Library (arduino system library)
#include <time.h>
#include <sys/time.h>

#include "src/Libs/ArduinoJson/ArduinoJson.h"  // For parsing serial data and for ESPDash ( https://github.com/bblanchon/ArduinoJson )
#include "src/Libs/MCP_CAN/mcp_can.h"          // CAN Bus Shield Compatibility Library ( https://github.com/coryjfowler/MCP_CAN_lib )

#include "src/Games/GameSimulation.h"
#include "src/Games/SimhubGame.h"

// CAN bus configuration
MCP_CAN CAN(SPI_CS_PIN);  // Set CS pin

// CAN bus Receiving
long unsigned int canRxId;
unsigned char canRxLen = 0;
unsigned char canRxBuf[8];
char canRxMsgString[128];  // Array to store serial string

// Cluster initialization - BMW F10/F11 6WA diesel only.
#include "src/Clusters/BMW_F/BMWFSeriesCluster.h"
BMWFSeriesCluster cluster(CAN, false);
ClusterConfiguration defaultClusterConfig = cluster.clusterConfig(false);

// Game simulation variables
ClusterConfiguration clusterConfig = ClusterConfiguration::updatedFromDefaults(defaultClusterConfig, SPEED_CORRECTION_FACTOR, RPM_CORRECTION_FACTOR, MAXIMUM_RPM, MAXIMUM_SPEED, MINIMUM_COOLANT_TEMPERATURE, MAXIMUM_COOLANT_TEMPERATURE, ANALOG_FUEL_POT_MINIMUM_VALUE, ANALOG_FUEL_POT_MAXIMUM_VALUE, ANALOG_FUEL_POT_MINIMUM_VALUE2, ANALOG_FUEL_POT_MAXIMUM_VALUE2);
GameState game(clusterConfig);
SimhubGame simhubGame(game);

bool bmwFUDSPassiveFlowControlEnabled = true;

const char *bmwFUDSClockProbeStatusText();
void startBMWFUDSCANClockProbe();
void startBMWFUDSCAN86ClockProbe();
void startBMWFUDSZGMInternalClockProbe();
void startBMWFUDSZGM86ClockProbe();
void startBMWFUDSClockProbe();
void startBMWFUDSZGWClockProbe();
void startBMWFUDSCANRTCWriteProbe();
void startBMWFUDSCAN86RTCWriteProbe();
void startBMWFUDSCAN86RTCWriteProgSession();
void startBMWFUDSCAN86SecuritySeedProgSession();
void startBMWFUDSCAN86RoutineScan();
void startBMWFUDSZGMRTCWriteProbe();
void startBMWFUDSZGM86RTCWriteProbe();
void startBMWFUDSCANSecuritySeedProbe();
void startBMWFUDSCAN86SecuritySeedProbe();
void startBMWFUDSZGMSecuritySeedProbe();
void startBMWFUDSZGM86SecuritySeedProbe();
void startBMWFUDSTargetScan();
void startBMWFUDSZGWTargetScan();
void startBMWFUDSDirectTargetScan();
void updateBMWFUDSClockProbe();
void handleBMWFUDSResponse(unsigned long id, unsigned char len, const unsigned char *data);
// DID sweep + downloadable log are defined further down; the web handlers below
// reference them, so declare them up here (extern for the log buffer globals).
void startBMWFUDSCAN86DidSweep();
void startBMWFUDSReadD113();
void startBMWFUDSReadVin();
extern char bmwFUDSLogBuffer[];
extern uint16_t bmwFUDSLogLen;

#if WIFI_ENABLED == 1
// Wifi/web portal variables
#include "src/Other/WifiFunctions.h"
#include "src/Other/WebDashboard.h"
#include "src/Other/AudioVUWebPage.h"

#include "src/Other/HttpWebServer.h"

#include "src/Games/ForzaHorizonGame.h"
#include "src/Games/BeamNGGame.h"

WifiFunctions wifiFunctions;
WebDashboard webDashboard(game, WIFI_WEB_DASHBOARD_UPDATE_INTERVAL);

ForzaHorizonGame forzaHorizonGame(game, WIFI_FORZA_UDP_PORT);
BeamNGGame beamNGGame(game, WIFI_BEAM_UDP_PORT);

void webDashboardGetState(DashboardState *data) {
  webDashboard.getState(data);
}
void webDashBoardSetState(DashboardState *data) {
  webDashboard.setState(data);
}
bool webDashboardCheckSteeringButtonPressed(void) {
  return false;
}
void webDashboardSetSteeringButtonPressed(HttpString params) {
  webDashboard.steeringWheelAction(params);
}
const char *checkedAttr(bool checked) {
  return checked ? " checked" : "";
}

const char *selectedAttr(bool selected) {
  return selected ? " selected" : "";
}

uint32_t httpUIntVar(HttpMessage *message, const char *name, uint32_t fallback, uint32_t minValue, uint32_t maxValue) {
  char value[16];
  if (httpGetVar(&message->body, name, value, sizeof(value)) <= 0) {
    return fallback;
  }

  char *end = nullptr;
  unsigned long parsed = strtoul(value, &end, 0);
  if (end == value) {
    return fallback;
  }
  if (parsed < minValue) {
    return minValue;
  }
  if (parsed > maxValue) {
    return maxValue;
  }
  return parsed;
}

void webDashboardBMWFControls(HttpConnection *connection, int event, void *eventData) {
  if (event != HTTP_EVENT_REQUEST) {
    return;
  }

  HttpMessage *message = (HttpMessage *)eventData;
  BMWFExperimentalOptions options = cluster.getExperimentalOptions();

  if (httpStringCompare(message->method, httpString("POST")) == 0) {
    char value[8];
    options.languageAndUnits291 = httpGetVar(&message->body, "lang291", value, sizeof(value)) > 0;
    options.enhancedLanguage291 = false;
    options.dateTime2F8 = httpGetVar(&message->body, "time2f8", value, sizeof(value)) > 0;
    options.time39E = httpGetVar(&message->body, "time39e", value, sizeof(value)) > 0;
    options.time39ESource = (uint8_t)httpUIntVar(message, "time39e_src", options.time39ESource, 0, 255);
    options.time39EFormat = (uint8_t)httpUIntVar(message, "time39e_fmt", options.time39EFormat, 0, 4);
    options.time3F1 = httpGetVar(&message->body, "time3f1", value, sizeof(value)) > 0;
    options.fastRPMRefresh = httpGetVar(&message->body, "rpm_fast", value, sizeof(value)) > 0;
    options.highResolutionRPM = httpGetVar(&message->body, "rpm_highres", value, sizeof(value)) > 0;
    options.lim287 = httpGetVar(&message->body, "lim287", value, sizeof(value)) > 0;
    options.lim289Icon = httpGetVar(&message->body, "lim289", value, sizeof(value)) > 0;
    options.outsideTemperature2CA = httpGetVar(&message->body, "temp2ca", value, sizeof(value)) > 0;
    options.autoStartStop30B = httpGetVar(&message->body, "ass30b", value, sizeof(value)) > 0;
    options.autoHold5C0 = httpGetVar(&message->body, "autohold5c0", value, sizeof(value)) > 0;
    options.tpmsWarningFL5C0 = httpGetVar(&message->body, "tpms_fl", value, sizeof(value)) > 0;
    options.tpmsWarningFR5C0 = httpGetVar(&message->body, "tpms_fr", value, sizeof(value)) > 0;
    options.tpmsWarningRL5C0 = httpGetVar(&message->body, "tpms_rl", value, sizeof(value)) > 0;
    options.tpmsWarningRR5C0 = httpGetVar(&message->body, "tpms_rr", value, sizeof(value)) > 0;
    options.tpmsCandidate31C = httpGetVar(&message->body, "tpms31c", value, sizeof(value)) > 0;
    options.tpmsCandidate31F = httpGetVar(&message->body, "tpms31f", value, sizeof(value)) > 0;
    options.tpmsCandidateB68 = httpGetVar(&message->body, "tpmsb68", value, sizeof(value)) > 0;
    options.laneAssistAlwaysOn = httpGetVar(&message->body, "lane_always", value, sizeof(value)) > 0;
    options.laneSweep = httpGetVar(&message->body, "lane_sweep", value, sizeof(value)) > 0;
    bmwFUDSPassiveFlowControlEnabled = httpGetVar(&message->body, "uds_passive_fc", value, sizeof(value)) > 0;
    bool isUDSAction =
      httpGetVar(&message->body, "uds_can_run", value, sizeof(value)) > 0 || httpGetVar(&message->body, "uds_can86_run", value, sizeof(value)) > 0 || httpGetVar(&message->body, "uds_zgm86_run", value, sizeof(value)) > 0 || httpGetVar(&message->body, "uds_zgm_run", value, sizeof(value)) > 0 || httpGetVar(&message->body, "uds_run", value, sizeof(value)) > 0 || httpGetVar(&message->body, "uds_zgw_run", value, sizeof(value)) > 0 || httpGetVar(&message->body, "uds_write_rtc_can", value, sizeof(value)) > 0 || httpGetVar(&message->body, "uds_write_rtc_can86", value, sizeof(value)) > 0 || httpGetVar(&message->body, "uds_write_rtc_zgm", value, sizeof(value)) > 0 || httpGetVar(&message->body, "uds_write_rtc_zgm86", value, sizeof(value)) > 0 || httpGetVar(&message->body, "uds_sa_seed_can", value, sizeof(value)) > 0 || httpGetVar(&message->body, "uds_sa_seed_can86", value, sizeof(value)) > 0 || httpGetVar(&message->body, "uds_sa_seed_zgm", value, sizeof(value)) > 0 || httpGetVar(&message->body, "uds_sa_seed_zgm86", value, sizeof(value)) > 0 || httpGetVar(&message->body, "uds_scan", value, sizeof(value)) > 0 || httpGetVar(&message->body, "uds_zgw_scan", value, sizeof(value)) > 0 || httpGetVar(&message->body, "uds_direct_scan", value, sizeof(value)) > 0 || httpGetVar(&message->body, "uds_sweep86_run", value, sizeof(value)) > 0 || httpGetVar(&message->body, "uds_read_d113", value, sizeof(value)) > 0 || httpGetVar(&message->body, "uds_write_rtc_can86_prog", value, sizeof(value)) > 0 || httpGetVar(&message->body, "uds_sa_seed_can86_prog", value, sizeof(value)) > 0 || httpGetVar(&message->body, "uds_routine_scan86", value, sizeof(value)) > 0;
    if (isUDSAction) {
      bmwFUDSPassiveFlowControlEnabled = true;
    }
    cluster.setExperimentalOptions(options);
    if (httpGetVar(&message->body, "bmwf_button_bc", value, sizeof(value)) > 0) {
      game.buttonEventToProcess = 1;
    }
    if (httpGetVar(&message->body, "uds_can_run", value, sizeof(value)) > 0) {
      startBMWFUDSCANClockProbe();
    } else if (httpGetVar(&message->body, "uds_can86_run", value, sizeof(value)) > 0) {
      startBMWFUDSCAN86ClockProbe();
    } else if (httpGetVar(&message->body, "uds_zgm86_run", value, sizeof(value)) > 0) {
      startBMWFUDSZGM86ClockProbe();
    } else if (httpGetVar(&message->body, "uds_zgm_run", value, sizeof(value)) > 0) {
      startBMWFUDSZGMInternalClockProbe();
    } else if (httpGetVar(&message->body, "uds_run", value, sizeof(value)) > 0) {
      startBMWFUDSClockProbe();
    } else if (httpGetVar(&message->body, "uds_zgw_run", value, sizeof(value)) > 0) {
      startBMWFUDSZGWClockProbe();
    } else if (httpGetVar(&message->body, "uds_write_rtc_can", value, sizeof(value)) > 0) {
      startBMWFUDSCANRTCWriteProbe();
    } else if (httpGetVar(&message->body, "uds_write_rtc_can86", value, sizeof(value)) > 0) {
      startBMWFUDSCAN86RTCWriteProbe();
    } else if (httpGetVar(&message->body, "uds_write_rtc_zgm", value, sizeof(value)) > 0) {
      startBMWFUDSZGMRTCWriteProbe();
    } else if (httpGetVar(&message->body, "uds_write_rtc_zgm86", value, sizeof(value)) > 0) {
      startBMWFUDSZGM86RTCWriteProbe();
    } else if (httpGetVar(&message->body, "uds_sa_seed_can", value, sizeof(value)) > 0) {
      startBMWFUDSCANSecuritySeedProbe();
    } else if (httpGetVar(&message->body, "uds_sa_seed_can86", value, sizeof(value)) > 0) {
      startBMWFUDSCAN86SecuritySeedProbe();
    } else if (httpGetVar(&message->body, "uds_sa_seed_zgm", value, sizeof(value)) > 0) {
      startBMWFUDSZGMSecuritySeedProbe();
    } else if (httpGetVar(&message->body, "uds_sa_seed_zgm86", value, sizeof(value)) > 0) {
      startBMWFUDSZGM86SecuritySeedProbe();
    } else if (httpGetVar(&message->body, "uds_scan", value, sizeof(value)) > 0) {
      startBMWFUDSTargetScan();
    } else if (httpGetVar(&message->body, "uds_zgw_scan", value, sizeof(value)) > 0) {
      startBMWFUDSZGWTargetScan();
    } else if (httpGetVar(&message->body, "uds_direct_scan", value, sizeof(value)) > 0) {
      startBMWFUDSDirectTargetScan();
    } else if (httpGetVar(&message->body, "uds_sweep86_run", value, sizeof(value)) > 0) {
      startBMWFUDSCAN86DidSweep();
    } else if (httpGetVar(&message->body, "uds_read_d113", value, sizeof(value)) > 0) {
      startBMWFUDSReadD113();
    } else if (httpGetVar(&message->body, "uds_write_rtc_can86_prog", value, sizeof(value)) > 0) {
      startBMWFUDSCAN86RTCWriteProgSession();
    } else if (httpGetVar(&message->body, "uds_sa_seed_can86_prog", value, sizeof(value)) > 0) {
      startBMWFUDSCAN86SecuritySeedProgSession();
    } else if (httpGetVar(&message->body, "uds_routine_scan86", value, sizeof(value)) > 0) {
      startBMWFUDSCAN86RoutineScan();
    } else if (httpGetVar(&message->body, "uds_read_vin", value, sizeof(value)) > 0) {
      startBMWFUDSReadVin();
    }
    httpReply(connection, 303, "Location: /bmw-f-controls\r\nCache-Control: no-store\r\nConnection: close\r\n", "");
    connection->is_draining = 1;
    return;
  }

  httpReply(
    connection,
    200,
    "Content-Type: text/html; charset=utf-8\r\nCache-Control: no-store\r\nConnection: close\r\n",
    "<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>BMW F Controls</title>"
    "<style>"
    "body{font-family:Arial,sans-serif;margin:0;background:#f5f6f8;color:#111827}"
    "main{max-width:720px;margin:0 auto;padding:18px}"
    "section{background:#fff;border:1px solid #d9dde3;border-radius:8px;padding:16px;margin:12px 0}"
    "h1{font-size:24px;margin:0 0 12px}h2{font-size:16px;margin:0 0 12px}"
    "label{display:flex;justify-content:space-between;gap:16px;align-items:center;padding:10px 0;border-top:1px solid #eef0f3}"
    "label:first-of-type{border-top:0}.meta{color:#5b6472;font-size:13px;line-height:1.35}"
    "button{background:#1f6feb;color:white;border:0;border-radius:6px;padding:10px 14px;font-weight:700}"
    "a{color:#1f6feb;text-decoration:none}input[type=checkbox]{width:22px;height:22px}"
    "input[type=text],input[type=number]{width:96px;padding:6px;border:1px solid #cfd6df;border-radius:6px}"
    "</style></head><body><main>"
    "<h1>BMW F Controls</h1>"
    "<p class='meta'><a href='/test'>Open /test experimental dashboard</a></p>"
    "<form method='post' action='/bmw-f-controls'>"
    "<section><h2>Language and units</h2>"
    "<label><span>Send 0x291 English/metric</span><input type='checkbox' name='lang291'%s></label>"
    "<p class='meta'>The 01 12 59 payload was removed because it switches this cluster to German.</p>"
    "</section>"
    "<section><h2>Needles</h2>"
    "<label><span>Fast RPM refresh</span><input type='checkbox' name='rpm_fast'%s></label>"
    "<label><span>High resolution RPM 0x0F3</span><input type='checkbox' name='rpm_highres'%s disabled></label>"
    "<p class='meta'>Fast refresh sends RPM every 10 ms using the corrected 12-bit F-series RPM payload. Speed is sent every 20 ms.</p>"
    "</section>"
    "<section><h2>BMW F extras</h2>"
    "<label><span>LIM 0x287 fixed 110 km/h</span><input type='checkbox' name='lim287'%s></label>"
    "<label><span>Show LIM icon (0x289 speed-limiter telltale)</span><input type='checkbox' name='lim289'%s></label>"
    "<label><span>Outside temperature 0x2CA</span><input type='checkbox' name='temp2ca'%s></label>"
    "<label><span>Auto Start/Stop 0x30B</span><input type='checkbox' name='ass30b'%s></label>"
    "<label><span>Auto Hold CC-ID 58</span><input type='checkbox' name='autohold5c0'%s></label>"
    "<p class='meta'>Auto Hold is enabled after boot. Other extra frames stay disabled until enabled here.</p>"
    "</section>"
    "<section><h2>Lane departure lines (KAFAS 0x327)</h2>"
    "<label><span>Keep lane lines lit (send 0x327 always)</span><input type='checkbox' name='lane_always'%s></label>"
    "<label><span>CRC seed sweep (0x00&ndash;0xFF, ~1.5 s each)</span><input type='checkbox' name='lane_sweep'%s></label>"
    "<p class='meta'>Sends the KAFAS lane frame 0x327 every 100 ms with a rolling alive counter, on after boot. The KOMBI only draws the lines if its coding has FZG_Ausstattung &rarr; TLC_VERBAUT = aktiv &mdash; a cluster from a car without lane departure ignores 0x327 entirely. The per-message CRC seed for 0x327 is unknown; enable the sweep, watch the Serial Monitor and the cluster, and note the <code>BMW_F LANE SWEEP ... seed=0x..</code> value printed when the lines appear, then set it as the CRC seed on the <a href='/test'>/test</a> page (Lane Assist / KAFAS). Drive a simulated speed above ~70 km/h while sweeping &mdash; lane departure only activates above its threshold.</p>"
    "</section>"
    "<section><h2>Buttons</h2>"
    "<button type='submit' name='bmwf_button_bc' value='1'>BC / menu cycle</button>"
    "<p class='meta'>Sends 0x1EE press and release using the enhanced BC/menu value.</p>"
    "</section>"
    "<section><h2>TPMS CC tests</h2>"
    "<label><span>Front left tire warning CC-ID 139</span><input type='checkbox' name='tpms_fl'%s></label>"
    "<label><span>Front right tire warning CC-ID 143</span><input type='checkbox' name='tpms_fr'%s></label>"
    "<label><span>Rear left tire warning CC-ID 141</span><input type='checkbox' name='tpms_rl'%s></label>"
    "<label><span>Rear right tire warning CC-ID 140</span><input type='checkbox' name='tpms_rr'%s></label>"
    "<p class='meta'>Global TPMS CC-ID 142 is set automatically when any tire warning is enabled.</p>"
    "</section>"
    "<section><h2>TPMS bus candidates - no effect in test</h2>"
    "<label><span>0x31C TPMS candidate - tested no effect</span><input type='checkbox' name='tpms31c'%s></label>"
    "<label><span>0x31F TPMS candidate - tested no effect</span><input type='checkbox' name='tpms31f'%s></label>"
    "<label><span>0xB68 extended TPMS candidate - tested no effect</span><input type='checkbox' name='tpmsb68'%s></label>"
    "<p class='meta'>Marked as non-working for this cluster with the current 0x369 OK payload.</p>"
    "</section>"
    "<section><h2>Cluster info</h2>"
    "<p class='meta'>%s</p>"
    "<button type='submit' name='uds_read_vin' value='1'>Read full VIN (0xF190 on 0x86)</button>"
    "<p class='meta'>Reads the 17-char VIN from the cluster via UDS. Click, wait ~1 s, then refresh this page (or watch the Serial Monitor) to see the result. It may be all zeros if the cluster's VIN was never programmed.</p>"
    "</section>"
    "<button type='submit'>Save</button> &nbsp; <a href='/'>Back to dashboard</a>"
    "</form></main></body></html>",
    checkedAttr(options.languageAndUnits291),
    checkedAttr(options.fastRPMRefresh),
    checkedAttr(options.highResolutionRPM),
    checkedAttr(options.lim287),
    checkedAttr(options.lim289Icon),
    checkedAttr(options.outsideTemperature2CA),
    checkedAttr(options.autoStartStop30B),
    checkedAttr(options.autoHold5C0),
    checkedAttr(options.laneAssistAlwaysOn),
    checkedAttr(options.laneSweep),
    checkedAttr(options.tpmsWarningFL5C0),
    checkedAttr(options.tpmsWarningFR5C0),
    checkedAttr(options.tpmsWarningRL5C0),
    checkedAttr(options.tpmsWarningRR5C0),
    checkedAttr(options.tpmsCandidate31C),
    checkedAttr(options.tpmsCandidate31F),
    checkedAttr(options.tpmsCandidateB68),
    bmwFUDSClockProbeStatusText());
  connection->is_draining = 1;
}

// Serves the in-RAM UDS sweep log as a downloadable .txt file.
void webDashboardBMWFUDSLog(HttpConnection *connection, int event, void *eventData) {
  if (event != HTTP_EVENT_REQUEST) {
    return;
  }
  httpSendDownload(connection, "bmw_f_uds_sweep.txt", bmwFUDSLogBuffer, bmwFUDSLogLen);
  connection->is_draining = 1;
}

void webDashboardBMWFTestControls(HttpConnection *connection, int event, void *eventData) {
  if (event != HTTP_EVENT_REQUEST) {
    return;
  }

  HttpMessage *message = (HttpMessage *)eventData;
  BMWFTestOptions options = cluster.getTestOptions();

  if (httpStringCompare(message->method, httpString("POST")) == 0) {
    char value[8];
    options.laneAssist327 = httpGetVar(&message->body, "lane327", value, sizeof(value)) > 0;
    options.laneAssistScanIds = httpGetVar(&message->body, "lane_scan", value, sizeof(value)) > 0;
    options.laneAssistId = (uint16_t)httpUIntVar(message, "lane_id", options.laneAssistId, 0, 0x7FF);
    options.laneAssistLeft = (uint8_t)httpUIntVar(message, "lane_left", options.laneAssistLeft, 0, 255);
    options.laneAssistRight = (uint8_t)httpUIntVar(message, "lane_right", options.laneAssistRight, 0, 255);
    options.laneAssistCrcSeed = (uint8_t)httpUIntVar(message, "lane_crc", options.laneAssistCrcSeed, 0, 255);
    options.frame1D6ButtonMode = (uint8_t)httpUIntVar(message, "frame1d6_mode", options.frame1D6ButtonMode, 0, 2);
    options.frame327RawA214 = httpGetVar(&message->body, "frame327_raw", value, sizeof(value)) > 0;
    options.frame349SplitFuel = httpGetVar(&message->body, "frame349_split", value, sizeof(value)) > 0;
    options.frame349FuelLeft = (uint16_t)httpUIntVar(message, "frame349_left", options.frame349FuelLeft, 0, 0xFFFF);
    options.frame349FuelRight = (uint16_t)httpUIntVar(message, "frame349_right", options.frame349FuelRight, 0, 0xFFFF);
    uint8_t fuelDlc = (uint8_t)httpUIntVar(message, "frame349_dlc", options.frame349FuelDlc, 4, 8);
    options.frame349FuelDlc = fuelDlc == 4 || fuelDlc == 5 || fuelDlc == 8 ? fuelDlc : 8;
    options.frame33BRawAcc = httpGetVar(&message->body, "frame33b_raw", value, sizeof(value)) > 0;
    options.frame21A12Dlc8 = httpGetVar(&message->body, "frame21a_12", value, sizeof(value)) > 0;
    options.frame2E4SteeringButton = httpGetVar(&message->body, "frame2e4_button", value, sizeof(value)) > 0;
    options.frame1EERawLeftMenu = httpGetVar(&message->body, "frame1ee_menu", value, sizeof(value)) > 0;
    options.frame393 = httpGetVar(&message->body, "frame393", value, sizeof(value)) > 0;
    options.frame1B3 = httpGetVar(&message->body, "frame1b3", value, sizeof(value)) > 0;
    options.frame2C5 = httpGetVar(&message->body, "frame2c5", value, sizeof(value)) > 0;
    options.frame381 = httpGetVar(&message->body, "frame381", value, sizeof(value)) > 0;
    options.frame2C3 = httpGetVar(&message->body, "frame2c3", value, sizeof(value)) > 0;
    options.frame3A0 = httpGetVar(&message->body, "frame3a0", value, sizeof(value)) > 0;
    options.frame581 = httpGetVar(&message->body, "frame581", value, sizeof(value)) > 0;
    options.frame0AB = httpGetVar(&message->body, "frame0ab", value, sizeof(value)) > 0;
    options.frame130Icm = httpGetVar(&message->body, "frame130", value, sizeof(value)) > 0;
    options.frame0C4Szl = httpGetVar(&message->body, "frame0c4", value, sizeof(value)) > 0;
    options.frame0AAWheelSpeed = httpGetVar(&message->body, "frame0aa", value, sizeof(value)) > 0;
    options.frame3D0Battery = httpGetVar(&message->body, "frame3d0", value, sizeof(value)) > 0;
    options.frame368Rdc = httpGetVar(&message->body, "frame368rdc", value, sizeof(value)) > 0;
    options.frame368RdcB1 = (uint8_t)httpUIntVar(message, "frame368_b1", options.frame368RdcB1, 0, 255);
    options.frame368RdcB2 = (uint8_t)httpUIntVar(message, "frame368_b2", options.frame368RdcB2, 0, 255);
    options.frame368RdcB3 = (uint8_t)httpUIntVar(message, "frame368_b3", options.frame368RdcB3, 0, 255);
    options.frame368RdcCrcSeed = (uint8_t)httpUIntVar(message, "frame368_crc", options.frame368RdcCrcSeed, 0, 255);
    options.frame369Tpms = httpGetVar(&message->body, "frame369tpms", value, sizeof(value)) > 0;
    options.frame369TpmsB1 = (uint8_t)httpUIntVar(message, "frame369_b1", options.frame369TpmsB1, 0, 255);
    options.frame369TpmsB2 = (uint8_t)httpUIntVar(message, "frame369_b2", options.frame369TpmsB2, 0, 255);
    options.frame369TpmsB3 = (uint8_t)httpUIntVar(message, "frame369_b3", options.frame369TpmsB3, 0, 255);
    options.cc50TyreMonitoringFailure = httpGetVar(&message->body, "cc50", value, sizeof(value)) > 0;
    options.cc63TyreFailure = httpGetVar(&message->body, "cc63", value, sizeof(value)) > 0;
    options.cc144TPMSFault = httpGetVar(&message->body, "cc144", value, sizeof(value)) > 0;
    options.cc145TPMSDeactivated = httpGetVar(&message->body, "cc145", value, sizeof(value)) > 0;
    options.cc147TyrePressureLoss = httpGetVar(&message->body, "cc147", value, sizeof(value)) > 0;
    options.cc149TPMSFailure = httpGetVar(&message->body, "cc149", value, sizeof(value)) > 0;
    options.cc192TPMSInitialising = httpGetVar(&message->body, "cc192", value, sizeof(value)) > 0;
    options.cc28OilLevel = httpGetVar(&message->body, "cc28", value, sizeof(value)) > 0;
    options.cc36DscOff = httpGetVar(&message->body, "cc36", value, sizeof(value)) > 0;
    options.cc39EngineOverheat = httpGetVar(&message->body, "cc39", value, sizeof(value)) > 0;
    options.time39E6waCandidate = httpGetVar(&message->body, "time39e_6wa", value, sizeof(value)) > 0;
    options.clearCC167ResetClock = httpGetVar(&message->body, "clearcc167", value, sizeof(value)) > 0;
    options.cc299ChassisWarning = httpGetVar(&message->body, "cc299", value, sizeof(value)) > 0;
    options.cc196SosWarning = httpGetVar(&message->body, "cc196", value, sizeof(value)) > 0;
    options.rawCCActive = httpGetVar(&message->body, "rawcc_active", value, sizeof(value)) > 0;
    options.rawCCId = (uint8_t)httpUIntVar(message, "rawcc_id", options.rawCCId, 0, 255);
    bool runNeedleSweep = httpGetVar(&message->body, "needle_sweep", value, sizeof(value)) > 0;
    cluster.setTestOptions(options);
    if (runNeedleSweep) {
      cluster.startNeedleSweep();
    }

    httpReply(connection, 303, "Location: /test\r\nCache-Control: no-store\r\nConnection: close\r\n", "");
    connection->is_draining = 1;
    return;
  }

  httpReply(
    connection,
    200,
    "Content-Type: text/html; charset=utf-8\r\nCache-Control: no-store\r\nConnection: close\r\n",
    "<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>BMW F Test</title>"
    "<style>"
    "body{font-family:Arial,sans-serif;margin:0;background:#f5f6f8;color:#111827}"
    "main{max-width:760px;margin:0 auto;padding:18px}"
    "section{background:#fff;border:1px solid #d9dde3;border-radius:8px;padding:16px;margin:12px 0}"
    "h1{font-size:24px;margin:0 0 12px}h2{font-size:16px;margin:0 0 12px}"
    "label{display:flex;justify-content:space-between;gap:16px;align-items:center;padding:10px 0;border-top:1px solid #eef0f3}"
    "label:first-of-type{border-top:0}.meta{color:#5b6472;font-size:13px;line-height:1.35}"
    "input[type=checkbox]{width:22px;height:22px}input[type=text],input[type=number],select{width:116px;padding:6px;border:1px solid #cfd6df;border-radius:6px;background:#fff}"
    "button{background:#1f6feb;color:white;border:0;border-radius:6px;padding:10px 14px;font-weight:700}"
    "a{color:#1f6feb;text-decoration:none}"
    "</style></head><body><main>"
    "<h1>BMW F Test</h1>"
    "<p class='meta'>All test candidates are off after boot. Enable one small group at a time.</p>"
    "<form method='post' action='/test'>"
    "<section><h2>Spotify / Voicemeeter VU needles</h2>"
    "<p class='meta'>Use the speedometer as the left-channel VU meter and the tachometer as the right-channel VU meter. Audio is analysed locally in the browser.</p>"
    "<a href='/test/vu'>Open Spotify / VU needles</a>"
    "</section>"
    "<section><h2>Lane Assist / KAFAS</h2>"
    "<label><span>Send lane assist on selected ID</span><input type='checkbox' name='lane327'%s></label>"
    "<label><span>Send lane assist on scan ID set</span><input type='checkbox' name='lane_scan'%s></label>"
    "<label><span>Lane CAN ID</span><input type='text' name='lane_id' value='0x%03X'></label>"
    "<label><span>Left lane byte</span><input type='number' min='0' max='255' name='lane_left' value='%u'></label>"
    "<label><span>Right lane byte</span><input type='number' min='0' max='255' name='lane_right' value='%u'></label>"
    "<label><span>CRC final XOR/seed</span><input type='text' name='lane_crc' value='0x%02X'></label>"
    "<p class='meta'>Source candidate: 0x327 DLC 4, payload CRC + 0x50|counter + left + right, seed 0x27. Scan set: 0x327, 0x345, 0x18A, 0x239, 0x1A6, 0x31B, 0x317, 0x337, 0x347.</p>"
    "</section>"
    "<section><h2>Raw F10 Source Candidates</h2>"
    "<label><span>0x1D6 button payload</span><select name='frame1d6_mode'><option value='0'%s>Off</option><option value='1'%s>C8 0C</option><option value='2'%s>C4 0C</option></select></label>"
    "<label><span>0x327 raw A2 14 00... DLC 8</span><input type='checkbox' name='frame327_raw'%s></label>"
    "<label><span>0x349 split fuel sender test</span><input type='checkbox' name='frame349_split'%s></label>"
    "<label><span>0x349 left sender</span><input type='text' name='frame349_left' value='0x%04X'></label>"
    "<label><span>0x349 right sender</span><input type='text' name='frame349_right' value='0x%04X'></label>"
    "<label><span>0x349 DLC</span><select name='frame349_dlc'><option value='4'%s>4 bytes</option><option value='5'%s>5 bytes</option><option value='8'%s>8 bytes</option></select></label>"
    "<label><span>0x33B raw ACC byte 3 = FF, DLC 8</span><input type='checkbox' name='frame33b_raw'%s></label>"
    "<label><span>0x21A source lights byte 2 = 12, DLC 8</span><input type='checkbox' name='frame21a_12'%s></label>"
    "<label><span>Hold 0x2E4 alternative steering button</span><input type='checkbox' name='frame2e4_button'%s></label>"
    "<label><span>Hold raw 0x1EE C4 0C left menu</span><input type='checkbox' name='frame1ee_menu'%s></label>"
    "<p class='meta'>These reproduce the supplied sketches exactly. A raw test temporarily suppresses the normal sender on the same CAN ID. The 0x21A test follows the live main/high-beam state; fog-light bits are not included in that source payload.</p>"
    "</section>"
    "<section><h2>Module Frames</h2>"
    "<label><span>0x393 fixed frame</span><input type='checkbox' name='frame393'%s></label>"
    "<label><span>0x1B3 CRC keepalive</span><input type='checkbox' name='frame1b3'%s></label>"
    "<label><span>0x2C5 CRC keepalive</span><input type='checkbox' name='frame2c5'%s></label>"
    "<label><span>0x381 fixed 79 20</span><input type='checkbox' name='frame381'%s></label>"
    "<label><span>0x2C3 SOS/status candidate - working, default ON</span><input type='checkbox' name='frame2c3'%s></label>"
    "<label><span>0x3A0 vehicle status candidate</span><input type='checkbox' name='frame3a0'%s></label>"
    "<label><span>0x581 seatbelt candidate</span><input type='checkbox' name='frame581'%s></label>"
    "<label><span>0x0AB airbag candidate</span><input type='checkbox' name='frame0ab'%s></label>"
    "</section>"
    "<section><h2>Enhanced Chassis Candidates</h2>"
    "<label><span>0x130 ICM alive candidate</span><input type='checkbox' name='frame130'%s></label>"
    "<label><span>0x0C4 SZL steering angle candidate</span><input type='checkbox' name='frame0c4'%s></label>"
    "<label><span>0x0AA wheel speed candidate</span><input type='checkbox' name='frame0aa'%s></label>"
    "<label><span>0x3D0 power/battery candidate</span><input type='checkbox' name='frame3d0'%s></label>"
    "<p class='meta'>From enhanced chassis module block. All four are off after boot.</p>"
    "</section>"
    "<section><h2>TPMS / RDC Bus Candidate</h2>"
    "<label><span>0x368 Status Tyre RDC candidate - diag says missing, default ON</span><input type='checkbox' name='frame368rdc'%s></label>"
    "<label><span>0x368 byte 1</span><input type='text' name='frame368_b1' value='0x%02X'></label>"
    "<label><span>0x368 byte 2</span><input type='text' name='frame368_b2' value='0x%02X'></label>"
    "<label><span>0x368 byte 3</span><input type='text' name='frame368_b3' value='0x%02X'></label>"
    "<label><span>0x368 CRC final XOR/seed</span><input type='text' name='frame368_crc' value='0x%02X'></label>"
    "<p class='meta'>Diagnostic faults E114A0/D018E8 point to missing Status Tyre RDC 0x368. Default test sends CRC(seed) + F0|counter A2 A0 A0, DLC 5.</p>"
    "<label><span>Override normal 0x369 TPMS/RDC payload</span><input type='checkbox' name='frame369tpms'%s></label>"
    "<label><span>0x369 byte 1</span><input type='text' name='frame369_b1' value='0x%02X'></label>"
    "<label><span>0x369 byte 2</span><input type='text' name='frame369_b2' value='0x%02X'></label>"
    "<label><span>0x369 byte 3</span><input type='text' name='frame369_b3' value='0x%02X'></label>"
    "<p class='meta'>Default payload from F10 Serial.h is CRC(seed 0xC5) + F0|counter A2 A0 A0, DLC 5. Firmware already sends 0x369 normally; this test only changes the three data bytes.</p>"
    "</section>"
    "<section><h2>TPMS CC-ID Presets</h2>"
    "<label><span>CC-ID 50 tyre monitoring failure</span><input type='checkbox' name='cc50'%s></label>"
    "<label><span>CC-ID 63 tyre failure / puncture</span><input type='checkbox' name='cc63'%s></label>"
    "<label><span>CC-ID 144 TPMS fault</span><input type='checkbox' name='cc144'%s></label>"
    "<label><span>CC-ID 145 TPMS deactivated</span><input type='checkbox' name='cc145'%s></label>"
    "<label><span>CC-ID 147 tyre pressure loss</span><input type='checkbox' name='cc147'%s></label>"
    "<label><span>CC-ID 149 TPMS failure</span><input type='checkbox' name='cc149'%s></label>"
    "<label><span>CC-ID 192 TPMS initialising</span><input type='checkbox' name='cc192'%s></label>"
    "<p class='meta'>From local Autobulbs CC-ID list. Higher tyre IDs such as 265, 327, 334, 384, 608-611 are not directly representable as the current 8-bit raw alert byte.</p>"
    "</section>"
    "<section><h2>Clock / Date Warning</h2>"
    "<label><span>0x39E 6WA Python candidate</span><input type='checkbox' name='time39e_6wa'%s></label>"
    "<p class='meta'>Sends 0x39E once per second as HH MM SS DD yearMSB yearLSB 00 F2. While enabled, the normal 0x39E clock frame is skipped to avoid mixed payloads.</p>"
    "<label><span>Clear CC-ID 167 Reset Clock - default ON</span><input type='checkbox' name='clearcc167'%s></label>"
    "<p class='meta'>Sends 0x5C0 clear for CC-ID 167 every update. This tests whether the yellow exclamation is only the Reset Clock check-control warning.</p>"
    "</section>"
    "<section><h2>Other CC-ID Tests</h2>"
    "<label><span>CC-ID 28 add engine oil</span><input type='checkbox' name='cc28'%s></label>"
    "<label><span>CC-ID 36 DSC deactivated</span><input type='checkbox' name='cc36'%s></label>"
    "<label><span>CC-ID 39 engine overheated</span><input type='checkbox' name='cc39'%s></label>"
    "<label><span>CC-ID 299 chassis warning byte wrap</span><input type='checkbox' name='cc299'%s></label>"
    "<label><span>CC-ID 196 SOS warning</span><input type='checkbox' name='cc196'%s></label>"
    "<label><span>Raw CC-ID active</span><input type='checkbox' name='rawcc_active'%s></label>"
    "<label><span>Raw CC-ID 0-255</span><input type='number' min='0' max='255' name='rawcc_id' value='%u'></label>"
    "<p class='meta'>Raw CC-ID uses 0x5C0 with byte 2 as the selected 8-bit value and byte 4 0x29 while active.</p>"
    "</section>"
    "<section><h2>Needle Sweep</h2>"
    "<p class='meta'>With ignition on, moves speed, RPM, fuel and temperature needles linearly to maximum in 3 seconds, then back to zero in 3 seconds.</p>"
    "<button type='submit' name='needle_sweep' value='1'>Run smooth needle sweep</button>"
    "</section>"
    "<button type='submit'>Save</button> &nbsp; <a href='/bmw-f-controls'>Back to BMW F controls</a>"
    "</form></main></body></html>",
    checkedAttr(options.laneAssist327),
    checkedAttr(options.laneAssistScanIds),
    (unsigned int)options.laneAssistId,
    (unsigned int)options.laneAssistLeft,
    (unsigned int)options.laneAssistRight,
    (unsigned int)options.laneAssistCrcSeed,
    selectedAttr(options.frame1D6ButtonMode == 0),
    selectedAttr(options.frame1D6ButtonMode == 1),
    selectedAttr(options.frame1D6ButtonMode == 2),
    checkedAttr(options.frame327RawA214),
    checkedAttr(options.frame349SplitFuel),
    (unsigned int)options.frame349FuelLeft,
    (unsigned int)options.frame349FuelRight,
    selectedAttr(options.frame349FuelDlc == 4),
    selectedAttr(options.frame349FuelDlc == 5),
    selectedAttr(options.frame349FuelDlc == 8),
    checkedAttr(options.frame33BRawAcc),
    checkedAttr(options.frame21A12Dlc8),
    checkedAttr(options.frame2E4SteeringButton),
    checkedAttr(options.frame1EERawLeftMenu),
    checkedAttr(options.frame393),
    checkedAttr(options.frame1B3),
    checkedAttr(options.frame2C5),
    checkedAttr(options.frame381),
    checkedAttr(options.frame2C3),
    checkedAttr(options.frame3A0),
    checkedAttr(options.frame581),
    checkedAttr(options.frame0AB),
    checkedAttr(options.frame130Icm),
    checkedAttr(options.frame0C4Szl),
    checkedAttr(options.frame0AAWheelSpeed),
    checkedAttr(options.frame3D0Battery),
    checkedAttr(options.frame368Rdc),
    (unsigned int)options.frame368RdcB1,
    (unsigned int)options.frame368RdcB2,
    (unsigned int)options.frame368RdcB3,
    (unsigned int)options.frame368RdcCrcSeed,
    checkedAttr(options.frame369Tpms),
    (unsigned int)options.frame369TpmsB1,
    (unsigned int)options.frame369TpmsB2,
    (unsigned int)options.frame369TpmsB3,
    checkedAttr(options.cc50TyreMonitoringFailure),
    checkedAttr(options.cc63TyreFailure),
    checkedAttr(options.cc144TPMSFault),
    checkedAttr(options.cc145TPMSDeactivated),
    checkedAttr(options.cc147TyrePressureLoss),
    checkedAttr(options.cc149TPMSFailure),
    checkedAttr(options.cc192TPMSInitialising),
    checkedAttr(options.time39E6waCandidate),
    checkedAttr(options.clearCC167ResetClock),
    checkedAttr(options.cc28OilLevel),
    checkedAttr(options.cc36DscOff),
    checkedAttr(options.cc39EngineOverheat),
    checkedAttr(options.cc299ChassisWarning),
    checkedAttr(options.cc196SosWarning),
    checkedAttr(options.rawCCActive),
    (unsigned int)options.rawCCId);
  connection->is_draining = 1;
}
#endif

// Serial JSON parsing
JsonDocument doc;

int buildMonthFromName(const char *monthName) {
  const char *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
  const char *month = strstr(months, monthName);
  if (month == nullptr) {
    return 0;
  }
  return ((month - months) / 3) + 1;
}

time_t firmwareBuildTime() {
  char monthName[4] = {};
  int day = 0;
  int year = 0;
  int hour = 0;
  int minute = 0;
  int second = 0;

  sscanf(__DATE__, "%3s %d %d", monthName, &day, &year);
  sscanf(__TIME__, "%d:%d:%d", &hour, &minute, &second);

  int month = buildMonthFromName(monthName);
  if (month == 0 || day == 0 || year == 0) {
    return 0;
  }

  struct tm buildTime = {};
  buildTime.tm_year = year - 1900;
  buildTime.tm_mon = month - 1;
  buildTime.tm_mday = day;
  buildTime.tm_hour = hour;
  buildTime.tm_min = minute;
  buildTime.tm_sec = second;
  buildTime.tm_isdst = -1;
  return mktime(&buildTime);
}

void setSystemTime(time_t newTime) {
  struct timeval tv = {};
  tv.tv_sec = newTime;
  tv.tv_usec = 0;
  settimeofday(&tv, nullptr);
}

void seedSystemTimeFromFirmwareBuild() {
  setenv("TZ", TIME_ZONE, 1);
  tzset();

  time_t buildTime = firmwareBuildTime();
  if (buildTime < 1609459200) {
    Serial.println("Firmware build time fallback is invalid");
    return;
  }

  setSystemTime(buildTime);
  Serial.print("System time seeded from firmware build: ");
  Serial.println(ctime(&buildTime));
}

void setSystemTimeFromJson(JsonDocument &json) {
  int year = json["year"] | 0;
  int month = json["month"] | 0;
  int day = json["day"] | 0;
  int hour = json["hour"] | 0;
  int minute = json["minute"] | 0;
  int second = json["second"] | 0;

  if (year < 2021 || month < 1 || month > 12 || day < 1 || day > 31 || hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) {
    Serial.println("Manual time rejected; use action 11 with year/month/day/hour/minute/second");
    return;
  }

  struct tm manualTime = {};
  manualTime.tm_year = year - 1900;
  manualTime.tm_mon = month - 1;
  manualTime.tm_mday = day;
  manualTime.tm_hour = hour;
  manualTime.tm_min = minute;
  manualTime.tm_sec = second;
  manualTime.tm_isdst = -1;

  time_t epoch = mktime(&manualTime);
  if (epoch < 1609459200) {
    Serial.println("Manual time conversion failed");
    return;
  }

  setSystemTime(epoch);
  Serial.print("Manual system time set: ");
  Serial.println(ctime(&epoch));
}

void printCanHexByte(uint8_t value) {
  if (value < 0x10) {
    Serial.print('0');
  }
  Serial.print(value, HEX);
}

void logBMWFCanRxFrame(unsigned long id, unsigned char len, const unsigned char *data) {
#if BMW_F_LOG_MODE != 0
  return;  // suppressed — set BMW_F_LOG_MODE 0 for all RX frames
#else
  unsigned long baseId = id & 0x1FFFFFFF;

  if ((id & 0x80000000) == 0x80000000) {
    snprintf(canRxMsgString, sizeof(canRxMsgString), "CAN RX EXT 0x%.8lX dlc=%u data=", baseId, len);
  } else {
    snprintf(canRxMsgString, sizeof(canRxMsgString), "CAN RX STD 0x%.3lX dlc=%u data=", baseId, len);
  }

  Serial.print(canRxMsgString);

  if ((id & 0x40000000) == 0x40000000) {
    Serial.print("REMOTE");
  } else {
    for (byte i = 0; i < len && i < 8; i++) {
      if (i > 0) {
        Serial.print(' ');
      }
      printCanHexByte(data[i]);
    }
  }

  Serial.println();
#endif
}

void logBMWFObservedClockCandidateFrame(unsigned long id, unsigned char len, const unsigned char *data) {
#if BMW_F_LOG_MODE == 1 || BMW_F_LOG_MODE == 3
  return;  // suppressed in UDS/ERRORS mode
#else
  unsigned long baseId = id & 0x1FFFFFFF;
  if (baseId != 0x393 && baseId != 0x328) {
    return;
  }

  static unsigned long last393Ms = 0;
  static unsigned long last328Ms = 0;
  unsigned long nowMs = millis();
  unsigned long *lastMs = baseId == 0x393 ? &last393Ms : &last328Ms;
  unsigned long deltaMs = *lastMs == 0 ? 0 : nowMs - *lastMs;
  *lastMs = nowMs;

  Serial.print("BMW_F WATCH 0x");
  Serial.print(baseId, HEX);
  Serial.print(" dt=");
  Serial.print(deltaMs);
  Serial.print("ms data=");
  for (uint8_t i = 0; i < len && i < 8; i++) {
    if (i > 0) {
      Serial.print(' ');
    }
    printCanHexByte(data[i]);
  }
  Serial.println();
#endif
}

void logBMWFTimeRxFrame(unsigned long id, unsigned char len, const unsigned char *data) {
#if BMW_F_LOG_MODE == 1 || BMW_F_LOG_MODE == 3
  return;  // suppressed in UDS/ERRORS mode
#else
  unsigned long baseId = id & 0x1FFFFFFF;
  if (baseId != 0x2F8 && baseId != 0x291 && baseId != 0x39E && baseId != 0x3F1 && (baseId < 0x500 || baseId > 0x5FF)) {
    return;
  }

  static unsigned long lastLog560Ms = 0;
  static unsigned long lastLog5E0Ms = 0;
  unsigned long nowMs = millis();
  if (baseId == 0x560 && nowMs - lastLog560Ms < 5000) {
    return;
  }
  if (baseId == 0x5E0 && nowMs - lastLog5E0Ms < 1000) {
    return;
  }
  if (baseId == 0x560) {
    lastLog560Ms = nowMs;
  } else if (baseId == 0x5E0) {
    lastLog5E0Ms = nowMs;
  }

  Serial.print("BMW_F RX 0x");
  if (baseId < 0x100) {
    Serial.print('0');
  }
  if (baseId < 0x10) {
    Serial.print('0');
  }
  Serial.print(baseId, HEX);
  Serial.print(" data=");
  for (uint8_t i = 0; i < len && i < 8; i++) {
    if (i > 0) {
      Serial.print(' ');
    }
    printCanHexByte(data[i]);
  }
  Serial.println();
#endif
}

const uint8_t BMWF_UDS_CAN_DIAG_ID = 0xF1;
const uint8_t BMWF_UDS_ETH_DIAG_ID = 0xF4;
const uint8_t BMWF_UDS_ZGM_INTERNAL_ID = 0xF0;
const uint8_t BMWF_UDS_TESTER_ID = BMWF_UDS_ETH_DIAG_ID;
const uint8_t BMWF_UDS_ZGW_ID = 0x10;
const uint8_t BMWF_UDS_DEFAULT_KOMBI_ID = 0x60;
const uint8_t BMWF_UDS_OBSERVED_ENDPOINT_ID = 0x86;
const uint8_t BMWF_UDS_SCAN_START_ID = 0x00;
const uint8_t BMWF_UDS_SCAN_END_ID = 0x7F;
const unsigned long BMWF_UDS_RESPONSE_TIMEOUT_MS = 700;
const unsigned long BMWF_UDS_LONG_WRITE_RESPONSE_TIMEOUT_MS = 5000;
// ISO-TP FlowControl tuning. BMW modules expect a full 8-byte (DLC=8) frame, so
// the FC is padded. BlockSize 0 = send all consecutive frames, STmin spaces them
// out so the ESP32 can drain each one. Raise STmin (e.g. 0x0A / 0x14) if multi-
// frame reads still drop bytes on a busy bus.
const uint8_t BMWF_UDS_FC_BLOCKSIZE = 0x00;
const uint8_t BMWF_UDS_FC_STMIN = 0x05;  // 5 ms between consecutive frames
// Max CAN frames drained from the MCP2515 per loop() pass (prevents a busy bus
// from starving the rest of the loop while still emptying both RX buffers).
const uint8_t BMWF_CAN_RX_DRAIN_LIMIT = 16;
const unsigned long BMWF_UDS_SCAN_RESPONSE_TIMEOUT_MS = 160;
const uint16_t BMWF_UDS_READ_DIDS[] = {
  0xF186,  // ActiveDiagnosticSession - safe communication test
  0xF190,  // VIN - identifies which module/car 0x86 belongs to (ASCII)
  0xF18C,  // ECU serial number (ASCII) - module identity
  0xF197,  // ECU name / application data on many BMW modules
  0x0404,  // Clock candidate from notes
  0x1704,  // Observed passive positive response from source 0x86
  0x1802,  // RTC write candidate, also useful to read
  0x1801,  // RTC write candidate, also useful to read
  0x6030,  // Clock candidate from notes
  0xF1A0,  // BMW vendor-specific clock candidate from notes
  0x0600,  // Generic clock candidate from notes
  0x2501,  // Additional time-like DID candidate from log analysis
  0x2500,
  0x4102,
  0x4900,
  0x4901,
  0x0D04,
  0x0F80
};
const uint16_t BMWF_UDS_RTC_WRITE_DIDS[] = {
  0xD113,  // PRIME cluster RTC candidate: sweep read back FD FD FD 1E 04 07 E9 E6
           // = invalid hh:mm:ss + date 30-04-2025 + 16-bit year 0x07E9. Tried first.
  0x1704,
  0x1802,
  0x1801
};

// --- Full ReadDID sweep on the one module that answers (0x86 via F1) ---
// Brute-forces 22 0000 .. 22 FFFF, logs every positive (0x62) DID with its full
// payload and every "interesting" negative (any NRC except 0x31 requestOutOfRange,
// i.e. DIDs that exist but are protected/conditional). Findings go to an in-RAM
// text log downloadable at /bmw-f-uds-log.txt so it can be sent for analysis.
const uint8_t BMWF_UDS_SWEEP_SOURCE_ID = BMWF_UDS_CAN_DIAG_ID;           // 0xF1 tester
const uint8_t BMWF_UDS_SWEEP_TARGET_ID = BMWF_UDS_OBSERVED_ENDPOINT_ID;  // 0x86 module
const uint32_t BMWF_UDS_SWEEP_DID_START = 0x0000;
const uint32_t BMWF_UDS_SWEEP_DID_END = 0xFFFF;
// Per-DID fallback timeout. The module NRC-rejects unknown DIDs almost instantly,
// so the sweep normally advances at response speed; this only fires if it goes
// quiet. ~5-7 min for a full pass in practice.
const unsigned long BMWF_UDS_SWEEP_RESPONSE_TIMEOUT_MS = 60;

// In-RAM downloadable log buffer. 32 KB so a full sweep fits even with a healthy
// number of positives; the noisy contiguous NRC blocks (0x31, 0x22) are counted,
// not logged per-line, so this is plenty.
const uint16_t BMWF_UDS_LOG_CAPACITY = 32768;
char bmwFUDSLogBuffer[BMWF_UDS_LOG_CAPACITY];
uint16_t bmwFUDSLogLen = 0;
bool bmwFUDSLogTruncated = false;

void bmwFUDSLogReset() {
  bmwFUDSLogLen = 0;
  bmwFUDSLogTruncated = false;
  bmwFUDSLogBuffer[0] = '\0';
}

void bmwFUDSLogLine(const char *line) {
  uint16_t n = (uint16_t)strlen(line);
  if ((uint32_t)bmwFUDSLogLen + n + 2 >= BMWF_UDS_LOG_CAPACITY) {
    bmwFUDSLogTruncated = true;
    return;
  }
  memcpy(&bmwFUDSLogBuffer[bmwFUDSLogLen], line, n);
  bmwFUDSLogLen += n;
  bmwFUDSLogBuffer[bmwFUDSLogLen++] = '\n';
  bmwFUDSLogBuffer[bmwFUDSLogLen] = '\0';
}

// Heuristic flag: does a DID value look like it carries a calendar date?
// Conservative on purpose (low false positives) - the full payload is logged
// regardless, this just tags likely candidates for a human to look at first.
bool bmwFUDSValueLooksDateLike(const uint8_t *value, uint16_t n) {
  for (uint16_t i = 0; i < n; i++) {
    if (i + 1 < n) {
      uint16_t w = ((uint16_t)value[i] << 8) | value[i + 1];
      if (w >= 0x07D0 && w <= 0x07F3) return true;  // binary year 2000..2035 (0x07D0=2000 = unset-RTC default)
    }
    if (value[i] >= 0x24 && value[i] <= 0x29) {                                    // BCD year 24..29 (2024..2029)
      if (i + 1 < n && value[i + 1] >= 0x01 && value[i + 1] <= 0x12) return true;  // +BCD month
      if (i >= 1 && value[i - 1] >= 0x01 && value[i - 1] <= 0x12) return true;
    }
  }
  return false;
}

enum BMWFUDSProbeState {
  BMWF_UDS_IDLE,
  BMWF_UDS_WAIT_SESSION,
  BMWF_UDS_WAIT_DEFAULT_SESSION,
  BMWF_UDS_WAIT_DID,
  BMWF_UDS_WAIT_WRITE_FC,
  BMWF_UDS_WAIT_WRITE_DID,
  BMWF_UDS_WAIT_SECURITY_SEED,
  BMWF_UDS_SCAN_WAIT_TARGET,
  BMWF_UDS_SWEEP_WAIT_DID,
  BMWF_UDS_WAIT_SINGLE_READ,
  BMWF_UDS_DONE
};

BMWFUDSProbeState bmwFUDSProbeState = BMWF_UDS_IDLE;
uint8_t bmwFUDSProbeDidIndex = 0;
uint8_t bmwFUDSTargetId = BMWF_UDS_DEFAULT_KOMBI_ID;
uint8_t bmwFUDSRequestSourceId = BMWF_UDS_TESTER_ID;
uint8_t bmwFUDSScanTargetId = BMWF_UDS_SCAN_START_ID;
uint8_t bmwFUDSScanFoundCount = 0;
bool bmwFUDSDirectAddressing = false;
bool bmwFUDSSkipExtendedSession = false;
unsigned long bmwFUDSProbeDeadlineMs = 0;
bool bmwFUDSRTCWriteMode = false;
bool bmwFUDSSecuritySeedMode = false;
bool bmwFUDSSweepMode = false;            // full ReadDID sweep in progress
uint32_t bmwFUDSSweepDid = 0;             // current DID (0x0000..0xFFFF; uint32 to detect end)
uint16_t bmwFUDSSweepFoundCount = 0;      // positive 0x62 responses
uint16_t bmwFUDSSweepInterestingNrc = 0;  // rare negatives (e.g. 0x33 security) - logged per line
uint16_t bmwFUDSSweepConditionsNrc = 0;   // 0x22 conditionsNotCorrect - counted only (whole blocks of these)
bool bmwFUDSSweepIsRoutine = false;       // true = RoutineControl scan (31 03 = read-only), false = ReadDID sweep (22)
uint8_t bmwFUDSWritePayload[24] = {};
uint8_t bmwFUDSWritePayloadLen = 0;
uint8_t bmwFUDSWriteNextIndex = 0;
uint8_t bmwFUDSWriteSeq = 1;
uint16_t bmwFUDSCurrentWriteDid = 0;
uint8_t bmwFUDSSessionSubfunction = 0x03; // DiagnosticSessionControl subfunction: 0x03 extended (default), 0x02 programming
char bmwFUDSProbeStatus[220] = "UDS probe idle. Read probes send 0x10/0x22; RTC write buttons send 0x2E only when clicked.";

bool bmwFUDSActiveAssemblyActive = false;
uint8_t bmwFUDSActiveAssemblySourceId = 0;
uint8_t bmwFUDSActiveAssemblyTargetId = 0;
uint8_t bmwFUDSActiveAssemblyExpectedSeq = 1;
uint16_t bmwFUDSActiveAssemblyExpectedLen = 0;
uint16_t bmwFUDSActiveAssemblyReceivedLen = 0;
uint8_t bmwFUDSActiveAssemblyPayload[200] = {};

bool bmwFUDSPassiveAssemblyActive = false;
uint8_t bmwFUDSPassiveAssemblySourceId = 0;
uint8_t bmwFUDSPassiveAssemblyTargetId = 0;
uint8_t bmwFUDSPassiveAssemblyExpectedSeq = 1;
uint16_t bmwFUDSPassiveAssemblyExpectedLen = 0;
uint16_t bmwFUDSPassiveAssemblyReceivedLen = 0;
uint8_t bmwFUDSPassiveAssemblyPayload[32] = {};

const char *bmwFUDSClockProbeStatusText() {
  return bmwFUDSProbeStatus;
}

void sendNextBMWFUDSReadDID();
void sendNextBMWFUDSRTCWriteDID();
void sendBMWFUDSSecuritySeedRequest();
void sendNextBMWFUDSSweepDid();
void bmwFUDSSweepHandleUdsResponse(const uint8_t *uds, uint16_t udsLen);
void startBMWFUDSCAN86DidSweep();
void reportBMWFUDSSingleRead(const uint8_t *uds, uint16_t udsLen);

const char *bmwFUDSAddressingModeLabel() {
  static char label[24];
  if (bmwFUDSDirectAddressing) {
    snprintf(label, sizeof(label), "direct source 0x%02X", bmwFUDSRequestSourceId);
  } else {
    snprintf(label, sizeof(label), "source 0x%02X", bmwFUDSRequestSourceId);
  }
  return label;
}

void setBMWFUDSProbeStatus(const char *status) {
  strncpy(bmwFUDSProbeStatus, status, sizeof(bmwFUDSProbeStatus) - 1);
  bmwFUDSProbeStatus[sizeof(bmwFUDSProbeStatus) - 1] = '\0';
}

void printBMWFUDSFrame(const char *direction, unsigned long id, unsigned char len, const unsigned char *data) {
  if (bmwFUDSSweepMode) return;  // suppress per-frame spam during a full DID sweep (tens of thousands of frames)
  Serial.print("BMW_F UDS ");
  Serial.print(direction);
  Serial.print(" 0x");
  unsigned long baseId = id & 0x1FFFFFFF;
  if (baseId < 0x100) {
    Serial.print('0');
  }
  if (baseId < 0x10) {
    Serial.print('0');
  }
  Serial.print(baseId, HEX);
  Serial.print(" data=");
  for (uint8_t i = 0; i < len && i < 8; i++) {
    if (i > 0) {
      Serial.print(' ');
    }
    printCanHexByte(data[i]);
  }
  Serial.println();
}

bool isBMWFUDSDiagnosticSource(uint8_t sourceId) {
  return sourceId == BMWF_UDS_CAN_DIAG_ID || sourceId == BMWF_UDS_ETH_DIAG_ID || sourceId == BMWF_UDS_ZGM_INTERNAL_ID || sourceId == BMWF_UDS_ZGW_ID;
}

void logBMWFUDSPassiveSummary(uint8_t responseSourceId, uint8_t responseTargetId, unsigned char len, const unsigned char *data) {
  uint8_t pciType = data[1] & 0xF0;
  Serial.print("BMW_F UDS passive src=0x");
  printCanHexByte(responseSourceId);
  Serial.print(" dst=0x");
  printCanHexByte(responseTargetId);

  if (pciType == 0x10 && len >= 4) {
    uint16_t totalLen = ((uint16_t)(data[1] & 0x0F) << 8) | data[2];
    Serial.print(" first-frame len=");
    Serial.print(totalLen);
    Serial.print(" sid=0x");
    printCanHexByte(data[3]);
    if (len >= 6) {
      Serial.print(" did=0x");
      printCanHexByte(data[4]);
      printCanHexByte(data[5]);
    }
  } else if (pciType == 0x20) {
    Serial.print(" consecutive-frame seq=");
    Serial.print(data[1] & 0x0F);
  } else if (pciType == 0x00 && len >= 3) {
    Serial.print(" single-frame sid=0x");
    printCanHexByte(data[2]);
  } else {
    Serial.print(" unsupported-pci=0x");
    printCanHexByte(data[1]);
  }

  Serial.println();
}

void printBMWFUDSPayload(const uint8_t *payload, uint16_t len) {
  for (uint16_t i = 0; i < len; i++) {
    if (i > 0) {
      Serial.print(' ');
    }
    printCanHexByte(payload[i]);
  }
}

void printBMWFUDSPassiveComplete() {
  Serial.print("BMW_F UDS passive complete src=0x");
  printCanHexByte(bmwFUDSPassiveAssemblySourceId);
  Serial.print(" dst=0x");
  printCanHexByte(bmwFUDSPassiveAssemblyTargetId);
  Serial.print(" payload=");
  printBMWFUDSPayload(bmwFUDSPassiveAssemblyPayload, bmwFUDSPassiveAssemblyReceivedLen);
  Serial.println();
}

void printBMWFUDSActiveComplete() {
  Serial.print("BMW_F UDS active complete src=0x");
  printCanHexByte(bmwFUDSActiveAssemblySourceId);
  Serial.print(" dst=0x");
  printCanHexByte(bmwFUDSActiveAssemblyTargetId);
  Serial.print(" payload=");
  printBMWFUDSPayload(bmwFUDSActiveAssemblyPayload, bmwFUDSActiveAssemblyReceivedLen);
  Serial.println();
}

void appendBMWFUDSActiveBytes(const unsigned char *data, uint8_t startIndex, unsigned char len) {
  for (uint8_t i = startIndex; i < len && bmwFUDSActiveAssemblyReceivedLen < sizeof(bmwFUDSActiveAssemblyPayload); i++) {
    if (bmwFUDSActiveAssemblyReceivedLen < bmwFUDSActiveAssemblyExpectedLen) {
      bmwFUDSActiveAssemblyPayload[bmwFUDSActiveAssemblyReceivedLen++] = data[i];
    }
  }
}

void finishBMWFUDSActiveAssembly() {
  bmwFUDSActiveAssemblyReceivedLen = bmwFUDSActiveAssemblyExpectedLen;
  printBMWFUDSActiveComplete();
  bmwFUDSActiveAssemblyActive = false;

  if (bmwFUDSSweepMode && bmwFUDSProbeState == BMWF_UDS_SWEEP_WAIT_DID) {
    bmwFUDSSweepHandleUdsResponse(bmwFUDSActiveAssemblyPayload, bmwFUDSActiveAssemblyReceivedLen);
    return;
  }

  if (bmwFUDSProbeState == BMWF_UDS_WAIT_SINGLE_READ) {
    reportBMWFUDSSingleRead(bmwFUDSActiveAssemblyPayload, bmwFUDSActiveAssemblyReceivedLen);
    bmwFUDSProbeState = BMWF_UDS_DONE;
    return;
  }

  uint8_t sid = bmwFUDSActiveAssemblyPayload[0];
  if (sid == 0x62 && bmwFUDSActiveAssemblyReceivedLen >= 3) {
    uint16_t did = ((uint16_t)bmwFUDSActiveAssemblyPayload[1] << 8) | bmwFUDSActiveAssemblyPayload[2];
    snprintf(bmwFUDSProbeStatus, sizeof(bmwFUDSProbeStatus), "UDS ReadDID full positive response for 0x%04X. See Serial Monitor payload.", did);
    if (bmwFUDSProbeState == BMWF_UDS_WAIT_DID) {
      bmwFUDSProbeDidIndex++;
      sendNextBMWFUDSReadDID();
    }
  } else if (sid == 0x7F && bmwFUDSActiveAssemblyReceivedLen >= 3) {
    uint8_t rejectedService = bmwFUDSActiveAssemblyPayload[1];
    uint8_t nrc = bmwFUDSActiveAssemblyPayload[2];
    snprintf(bmwFUDSProbeStatus, sizeof(bmwFUDSProbeStatus), "UDS full negative response: service 0x%02X NRC 0x%02X.", rejectedService, nrc);
  } else {
    snprintf(bmwFUDSProbeStatus, sizeof(bmwFUDSProbeStatus), "UDS full multi-frame response SID 0x%02X. See Serial Monitor.", sid);
  }
}

void startBMWFUDSActiveAssembly(uint8_t responseSourceId, uint8_t responseTargetId, unsigned char len, const unsigned char *data) {
  bmwFUDSActiveAssemblyActive = true;
  bmwFUDSActiveAssemblySourceId = responseSourceId;
  bmwFUDSActiveAssemblyTargetId = responseTargetId;
  bmwFUDSActiveAssemblyExpectedSeq = 1;
  bmwFUDSActiveAssemblyExpectedLen = ((uint16_t)(data[1] & 0x0F) << 8) | data[2];
  bmwFUDSActiveAssemblyReceivedLen = 0;
  memset(bmwFUDSActiveAssemblyPayload, 0, sizeof(bmwFUDSActiveAssemblyPayload));
  appendBMWFUDSActiveBytes(data, 3, len);

  if (bmwFUDSActiveAssemblyExpectedLen > sizeof(bmwFUDSActiveAssemblyPayload)) {
    Serial.print("BMW_F UDS active first-frame len=");
    Serial.print(bmwFUDSActiveAssemblyExpectedLen);
    Serial.println(" TRUNCATED to buffer size");
    bmwFUDSActiveAssemblyExpectedLen = sizeof(bmwFUDSActiveAssemblyPayload);
  } else {
    Serial.print("BMW_F UDS active first-frame len=");
    Serial.println(bmwFUDSActiveAssemblyExpectedLen);
  }

  if (bmwFUDSActiveAssemblyReceivedLen >= bmwFUDSActiveAssemblyExpectedLen) {
    finishBMWFUDSActiveAssembly();
  } else {
    bmwFUDSProbeDeadlineMs = millis() + BMWF_UDS_RESPONSE_TIMEOUT_MS;
    setBMWFUDSProbeStatus("UDS active multi-frame response started; waiting for consecutive frames.");
  }
}

void updateBMWFUDSActiveAssembly(uint8_t responseSourceId, uint8_t responseTargetId, unsigned char len, const unsigned char *data) {
  if (!bmwFUDSActiveAssemblyActive || responseSourceId != bmwFUDSActiveAssemblySourceId || responseTargetId != bmwFUDSActiveAssemblyTargetId) {
    setBMWFUDSProbeStatus("UDS consecutive frame received without matching active multi-frame response.");
    return;
  }

  uint8_t seq = data[1] & 0x0F;
  if (seq != bmwFUDSActiveAssemblyExpectedSeq) {
    // Don't abort — MCP2515 dropped a frame. Log it, skip the missing bytes, and continue.
    Serial.print("BMW_F UDS active seq skip expected=");
    Serial.print(bmwFUDSActiveAssemblyExpectedSeq);
    Serial.print(" got=");
    Serial.println(seq);
    // Advance expected to seq+1 so subsequent frames can still be captured
    bmwFUDSActiveAssemblyExpectedSeq = (seq + 1) & 0x0F;
  } else {
    bmwFUDSActiveAssemblyExpectedSeq = (bmwFUDSActiveAssemblyExpectedSeq + 1) & 0x0F;
  }

  appendBMWFUDSActiveBytes(data, 2, len);
  bmwFUDSProbeDeadlineMs = millis() + BMWF_UDS_RESPONSE_TIMEOUT_MS;

  if (bmwFUDSActiveAssemblyReceivedLen >= bmwFUDSActiveAssemblyExpectedLen) {
    finishBMWFUDSActiveAssembly();
  }
}

void appendBMWFUDSPassiveBytes(const unsigned char *data, uint8_t startIndex, unsigned char len) {
  for (uint8_t i = startIndex; i < len && bmwFUDSPassiveAssemblyReceivedLen < sizeof(bmwFUDSPassiveAssemblyPayload); i++) {
    if (bmwFUDSPassiveAssemblyReceivedLen < bmwFUDSPassiveAssemblyExpectedLen) {
      bmwFUDSPassiveAssemblyPayload[bmwFUDSPassiveAssemblyReceivedLen++] = data[i];
    }
  }
}

void updateBMWFUDSPassiveAssembly(uint8_t responseSourceId, uint8_t responseTargetId, unsigned char len, const unsigned char *data) {
  uint8_t pciType = data[1] & 0xF0;

  if (pciType == 0x10 && len >= 4) {
    bmwFUDSPassiveAssemblyActive = true;
    bmwFUDSPassiveAssemblySourceId = responseSourceId;
    bmwFUDSPassiveAssemblyTargetId = responseTargetId;
    bmwFUDSPassiveAssemblyExpectedSeq = 1;
    bmwFUDSPassiveAssemblyExpectedLen = ((uint16_t)(data[1] & 0x0F) << 8) | data[2];
    bmwFUDSPassiveAssemblyReceivedLen = 0;
    memset(bmwFUDSPassiveAssemblyPayload, 0, sizeof(bmwFUDSPassiveAssemblyPayload));
    appendBMWFUDSPassiveBytes(data, 3, len);
  } else if (pciType == 0x20 && bmwFUDSPassiveAssemblyActive && responseSourceId == bmwFUDSPassiveAssemblySourceId && responseTargetId == bmwFUDSPassiveAssemblyTargetId) {
    uint8_t seq = data[1] & 0x0F;
    if (seq != bmwFUDSPassiveAssemblyExpectedSeq) {
      Serial.print("BMW_F UDS passive sequence mismatch expected=");
      Serial.print(bmwFUDSPassiveAssemblyExpectedSeq);
      Serial.print(" got=");
      Serial.println(seq);
      bmwFUDSPassiveAssemblyActive = false;
      return;
    }
    appendBMWFUDSPassiveBytes(data, 2, len);
    bmwFUDSPassiveAssemblyExpectedSeq = (bmwFUDSPassiveAssemblyExpectedSeq + 1) & 0x0F;
  } else if (pciType == 0x00 && len >= 3) {
    Serial.print("BMW_F UDS passive complete src=0x");
    printCanHexByte(responseSourceId);
    Serial.print(" dst=0x");
    printCanHexByte(responseTargetId);
    Serial.print(" payload=");
    uint8_t payloadLen = data[1] & 0x0F;
    uint8_t availableLen = len > 2 ? len - 2 : 0;
    printBMWFUDSPayload(&data[2], min(payloadLen, availableLen));
    Serial.println();
    return;
  }

  if (bmwFUDSPassiveAssemblyActive && bmwFUDSPassiveAssemblyExpectedLen > 0 && bmwFUDSPassiveAssemblyReceivedLen >= bmwFUDSPassiveAssemblyExpectedLen) {
    bmwFUDSPassiveAssemblyReceivedLen = bmwFUDSPassiveAssemblyExpectedLen;
    printBMWFUDSPassiveComplete();
    bmwFUDSPassiveAssemblyActive = false;
  }
}

void sendBMWFUDSFlowControlFromSource(uint8_t sourceId, uint8_t targetId, const char *label) {
  // 8-byte (DLC=8) padded FlowControl — BMW modules ignore short FC frames.
  uint8_t frame[8] = { targetId, 0x30, BMWF_UDS_FC_BLOCKSIZE, BMWF_UDS_FC_STMIN, 0x00, 0x00, 0x00, 0x00 };
  uint16_t requestId = 0x600 | sourceId;
  INT8U sendStatus = CAN.sendMsgBuf(requestId, 0, sizeof(frame), frame);
  printBMWFUDSFrame(label, requestId, sizeof(frame), frame);
  if (sendStatus != CAN_OK) {
    Serial.print("BMW_F UDS FC failed status=");
    Serial.println(sendStatus);
  }
}

bool sendBMWFUDSSingleFrame(const uint8_t *udsPayload, uint8_t udsLen, const char *label) {
  if (udsLen > 6) {
    setBMWFUDSProbeStatus("UDS TX rejected: single-frame payload too long.");
    return false;
  }

  uint8_t frame[8] = {
    bmwFUDSDirectAddressing ? bmwFUDSRequestSourceId : bmwFUDSTargetId,
    udsLen,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00
  };
  for (uint8_t i = 0; i < udsLen; i++) {
    frame[2 + i] = udsPayload[i];
  }

  uint16_t requestId = bmwFUDSDirectAddressing ? (0x600 | bmwFUDSTargetId) : (0x600 | bmwFUDSRequestSourceId);
  uint8_t frameLen = 2 + udsLen;
  INT8U sendStatus = CAN.sendMsgBuf(requestId, 0, frameLen, frame);
  printBMWFUDSFrame("TX", requestId, frameLen, frame);

  if (sendStatus != CAN_OK) {
    snprintf(bmwFUDSProbeStatus, sizeof(bmwFUDSProbeStatus), "UDS TX failed for %s, MCP status=%u.", label, sendStatus);
    Serial.print("BMW_F UDS TX failed status=");
    Serial.println(sendStatus);
    return false;
  }

  snprintf(bmwFUDSProbeStatus, sizeof(bmwFUDSProbeStatus), "UDS sent %s to target 0x%02X via %s mode.", label, bmwFUDSTargetId, bmwFUDSAddressingModeLabel());
  bmwFUDSProbeDeadlineMs = millis() + BMWF_UDS_RESPONSE_TIMEOUT_MS;
  return true;
}

bool getBMWFUDSLocalTime(struct tm &localTime) {
  time_t now = time(nullptr);
  if (now < 1609459200) {
    return false;
  }

  localtime_r(&now, &localTime);
  return true;
}

bool sendBMWFUDSFirstFrame(const uint8_t *udsPayload, uint8_t udsLen, const char *label) {
  if (udsLen <= 6) {
    return sendBMWFUDSSingleFrame(udsPayload, udsLen, label);
  }
  if (udsLen > sizeof(bmwFUDSWritePayload)) {
    setBMWFUDSProbeStatus("UDS TX rejected: multi-frame payload too long.");
    return false;
  }

  memcpy(bmwFUDSWritePayload, udsPayload, udsLen);
  bmwFUDSWritePayloadLen = udsLen;
  bmwFUDSWriteNextIndex = 5;
  bmwFUDSWriteSeq = 1;

  uint8_t frame[8] = {
    bmwFUDSDirectAddressing ? bmwFUDSRequestSourceId : bmwFUDSTargetId,
    (uint8_t)(0x10 | ((udsLen >> 8) & 0x0F)),
    udsLen,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00
  };
  for (uint8_t i = 0; i < 5 && i < udsLen; i++) {
    frame[3 + i] = udsPayload[i];
  }

  uint16_t requestId = bmwFUDSDirectAddressing ? (0x600 | bmwFUDSTargetId) : (0x600 | bmwFUDSRequestSourceId);
  INT8U sendStatus = CAN.sendMsgBuf(requestId, 0, sizeof(frame), frame);
  printBMWFUDSFrame("TX-FF", requestId, sizeof(frame), frame);

  if (sendStatus != CAN_OK) {
    snprintf(bmwFUDSProbeStatus, sizeof(bmwFUDSProbeStatus), "UDS first-frame TX failed for %s, MCP status=%u.", label, sendStatus);
    Serial.print("BMW_F UDS TX-FF failed status=");
    Serial.println(sendStatus);
    return false;
  }

  snprintf(bmwFUDSProbeStatus, sizeof(bmwFUDSProbeStatus), "UDS sent first frame for %s; waiting for FlowControl.", label);
  bmwFUDSProbeState = BMWF_UDS_WAIT_WRITE_FC;
  bmwFUDSProbeDeadlineMs = millis() + BMWF_UDS_RESPONSE_TIMEOUT_MS;
  return true;
}

unsigned long bmwFUDSWriteResponseTimeoutMs() {
  if (bmwFUDSRTCWriteMode && bmwFUDSCurrentWriteDid == 0x1704 && bmwFUDSRequestSourceId == BMWF_UDS_CAN_DIAG_ID && bmwFUDSTargetId == BMWF_UDS_OBSERVED_ENDPOINT_ID) {
    return BMWF_UDS_LONG_WRITE_RESPONSE_TIMEOUT_MS;
  }

  return BMWF_UDS_RESPONSE_TIMEOUT_MS;
}

bool sendBMWFUDSConsecutiveFrames(uint8_t blockSize, uint8_t stMin) {
  uint8_t sentInBlock = 0;
  while (bmwFUDSWriteNextIndex < bmwFUDSWritePayloadLen) {
    if (blockSize > 0 && sentInBlock >= blockSize) {
      bmwFUDSProbeState = BMWF_UDS_WAIT_WRITE_FC;
      bmwFUDSProbeDeadlineMs = millis() + BMWF_UDS_RESPONSE_TIMEOUT_MS;
      return true;
    }

    uint8_t frame[8] = {
      bmwFUDSDirectAddressing ? bmwFUDSRequestSourceId : bmwFUDSTargetId,
      (uint8_t)(0x20 | (bmwFUDSWriteSeq & 0x0F)),
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00
    };
    uint8_t remaining = bmwFUDSWritePayloadLen - bmwFUDSWriteNextIndex;
    uint8_t chunk = min((uint8_t)6, remaining);
    for (uint8_t i = 0; i < chunk; i++) {
      frame[2 + i] = bmwFUDSWritePayload[bmwFUDSWriteNextIndex + i];
    }

    if (stMin > 0 && stMin <= 0x7F) {
      delay(stMin);
    }

    uint16_t requestId = bmwFUDSDirectAddressing ? (0x600 | bmwFUDSTargetId) : (0x600 | bmwFUDSRequestSourceId);
    INT8U sendStatus = CAN.sendMsgBuf(requestId, 0, sizeof(frame), frame);
    printBMWFUDSFrame("TX-CF", requestId, sizeof(frame), frame);
    if (sendStatus != CAN_OK) {
      snprintf(bmwFUDSProbeStatus, sizeof(bmwFUDSProbeStatus), "UDS consecutive-frame TX failed, MCP status=%u.", sendStatus);
      Serial.print("BMW_F UDS TX-CF failed status=");
      Serial.println(sendStatus);
      return false;
    }

    bmwFUDSWriteNextIndex += chunk;
    bmwFUDSWriteSeq = (bmwFUDSWriteSeq + 1) & 0x0F;
    sentInBlock++;
  }

  bmwFUDSProbeState = BMWF_UDS_WAIT_WRITE_DID;
  unsigned long responseTimeoutMs = bmwFUDSWriteResponseTimeoutMs();
  bmwFUDSProbeDeadlineMs = millis() + responseTimeoutMs;
  snprintf(bmwFUDSProbeStatus, sizeof(bmwFUDSProbeStatus), "UDS RTC WriteDID 0x%04X sent; waiting %lums for response.", bmwFUDSCurrentWriteDid, responseTimeoutMs);
  return true;
}

void sendBMWFUDSFlowControl() {
  if (bmwFUDSDirectAddressing) {
    // 8-byte (DLC=8) padded FlowControl — BMW modules ignore short FC frames.
    uint8_t frame[8] = { bmwFUDSRequestSourceId, 0x30, BMWF_UDS_FC_BLOCKSIZE, BMWF_UDS_FC_STMIN, 0x00, 0x00, 0x00, 0x00 };
    uint16_t requestId = 0x600 | bmwFUDSTargetId;
    INT8U sendStatus = CAN.sendMsgBuf(requestId, 0, sizeof(frame), frame);
    printBMWFUDSFrame("FC", requestId, sizeof(frame), frame);
    if (sendStatus != CAN_OK) {
      Serial.print("BMW_F UDS FC failed status=");
      Serial.println(sendStatus);
    }
  } else {
    sendBMWFUDSFlowControlFromSource(bmwFUDSRequestSourceId, bmwFUDSTargetId, "FC");
  }
}

void sendNextBMWFUDSReadDID() {
  if (bmwFUDSProbeDidIndex >= sizeof(BMWF_UDS_READ_DIDS) / sizeof(BMWF_UDS_READ_DIDS[0])) {
    bmwFUDSProbeState = BMWF_UDS_DONE;
    setBMWFUDSProbeStatus("UDS read-only probe finished. Check Serial Monitor for 0x660 responses and NRC codes.");
    Serial.println("BMW_F UDS probe finished");
    return;
  }

  uint16_t did = BMWF_UDS_READ_DIDS[bmwFUDSProbeDidIndex];
  uint8_t payload[] = { 0x22, hi8(did), lo8(did) };
  char label[32];
  snprintf(label, sizeof(label), "ReadDID 0x%04X", did);
  bmwFUDSProbeState = BMWF_UDS_WAIT_DID;
  sendBMWFUDSSingleFrame(payload, sizeof(payload), label);
}

void sendNextBMWFUDSRTCWriteDID() {
  if (bmwFUDSProbeDidIndex >= sizeof(BMWF_UDS_RTC_WRITE_DIDS) / sizeof(BMWF_UDS_RTC_WRITE_DIDS[0])) {
    bmwFUDSProbeState = BMWF_UDS_DONE;
    bmwFUDSRTCWriteMode = false;
    setBMWFUDSProbeStatus("UDS RTC WriteDID probe finished without positive 0x6E response. Check Serial Monitor NRC codes.");
    Serial.println("BMW_F UDS RTC WriteDID probe finished without positive response");
    return;
  }

  struct tm localTime;
  if (!getBMWFUDSLocalTime(localTime)) {
    bmwFUDSProbeState = BMWF_UDS_DONE;
    bmwFUDSRTCWriteMode = false;
    setBMWFUDSProbeStatus("UDS RTC write aborted: ESP32 system time is invalid.");
    Serial.println("BMW_F UDS RTC write aborted: ESP32 system time invalid");
    return;
  }

  uint16_t did = BMWF_UDS_RTC_WRITE_DIDS[bmwFUDSProbeDidIndex];
  uint16_t year = localTime.tm_year + 1900;
  bmwFUDSCurrentWriteDid = did;

  uint8_t payload[12];
  uint8_t payloadLen;
  payload[0] = 0x2E;
  payload[1] = hi8(did);
  payload[2] = lo8(did);
  payload[3] = (uint8_t)localTime.tm_hour;
  payload[4] = (uint8_t)localTime.tm_min;
  payload[5] = (uint8_t)localTime.tm_sec;
  payload[6] = (uint8_t)localTime.tm_mday;
  payload[7] = (uint8_t)(localTime.tm_mon + 1);
  if (did == 0xD113) {
    // Cluster RTC layout from the sweep: hh mm ss dd mo yearHi yearLo trailer.
    // Year is BIG-endian here (0x07E9 read back), unlike the 1704/1802 candidates,
    // and there is a trailing byte (0xE6 read back; meaning unconfirmed - likely
    // weekday/flags - so we echo it). 8 value bytes to match the ReadDID length.
    payload[8] = hi8(year);
    payload[9] = lo8(year);
    payload[10] = 0xE6;
    payloadLen = 11;
  } else {
    payload[8] = lo8(year);
    payload[9] = hi8(year);
    payloadLen = 10;
  }

  char label[40];
  snprintf(label, sizeof(label), "WriteDID RTC 0x%04X", did);
  Serial.print("BMW_F UDS RTC WriteDID trying 0x");
  Serial.print(did, HEX);
  Serial.print(" time=");
  Serial.print(localTime.tm_hour);
  Serial.print(':');
  Serial.print(localTime.tm_min);
  Serial.print(':');
  Serial.print(localTime.tm_sec);
  Serial.print(' ');
  Serial.print(localTime.tm_mday);
  Serial.print('.');
  Serial.print(localTime.tm_mon + 1);
  Serial.print('.');
  Serial.println(year);
  sendBMWFUDSFirstFrame(payload, payloadLen, label);
}

void sendBMWFUDSSecuritySeedRequest() {
  bmwFUDSSecuritySeedMode = true;
  uint8_t payload[] = { 0x27, 0x01 };
  bmwFUDSProbeState = BMWF_UDS_WAIT_SECURITY_SEED;
  sendBMWFUDSSingleFrame(payload, sizeof(payload), "SecurityAccess seed 0x27 0x01");
}

void sendNextBMWFUDSSweepDid() {
  if (bmwFUDSSweepDid > BMWF_UDS_SWEEP_DID_END) {
    bmwFUDSProbeState = BMWF_UDS_DONE;
    bmwFUDSSweepMode = false;
    const char *what = bmwFUDSSweepIsRoutine ? "Routine scan" : "DID sweep";
    char summary[180];
    snprintf(summary, sizeof(summary),
             "# %s finished: %u positive, %u other-NRC, %u conditions-NRC(0x22).%s",
             what, bmwFUDSSweepFoundCount, bmwFUDSSweepInterestingNrc, bmwFUDSSweepConditionsNrc,
             bmwFUDSLogTruncated ? " *** LOG TRUNCATED - re-run a narrower range ***" : "");
    bmwFUDSLogLine(summary);
    snprintf(bmwFUDSProbeStatus, sizeof(bmwFUDSProbeStatus),
             "UDS %s done: %u positive, %u other-NRC, %u cond-NRC. Download /bmw-f-uds-log.txt",
             what, bmwFUDSSweepFoundCount, bmwFUDSSweepInterestingNrc, bmwFUDSSweepConditionsNrc);
    Serial.print("BMW_F UDS ");
    Serial.print(what);
    Serial.print(" finished: ");
    Serial.print(bmwFUDSSweepFoundCount);
    Serial.println(" positive");
    bmwFUDSSweepIsRoutine = false;
    return;
  }

  uint16_t did = (uint16_t)bmwFUDSSweepDid;
  bmwFUDSProbeState = BMWF_UDS_SWEEP_WAIT_DID;
  if (bmwFUDSSweepIsRoutine) {
    // 31 03 = RoutineControl requestRoutineResults. This READS a routine's result;
    // it does NOT start (31 01) or stop (31 02) anything, so the scan is safe.
    uint8_t payload[] = { 0x31, 0x03, hi8(did), lo8(did) };
    sendBMWFUDSSingleFrame(payload, sizeof(payload), "Routine scan");
  } else {
    uint8_t payload[] = { 0x22, hi8(did), lo8(did) };
    sendBMWFUDSSingleFrame(payload, sizeof(payload), "DID sweep");
  }
  bmwFUDSProbeDeadlineMs = millis() + BMWF_UDS_SWEEP_RESPONSE_TIMEOUT_MS;
}

// Handles one assembled UDS response (single- or multi-frame) during a sweep,
// logs anything noteworthy, then advances to the next DID.
void bmwFUDSSweepHandleUdsResponse(const uint8_t *uds, uint16_t udsLen) {
  uint16_t did = (uint16_t)bmwFUDSSweepDid;
  char line[200];
  uint8_t posSid = bmwFUDSSweepIsRoutine ? 0x71 : 0x62;   // positive SID: 71=routine, 62=ReadDID
  const char *kind = bmwFUDSSweepIsRoutine ? "ROUTINE" : "DID";

  if (udsLen >= 1 && uds[0] == posSid) {
    // Positive. Log the requested id and the full payload.
    int pos = snprintf(line, sizeof(line), "%s 0x%04X = ", kind, did);
    for (uint16_t i = 0; i < udsLen && pos < (int)sizeof(line) - 4; i++) {
      pos += snprintf(&line[pos], sizeof(line) - pos, "%02X ", uds[i]);
    }
    if (!bmwFUDSSweepIsRoutine) {
      const uint8_t *value = (udsLen >= 3) ? &uds[3] : uds;
      uint16_t valueLen = (udsLen >= 3) ? (udsLen - 3) : 0;
      if (bmwFUDSValueLooksDateLike(value, valueLen) && pos < (int)sizeof(line) - 12) {
        snprintf(&line[pos], sizeof(line) - pos, " <== DATE?");
      }
    }
    bmwFUDSLogLine(line);
    bmwFUDSSweepFoundCount++;
  } else if (udsLen >= 3 && uds[0] == 0x7F) {
    // Negative response. Two NRCs come in huge contiguous blocks and are just
    // noise, so we COUNT them instead of logging each one:
    //   0x31 requestOutOfRange  - DID not supported
    //   0x22 conditionsNotCorrect - exists but needs conditions (measurement blocks)
    // Every other NRC (e.g. 0x33 securityAccessDenied) is rare and interesting -
    // a clock-write DID could be security-gated - so log those per line.
    uint8_t nrc = uds[2];
    if (nrc == 0x31) {
      // skip
    } else if (nrc == 0x22) {
      bmwFUDSSweepConditionsNrc++;
    } else {
      // For routines, NRC 0x24 (requestSequenceError) typically means the routine
      // EXISTS but hasn't been started - exactly the candidates we want to see.
      snprintf(line, sizeof(line), "%s 0x%04X NRC 0x%02X (exists/gated)", kind, did, nrc);
      bmwFUDSLogLine(line);
      bmwFUDSSweepInterestingNrc++;
    }
  }

  bmwFUDSSweepDid++;
  sendNextBMWFUDSSweepDid();
}

void startBMWFUDSCAN86DidSweep() {
  bmwFUDSSweepMode = true;
  bmwFUDSSweepIsRoutine = false;
  bmwFUDSRTCWriteMode = false;
  bmwFUDSSecuritySeedMode = false;
  bmwFUDSDirectAddressing = false;
  bmwFUDSSkipExtendedSession = true;
  bmwFUDSActiveAssemblyActive = false;
  bmwFUDSRequestSourceId = BMWF_UDS_SWEEP_SOURCE_ID;
  bmwFUDSTargetId = BMWF_UDS_SWEEP_TARGET_ID;
  bmwFUDSSweepDid = BMWF_UDS_SWEEP_DID_START;
  bmwFUDSSweepFoundCount = 0;
  bmwFUDSSweepInterestingNrc = 0;
  bmwFUDSSweepConditionsNrc = 0;

  bmwFUDSLogReset();
  char header[160];
  snprintf(header, sizeof(header),
           "# BMW F UDS ReadDID sweep  source=0x%02X target=0x%02X  range 0x%04X-0x%04X",
           BMWF_UDS_SWEEP_SOURCE_ID, BMWF_UDS_SWEEP_TARGET_ID,
           (unsigned)BMWF_UDS_SWEEP_DID_START, (unsigned)BMWF_UDS_SWEEP_DID_END);
  bmwFUDSLogLine(header);
  bmwFUDSLogLine("# Lines = positive ReadDID payloads and non-0x31 NRCs. 'DATE?' = possible calendar bytes.");

  setBMWFUDSProbeStatus("UDS DID sweep running on 0x86. Leave it; download /bmw-f-uds-log.txt when done.");
  Serial.println("BMW_F UDS full DID sweep starting on 0x86 via F1 (Serial frame spam suppressed)");
  sendNextBMWFUDSSweepDid();
}

// RoutineControl enumeration scan on 0x86. Uses 31 03 (requestRoutineResults),
// which READS routine results and never starts/stops anything, so it is safe to
// sweep. Opens an extended session first (routines usually require it), then walks
// 0x0000..0xFFFF logging positives (0x71) and non-0x31 NRCs (0x24 = routine exists
// but not started - the candidates). Results go to /bmw-f-uds-log.txt.
void startBMWFUDSCAN86RoutineScan() {
  bmwFUDSSessionSubfunction = 0x03;
  bmwFUDSSweepMode = true;
  bmwFUDSSweepIsRoutine = true;
  bmwFUDSRTCWriteMode = false;
  bmwFUDSSecuritySeedMode = false;
  bmwFUDSDirectAddressing = false;
  bmwFUDSSkipExtendedSession = false;
  bmwFUDSActiveAssemblyActive = false;
  bmwFUDSRequestSourceId = BMWF_UDS_SWEEP_SOURCE_ID;
  bmwFUDSTargetId = BMWF_UDS_SWEEP_TARGET_ID;
  bmwFUDSSweepDid = BMWF_UDS_SWEEP_DID_START;
  bmwFUDSSweepFoundCount = 0;
  bmwFUDSSweepInterestingNrc = 0;
  bmwFUDSSweepConditionsNrc = 0;

  bmwFUDSLogReset();
  char header[160];
  snprintf(header, sizeof(header),
           "# BMW F UDS RoutineControl scan (31 03, read-only)  source=0x%02X target=0x%02X  range 0x%04X-0x%04X",
           BMWF_UDS_SWEEP_SOURCE_ID, BMWF_UDS_SWEEP_TARGET_ID,
           (unsigned)BMWF_UDS_SWEEP_DID_START, (unsigned)BMWF_UDS_SWEEP_DID_END);
  bmwFUDSLogLine(header);
  bmwFUDSLogLine("# Lines = positive 0x71 routine results and non-0x31 NRCs (0x24 = routine exists, not started).");

  bmwFUDSProbeState = BMWF_UDS_WAIT_SESSION;
  setBMWFUDSProbeStatus("UDS RoutineControl scan running on 0x86. Leave it; download /bmw-f-uds-log.txt when done.");
  Serial.println("BMW_F UDS RoutineControl scan starting on 0x86 via F1 (31 03 read-only, extended session first)");
  uint8_t payload[] = { 0x10, bmwFUDSSessionSubfunction };
  sendBMWFUDSSingleFrame(payload, sizeof(payload), "DiagnosticSessionControl before routine scan");
}

// Formats a single ReadDID 0xD113 response (the cluster RTC) into the status line
// and Serial, decoding the time/date so a bus-frame test shows pass/fail at a glance.
uint16_t bmwFUDSSingleReadDid = 0;  // which DID the current single-read requested

void reportBMWFUDSSingleRead(const uint8_t *uds, uint16_t udsLen) {
  // VIN (or any text DID): decode the value bytes as printable ASCII.
  if (bmwFUDSSingleReadDid == 0xF190 && udsLen >= 4 && uds[0] == 0x62) {
    char vin[40];
    int vp = 0;
    for (uint16_t i = 3; i < udsLen && vp < (int)sizeof(vin) - 1; i++) {
      uint8_t c = uds[i];
      vin[vp++] = (c >= 0x20 && c < 0x7F) ? (char)c : '.';
    }
    vin[vp] = '\0';
    snprintf(bmwFUDSProbeStatus, sizeof(bmwFUDSProbeStatus), "VIN (0xF190): %s", vin);
    Serial.print("BMW_F UDS ");
    Serial.println(bmwFUDSProbeStatus);
    return;
  }

  char raw[80];
  int rp = 0;
  raw[0] = '\0';
  for (uint16_t i = 0; i < udsLen && rp < (int)sizeof(raw) - 4; i++) {
    rp += snprintf(&raw[rp], sizeof(raw) - rp, "%02X ", uds[i]);
  }

  // Expect 62 D1 13 + 8 value bytes: hh mm ss dd mo yearHi yearLo trailer.
  if (udsLen >= 11 && uds[0] == 0x62) {
    const uint8_t *v = &uds[3];
    uint8_t hh = v[0], mm = v[1], ss = v[2], dd = v[3], mo = v[4];
    uint16_t yr = ((uint16_t)v[5] << 8) | v[6];
    char timeStr[28];
    if (hh <= 23 && mm <= 59 && ss <= 59) {
      snprintf(timeStr, sizeof(timeStr), "%02u:%02u:%02u VALID", hh, mm, ss);
    } else {
      snprintf(timeStr, sizeof(timeStr), "INVALID(%02X %02X %02X)", hh, mm, ss);
    }
    snprintf(bmwFUDSProbeStatus, sizeof(bmwFUDSProbeStatus),
             "RTC 0xD113: time %s  date %02u-%02u-%04u  raw %s", timeStr, dd, mo, yr, raw);
  } else {
    snprintf(bmwFUDSProbeStatus, sizeof(bmwFUDSProbeStatus),
             "RTC 0xD113 read (unexpected %u bytes): %s", udsLen, raw);
  }
  Serial.print("BMW_F UDS ");
  Serial.println(bmwFUDSProbeStatus);
}

// One-shot ReadDID 0xD113 on F1 -> 0x86 (no session, as the sweep proved it answers).
// Use it to check the cluster RTC after enabling a candidate time broadcast: if the
// time bytes flip from FD/invalid to your sent value, that broadcast frame works.
void startBMWFUDSReadD113() {
  bmwFUDSSweepMode = false;
  bmwFUDSRTCWriteMode = false;
  bmwFUDSSecuritySeedMode = false;
  bmwFUDSDirectAddressing = false;
  bmwFUDSSkipExtendedSession = true;
  bmwFUDSActiveAssemblyActive = false;
  bmwFUDSRequestSourceId = BMWF_UDS_CAN_DIAG_ID;    // 0xF1
  bmwFUDSTargetId = BMWF_UDS_OBSERVED_ENDPOINT_ID;  // 0x86
  bmwFUDSSingleReadDid = 0xD113;
  bmwFUDSProbeState = BMWF_UDS_WAIT_SINGLE_READ;

  uint8_t payload[] = { 0x22, 0xD1, 0x13 };
  Serial.println("BMW_F UDS single RTC read 0xD113 via F1 -> 0x86");
  sendBMWFUDSSingleFrame(payload, sizeof(payload), "ReadDID 0xD113");
  bmwFUDSProbeDeadlineMs = millis() + BMWF_UDS_RESPONSE_TIMEOUT_MS;
}

// One-shot ReadDID 0xF190 (VIN) on F1 -> 0x86, decoded as ASCII. Result lands in
// the UDS status line (and Serial). The cluster VIN may be all-zero if unprogrammed.
void startBMWFUDSReadVin() {
  bmwFUDSSweepMode = false;
  bmwFUDSRTCWriteMode = false;
  bmwFUDSSecuritySeedMode = false;
  bmwFUDSDirectAddressing = false;
  bmwFUDSSkipExtendedSession = true;
  bmwFUDSActiveAssemblyActive = false;
  bmwFUDSRequestSourceId = BMWF_UDS_CAN_DIAG_ID;    // 0xF1
  bmwFUDSTargetId = BMWF_UDS_OBSERVED_ENDPOINT_ID;  // 0x86
  bmwFUDSSingleReadDid = 0xF190;
  bmwFUDSProbeState = BMWF_UDS_WAIT_SINGLE_READ;
  setBMWFUDSProbeStatus("Reading VIN (0xF190) from 0x86...");

  uint8_t payload[] = { 0x22, 0xF1, 0x90 };
  Serial.println("BMW_F UDS VIN read 0xF190 via F1 -> 0x86");
  sendBMWFUDSSingleFrame(payload, sizeof(payload), "ReadDID 0xF190 VIN");
  bmwFUDSProbeDeadlineMs = millis() + BMWF_UDS_RESPONSE_TIMEOUT_MS;
}

void sendNextBMWFUDSScanTarget() {
  if (bmwFUDSScanTargetId > BMWF_UDS_SCAN_END_ID) {
    bmwFUDSProbeState = BMWF_UDS_DONE;
    snprintf(bmwFUDSProbeStatus, sizeof(bmwFUDSProbeStatus), "UDS target scan finished. Found %u target(s). See Serial Monitor.", bmwFUDSScanFoundCount);
    Serial.print("BMW_F UDS target scan finished, found ");
    Serial.print(bmwFUDSScanFoundCount);
    Serial.println(" target(s)");
    return;
  }

  bmwFUDSTargetId = bmwFUDSScanTargetId;
  uint8_t payload[] = { 0x22, 0xF1, 0x86 };
  char label[32];
  snprintf(label, sizeof(label), "Scan target 0x%02X", bmwFUDSTargetId);
  bmwFUDSProbeState = BMWF_UDS_SCAN_WAIT_TARGET;
  sendBMWFUDSSingleFrame(payload, sizeof(payload), label);
  bmwFUDSProbeDeadlineMs = millis() + BMWF_UDS_SCAN_RESPONSE_TIMEOUT_MS;
}

void startBMWFUDSClockProbeWithSourceTarget(uint8_t sourceId, uint8_t targetId, const char *label, bool skipExtendedSession = false) {
  bmwFUDSProbeDidIndex = 0;
  bmwFUDSSweepMode = false;
  bmwFUDSTargetId = targetId;
  bmwFUDSRequestSourceId = sourceId;
  bmwFUDSDirectAddressing = false;
  bmwFUDSSkipExtendedSession = skipExtendedSession;
  bmwFUDSRTCWriteMode = false;
  bmwFUDSSecuritySeedMode = false;
  bmwFUDSActiveAssemblyActive = false;
  bmwFUDSProbeState = skipExtendedSession ? BMWF_UDS_WAIT_DID : BMWF_UDS_WAIT_SESSION;
  Serial.print("BMW_F UDS read-only clock probe starting via ");
  Serial.println(label);
  Serial.print("BMW_F UDS probe target=0x");
  printCanHexByte(bmwFUDSTargetId);
  Serial.print(" source=0x");
  printCanHexByte(bmwFUDSRequestSourceId);
  if (skipExtendedSession) {
    Serial.print(" no-session");
  }
  Serial.println();
  if (skipExtendedSession) {
    sendNextBMWFUDSReadDID();
  } else {
    uint8_t payload[] = { 0x10, 0x03 };
    sendBMWFUDSSingleFrame(payload, sizeof(payload), "DiagnosticSessionControl 0x10 0x03");
  }
}

void startBMWFUDSClockProbeWithSource(uint8_t sourceId, const char *label) {
  startBMWFUDSClockProbeWithSourceTarget(sourceId, BMWF_UDS_DEFAULT_KOMBI_ID, label);
}

void startBMWFUDSRTCWriteProbeWithSourceTarget(uint8_t sourceId, uint8_t targetId, const char *label, bool skipExtendedSession = false, uint8_t sessionSub = 0x03) {
  bmwFUDSSessionSubfunction = sessionSub;
  bmwFUDSProbeDidIndex = 0;
  bmwFUDSSweepMode = false;
  bmwFUDSTargetId = targetId;
  bmwFUDSRequestSourceId = sourceId;
  bmwFUDSDirectAddressing = false;
  bmwFUDSSkipExtendedSession = skipExtendedSession;
  bmwFUDSRTCWriteMode = true;
  bmwFUDSSecuritySeedMode = false;
  bmwFUDSActiveAssemblyActive = false;
  bmwFUDSWritePayloadLen = 0;
  bmwFUDSWriteNextIndex = 0;
  bmwFUDSWriteSeq = 1;
  bmwFUDSCurrentWriteDid = 0;
  bmwFUDSProbeState = skipExtendedSession ? BMWF_UDS_WAIT_WRITE_DID : BMWF_UDS_WAIT_SESSION;

  Serial.print("BMW_F UDS RTC WriteDID probe starting via ");
  Serial.println(label);
  Serial.print("BMW_F UDS RTC write target=0x");
  printCanHexByte(bmwFUDSTargetId);
  Serial.print(" source=0x");
  printCanHexByte(bmwFUDSRequestSourceId);
  if (skipExtendedSession) {
    Serial.print(" no-session");
  }
  Serial.println();

  if (skipExtendedSession) {
    sendNextBMWFUDSRTCWriteDID();
  } else {
    uint8_t payload[] = { 0x10, bmwFUDSSessionSubfunction };
    Serial.print("BMW_F UDS opening session 0x10 0x");
    printCanHexByte(bmwFUDSSessionSubfunction);
    Serial.println(" before RTC write");
    sendBMWFUDSSingleFrame(payload, sizeof(payload), "DiagnosticSessionControl before RTC write");
  }
}

void startBMWFUDSSecuritySeedProbeWithSourceTarget(uint8_t sourceId, uint8_t targetId, const char *label, uint8_t sessionSub = 0x03) {
  bmwFUDSSessionSubfunction = sessionSub;
  bmwFUDSProbeDidIndex = 0;
  bmwFUDSSweepMode = false;
  bmwFUDSTargetId = targetId;
  bmwFUDSRequestSourceId = sourceId;
  bmwFUDSDirectAddressing = false;
  bmwFUDSSkipExtendedSession = false;
  bmwFUDSRTCWriteMode = false;
  bmwFUDSSecuritySeedMode = true;
  bmwFUDSActiveAssemblyActive = false;
  bmwFUDSProbeState = BMWF_UDS_WAIT_SESSION;

  Serial.print("BMW_F UDS SecurityAccess seed probe starting via ");
  Serial.println(label);
  Serial.print("BMW_F UDS SA seed target=0x");
  printCanHexByte(bmwFUDSTargetId);
  Serial.print(" source=0x");
  printCanHexByte(bmwFUDSRequestSourceId);
  Serial.println();

  uint8_t payload[] = { 0x10, bmwFUDSSessionSubfunction };
  Serial.print("BMW_F UDS opening session 0x10 0x");
  printCanHexByte(bmwFUDSSessionSubfunction);
  Serial.println(" before SA seed");
  sendBMWFUDSSingleFrame(payload, sizeof(payload), "DiagnosticSessionControl before SA seed");
}

void startBMWFUDSClockProbe() {
  startBMWFUDSClockProbeWithSource(BMWF_UDS_ETH_DIAG_ID, "ETH diagnostic source 0xF4");
}

void startBMWFUDSCANClockProbe() {
  startBMWFUDSClockProbeWithSource(BMWF_UDS_CAN_DIAG_ID, "CAN diagnostic source 0xF1");
}

void startBMWFUDSCAN86ClockProbe() {
  startBMWFUDSClockProbeWithSourceTarget(BMWF_UDS_CAN_DIAG_ID, BMWF_UDS_OBSERVED_ENDPOINT_ID, "CAN diagnostic source 0xF1 to observed endpoint 0x86", true);
}

void startBMWFUDSZGMInternalClockProbe() {
  startBMWFUDSClockProbeWithSource(BMWF_UDS_ZGM_INTERNAL_ID, "ZGM internal source 0xF0");
}

void startBMWFUDSZGM86ClockProbe() {
  startBMWFUDSClockProbeWithSourceTarget(BMWF_UDS_ZGM_INTERNAL_ID, BMWF_UDS_OBSERVED_ENDPOINT_ID, "ZGM internal source 0xF0 to observed endpoint 0x86", true);
}

void startBMWFUDSZGWClockProbe() {
  startBMWFUDSClockProbeWithSource(BMWF_UDS_ZGW_ID, "ZGW diagnostic source 0x10");
}

void startBMWFUDSCANRTCWriteProbe() {
  startBMWFUDSRTCWriteProbeWithSourceTarget(BMWF_UDS_CAN_DIAG_ID, BMWF_UDS_DEFAULT_KOMBI_ID, "CAN diagnostic source 0xF1 to KOMBI 0x60");
}

void startBMWFUDSCAN86RTCWriteProbe() {
  startBMWFUDSRTCWriteProbeWithSourceTarget(BMWF_UDS_CAN_DIAG_ID, BMWF_UDS_OBSERVED_ENDPOINT_ID, "CAN diagnostic source 0xF1 to observed endpoint 0x86");
}

// Option 1: try the RTC write in a PROGRAMMING session (0x10 0x02) instead of
// extended (0x03). If 0x86 returns 7F 2E 33 the write is security-gated; if it
// returns 6E the clock is set; if still 7F 2E 31 the DID is genuinely read-only.
void startBMWFUDSCAN86RTCWriteProgSession() {
  startBMWFUDSRTCWriteProbeWithSourceTarget(BMWF_UDS_CAN_DIAG_ID, BMWF_UDS_OBSERVED_ENDPOINT_ID, "CAN F1 to 0x86 programming session 10 02", false, 0x02);
}

void startBMWFUDSZGMRTCWriteProbe() {
  startBMWFUDSRTCWriteProbeWithSourceTarget(BMWF_UDS_ZGM_INTERNAL_ID, BMWF_UDS_DEFAULT_KOMBI_ID, "ZGM internal source 0xF0 to KOMBI 0x60");
}

void startBMWFUDSZGM86RTCWriteProbe() {
  startBMWFUDSRTCWriteProbeWithSourceTarget(BMWF_UDS_ZGM_INTERNAL_ID, BMWF_UDS_OBSERVED_ENDPOINT_ID, "ZGM internal source 0xF0 to observed endpoint 0x86");
}

void startBMWFUDSCANSecuritySeedProbe() {
  startBMWFUDSSecuritySeedProbeWithSourceTarget(BMWF_UDS_CAN_DIAG_ID, BMWF_UDS_DEFAULT_KOMBI_ID, "CAN diagnostic source 0xF1 to KOMBI 0x60");
}

void startBMWFUDSCAN86SecuritySeedProbe() {
  startBMWFUDSSecuritySeedProbeWithSourceTarget(BMWF_UDS_CAN_DIAG_ID, BMWF_UDS_OBSERVED_ENDPOINT_ID, "CAN diagnostic source 0xF1 to observed endpoint 0x86");
}

// Option 1 companion: request the SecurityAccess seed in a PROGRAMMING session.
// Earlier 27 01 in extended session returned "not supported in this session", so
// security may only be offered here. Logs the seed (67 01 ...) or the NRC. No key
// is sent - the BMW seed->key algorithm is not known to us yet.
void startBMWFUDSCAN86SecuritySeedProgSession() {
  startBMWFUDSSecuritySeedProbeWithSourceTarget(BMWF_UDS_CAN_DIAG_ID, BMWF_UDS_OBSERVED_ENDPOINT_ID, "CAN F1 to 0x86 programming session 10 02", 0x02);
}

void startBMWFUDSZGMSecuritySeedProbe() {
  startBMWFUDSSecuritySeedProbeWithSourceTarget(BMWF_UDS_ZGM_INTERNAL_ID, BMWF_UDS_DEFAULT_KOMBI_ID, "ZGM internal source 0xF0 to KOMBI 0x60");
}

void startBMWFUDSZGM86SecuritySeedProbe() {
  startBMWFUDSSecuritySeedProbeWithSourceTarget(BMWF_UDS_ZGM_INTERNAL_ID, BMWF_UDS_OBSERVED_ENDPOINT_ID, "ZGM internal source 0xF0 to observed endpoint 0x86");
}

void startBMWFUDSTargetScanWithSource(uint8_t sourceId, const char *label) {
  bmwFUDSScanTargetId = BMWF_UDS_SCAN_START_ID;
  bmwFUDSScanFoundCount = 0;
  bmwFUDSSweepMode = false;
  bmwFUDSRequestSourceId = sourceId;
  bmwFUDSDirectAddressing = false;
  bmwFUDSProbeState = BMWF_UDS_SCAN_WAIT_TARGET;
  Serial.print("BMW_F UDS target scan starting via ");
  Serial.print(label);
  Serial.println(", read-only DID 0xF186");
  sendNextBMWFUDSScanTarget();
}

void startBMWFUDSTargetScan() {
  startBMWFUDSTargetScanWithSource(BMWF_UDS_ETH_DIAG_ID, "ETH diagnostic source 0xF4");
}

void startBMWFUDSZGWTargetScan() {
  startBMWFUDSTargetScanWithSource(BMWF_UDS_ZGW_ID, "ZGW source 0x10");
}

void startBMWFUDSDirectTargetScan() {
  bmwFUDSScanTargetId = BMWF_UDS_SCAN_START_ID;
  bmwFUDSScanFoundCount = 0;
  bmwFUDSSweepMode = false;
  bmwFUDSRequestSourceId = BMWF_UDS_TESTER_ID;
  bmwFUDSDirectAddressing = true;
  bmwFUDSProbeState = BMWF_UDS_SCAN_WAIT_TARGET;
  Serial.println("BMW_F UDS direct target scan starting on 0x600+target with source 0xF4, read-only DID 0xF186");
  sendNextBMWFUDSScanTarget();
}

void updateBMWFUDSClockProbe() {
  if (bmwFUDSProbeState == BMWF_UDS_IDLE || bmwFUDSProbeState == BMWF_UDS_DONE) {
    return;
  }

  if ((long)(millis() - bmwFUDSProbeDeadlineMs) < 0) {
    return;
  }

  if (bmwFUDSProbeState == BMWF_UDS_WAIT_SESSION) {
    if (bmwFUDSRTCWriteMode) {
      Serial.println("BMW_F UDS session response timeout; continuing with RTC WriteDID probe");
      setBMWFUDSProbeStatus("UDS session timed out; continuing with RTC WriteDID probe.");
      sendNextBMWFUDSRTCWriteDID();
    } else if (bmwFUDSSecuritySeedMode) {
      Serial.println("BMW_F UDS session response timeout; continuing with SecurityAccess seed request");
      setBMWFUDSProbeStatus("UDS session timed out; continuing with SecurityAccess seed request.");
      sendBMWFUDSSecuritySeedRequest();
    } else if (bmwFUDSSweepMode) {
      Serial.println("BMW_F UDS session response timeout; starting scan anyway");
      sendNextBMWFUDSSweepDid();
    } else {
      Serial.println("BMW_F UDS session response timeout; continuing with read-only DID probe");
      setBMWFUDSProbeStatus("UDS session timed out; continuing with read-only DID probes.");
      sendNextBMWFUDSReadDID();
    }
  } else if (bmwFUDSProbeState == BMWF_UDS_WAIT_DEFAULT_SESSION) {
    Serial.println("BMW_F UDS default session response timeout; continuing with read-only DID probe");
    setBMWFUDSProbeStatus("UDS default session timed out; continuing with read-only DID probes.");
    sendNextBMWFUDSReadDID();
  } else if (bmwFUDSProbeState == BMWF_UDS_WAIT_DID) {
    uint16_t did = BMWF_UDS_READ_DIDS[bmwFUDSProbeDidIndex];
    Serial.print("BMW_F UDS ReadDID timeout 0x");
    Serial.println(did, HEX);
    bmwFUDSProbeDidIndex++;
    sendNextBMWFUDSReadDID();
  } else if (bmwFUDSProbeState == BMWF_UDS_WAIT_WRITE_FC) {
    Serial.print("BMW_F UDS FlowControl timeout for RTC WriteDID 0x");
    Serial.print(bmwFUDSCurrentWriteDid, HEX);
    Serial.println("; sending consecutive frame fallback");
    setBMWFUDSProbeStatus("UDS FlowControl timeout; sending RTC write consecutive-frame fallback.");
    sendBMWFUDSConsecutiveFrames(0, 0);
  } else if (bmwFUDSProbeState == BMWF_UDS_WAIT_WRITE_DID) {
    Serial.print("BMW_F UDS RTC WriteDID timeout 0x");
    Serial.println(bmwFUDSCurrentWriteDid, HEX);
    bmwFUDSProbeDidIndex++;
    sendNextBMWFUDSRTCWriteDID();
  } else if (bmwFUDSProbeState == BMWF_UDS_WAIT_SECURITY_SEED) {
    Serial.println("BMW_F UDS SecurityAccess seed timeout");
    bmwFUDSProbeState = BMWF_UDS_DONE;
    bmwFUDSSecuritySeedMode = false;
    setBMWFUDSProbeStatus("UDS SecurityAccess seed timeout.");
  } else if (bmwFUDSProbeState == BMWF_UDS_SCAN_WAIT_TARGET) {
    bmwFUDSScanTargetId++;
    sendNextBMWFUDSScanTarget();
  } else if (bmwFUDSProbeState == BMWF_UDS_SWEEP_WAIT_DID) {
    // No response for this DID; record nothing and move on.
    bmwFUDSSweepDid++;
    sendNextBMWFUDSSweepDid();
  } else if (bmwFUDSProbeState == BMWF_UDS_WAIT_SINGLE_READ) {
    setBMWFUDSProbeStatus("RTC read 0xD113 timed out (no response from 0x86).");
    Serial.println("BMW_F UDS RTC read 0xD113 timeout");
    bmwFUDSProbeState = BMWF_UDS_DONE;
  }
}

void handleBMWFUDSResponse(unsigned long id, unsigned char len, const unsigned char *data) {
  unsigned long baseId = id & 0x1FFFFFFF;
  if (baseId < 0x600 || baseId > 0x6FF || len < 3) {
    return;
  }

  printBMWFUDSFrame("RX", id, len, data);

  uint8_t responseSourceId = baseId & 0xFF;
  uint8_t responseTargetId = data[0];
  uint8_t pciType = data[1] & 0xF0;
  bool responseIsFirstFrame = pciType == 0x10 && len >= 4;

  if (responseTargetId != bmwFUDSRequestSourceId) {
    logBMWFUDSPassiveSummary(responseSourceId, responseTargetId, len, data);
    updateBMWFUDSPassiveAssembly(responseSourceId, responseTargetId, len, data);
    if (bmwFUDSPassiveFlowControlEnabled && responseIsFirstFrame && isBMWFUDSDiagnosticSource(responseTargetId)) {
      sendBMWFUDSFlowControlFromSource(responseTargetId, responseSourceId, "PFC");
    }
    return;
  }

  if (bmwFUDSProbeState == BMWF_UDS_SCAN_WAIT_TARGET) {
    bmwFUDSScanFoundCount++;
    Serial.print("BMW_F UDS target FOUND 0x");
    printCanHexByte(responseSourceId);
    Serial.print(" while scanning 0x");
    printCanHexByte(bmwFUDSTargetId);
    Serial.println();
    bmwFUDSScanTargetId++;
    sendNextBMWFUDSScanTarget();
    return;
  }

  if (responseSourceId != bmwFUDSTargetId) {
    logBMWFUDSPassiveSummary(responseSourceId, responseTargetId, len, data);
    updateBMWFUDSPassiveAssembly(responseSourceId, responseTargetId, len, data);
    if (bmwFUDSPassiveFlowControlEnabled && responseIsFirstFrame && isBMWFUDSDiagnosticSource(responseTargetId)) {
      sendBMWFUDSFlowControlFromSource(responseTargetId, responseSourceId, "PFC");
    }
    return;
  }

  uint8_t udsOffset = 2;
  if (pciType == 0x30) {
    uint8_t blockSize = len > 2 ? data[2] : 0;
    uint8_t stMin = len > 3 ? data[3] : 0;
    Serial.print("BMW_F UDS FlowControl RX blockSize=");
    Serial.print(blockSize);
    Serial.print(" stMin=");
    Serial.println(stMin);
    if (bmwFUDSProbeState == BMWF_UDS_WAIT_WRITE_FC) {
      sendBMWFUDSConsecutiveFrames(blockSize, stMin);
    } else {
      setBMWFUDSProbeStatus("UDS FlowControl received outside write transfer; see Serial Monitor.");
    }
    return;
  } else if (pciType == 0x10 && len >= 4) {
    startBMWFUDSActiveAssembly(responseSourceId, responseTargetId, len, data);  // setup state first
    sendBMWFUDSFlowControl();                                                   // FC sent after setup so KOMBI starts CFs only after we're ready
    return;
  } else if (pciType == 0x20) {
    updateBMWFUDSActiveAssembly(responseSourceId, responseTargetId, len, data);
    return;
  } else if (pciType != 0x00) {
    setBMWFUDSProbeStatus("UDS response has unsupported ISO-TP PCI type; see Serial Monitor.");
    return;
  }

  // Single-frame UDS response during a sweep: log + advance, skip normal dispatch.
  if (bmwFUDSSweepMode && bmwFUDSProbeState == BMWF_UDS_SWEEP_WAIT_DID) {
    uint8_t sfLen = data[1] & 0x0F;  // ISO-TP single-frame UDS payload length
    uint8_t avail = (len > udsOffset) ? (len - udsOffset) : 0;
    if (sfLen > avail) sfLen = avail;
    bmwFUDSSweepHandleUdsResponse(&data[udsOffset], sfLen);
    return;
  }

  // Single-shot RTC read (short/single-frame variant): report and stop.
  if (bmwFUDSProbeState == BMWF_UDS_WAIT_SINGLE_READ) {
    uint8_t sfLen = data[1] & 0x0F;
    uint8_t avail = (len > udsOffset) ? (len - udsOffset) : 0;
    if (sfLen > avail) sfLen = avail;
    reportBMWFUDSSingleRead(&data[udsOffset], sfLen);
    bmwFUDSProbeState = BMWF_UDS_DONE;
    return;
  }

  uint8_t sid = data[udsOffset];
  if (sid == 0x50) {
    setBMWFUDSProbeStatus("UDS Diagnostic Session accepted by target.");
    if (bmwFUDSProbeState == BMWF_UDS_WAIT_SESSION || bmwFUDSProbeState == BMWF_UDS_WAIT_DEFAULT_SESSION) {
      if (bmwFUDSRTCWriteMode) {
        sendNextBMWFUDSRTCWriteDID();
      } else if (bmwFUDSSecuritySeedMode) {
        sendBMWFUDSSecuritySeedRequest();
      } else if (bmwFUDSSweepMode) {
        sendNextBMWFUDSSweepDid();
      } else {
        sendNextBMWFUDSReadDID();
      }
    }
    return;
  }

  if (sid == 0x62 && len > udsOffset + 2) {
    uint16_t did = ((uint16_t)data[udsOffset + 1] << 8) | data[udsOffset + 2];
    snprintf(bmwFUDSProbeStatus, sizeof(bmwFUDSProbeStatus), "UDS ReadDID positive response for 0x%04X. See Serial Monitor.", did);
    if (bmwFUDSProbeState == BMWF_UDS_WAIT_DID) {
      bmwFUDSProbeDidIndex++;
      sendNextBMWFUDSReadDID();
    }
    return;
  }

  if (sid == 0x67 && len > udsOffset + 1) {
    uint8_t level = data[udsOffset + 1];
    Serial.print("BMW_F UDS SecurityAccess positive level=0x");
    printCanHexByte(level);
    Serial.print(" seed=");
    for (uint8_t i = udsOffset + 2; i < len; i++) {
      if (i > udsOffset + 2) {
        Serial.print(' ');
      }
      printCanHexByte(data[i]);
    }
    Serial.println();
    snprintf(bmwFUDSProbeStatus, sizeof(bmwFUDSProbeStatus), "UDS SecurityAccess seed positive level 0x%02X. See Serial Monitor seed bytes.", level);
    bmwFUDSProbeState = BMWF_UDS_DONE;
    bmwFUDSSecuritySeedMode = false;
    return;
  }

  if (sid == 0x6E && len > udsOffset + 2) {
    uint16_t did = ((uint16_t)data[udsOffset + 1] << 8) | data[udsOffset + 2];
    snprintf(bmwFUDSProbeStatus, sizeof(bmwFUDSProbeStatus), "UDS RTC WriteDID positive response for 0x%04X. Check if clock warning disappeared.", did);
    Serial.print("BMW_F UDS RTC WriteDID POSITIVE 0x");
    Serial.println(did, HEX);
    bmwFUDSProbeState = BMWF_UDS_DONE;
    bmwFUDSRTCWriteMode = false;
    return;
  }

  if (sid == 0x7F && len > udsOffset + 2) {
    uint8_t rejectedService = data[udsOffset + 1];
    uint8_t nrc = data[udsOffset + 2];
    snprintf(bmwFUDSProbeStatus, sizeof(bmwFUDSProbeStatus), "UDS negative response: service 0x%02X NRC 0x%02X. See Serial Monitor.", rejectedService, nrc);
    if (rejectedService == 0x2E && bmwFUDSRTCWriteMode) {
      Serial.print("BMW_F UDS RTC WriteDID NRC for 0x");
      Serial.print(bmwFUDSCurrentWriteDid, HEX);
      Serial.print(" nrc=0x");
      printCanHexByte(nrc);
      Serial.println();
      bmwFUDSProbeDidIndex++;
      sendNextBMWFUDSRTCWriteDID();
      return;
    }
    if (rejectedService == 0x27 && bmwFUDSSecuritySeedMode) {
      Serial.print("BMW_F UDS SecurityAccess seed NRC nrc=0x");
      printCanHexByte(nrc);
      Serial.println();
      bmwFUDSProbeState = BMWF_UDS_DONE;
      bmwFUDSSecuritySeedMode = false;
      return;
    }
    if (bmwFUDSProbeState == BMWF_UDS_WAIT_SESSION || bmwFUDSProbeState == BMWF_UDS_WAIT_DEFAULT_SESSION) {
      if (bmwFUDSRTCWriteMode) {
        sendNextBMWFUDSRTCWriteDID();
      } else if (bmwFUDSSecuritySeedMode) {
        sendBMWFUDSSecuritySeedRequest();
      } else if (bmwFUDSSweepMode) {
        sendNextBMWFUDSSweepDid();
      } else {
        sendNextBMWFUDSReadDID();
      }
    } else if (bmwFUDSProbeState == BMWF_UDS_WAIT_DID) {
      bmwFUDSProbeDidIndex++;
      sendNextBMWFUDSReadDID();
    }
    return;
  }

  snprintf(bmwFUDSProbeStatus, sizeof(bmwFUDSProbeStatus), "UDS response SID 0x%02X received. See Serial Monitor.", sid);
}

void setup() {
  // Define the outputs
  pinMode(SPI_CS_PIN, OUTPUT);
  pinMode(CAN_INT, INPUT);

  //Begin with Serial Connection
  Serial.begin(SERIAL_BAUD_RATE);

  delay(1000);
  Serial.println("Starting CarCluster...");

#if WIFI_ENABLED == 1
  wifiFunctions.begin(WIFI_CONFIG_PORTAL_ACCESS_POINT_NAME, WIFI_CONFIG_PORTAL_ACCESS_POINT_PASSWORD, WIFI_CONFIG_PORTAL_TIMEOUT);

  webServerSetHttpHandlers("state", webDashboardGetState, webDashBoardSetState);
  webServerSetHttpHandlers("steering_button_pressed", webDashboardCheckSteeringButtonPressed, webDashboardSetSteeringButtonPressed);
  webServerAddRoute("/bmw-f-controls", webDashboardBMWFControls);
  webServerAddRoute("/bmw-f-uds-log.txt", webDashboardBMWFUDSLog);
  webServerAddRoute("/test", webDashboardBMWFTestControls);
  webServerAddRoute("/test/vu", webDashboardAudioVU);
  webServerInit();
  forzaHorizonGame.begin();
  beamNGGame.begin();
#endif

  simhubGame.begin();

  INT8U canSpeed = CAN_500KBPS;

  //Begin with CAN Bus Initialization
START_INIT:
  if (CAN_OK == CAN.begin(MCP_ANY, canSpeed, MCP_8MHZ))  // init can bus
  {
    Serial.println("CAN BUS Shield init ok!");
  } else {
    Serial.println("CAN BUS Shield init fail");
    Serial.println("Init CAN BUS Shield again");
    delay(100);
    goto START_INIT;
  }
  CAN.setMode(MCP_NORMAL);
}

void loop() {
  // Update the cluster with current state of the game
  cluster.updateWithGame(game);

  // Serial message handling
  readSerialJson();

  // Handle data from connected CAN hardware
  readCanBuffer();
  updateBMWFUDSClockProbe();  // drives the single-read state machine (VIN read)

// Update the web dashboard
#if WIFI_ENABLED == 1
  webDashboard.update();
  webServerPoll();
#endif
}

void readSerialJson() {
  //Check to see if anything is available in the serial receive buffer
  while (Serial.available() > 0) {
    //Create a place to hold the incoming message
    static char message[MAX_SERIAL_MESSAGE_LENGTH];
    static unsigned int message_pos = 0;

    //Read the next available byte in the serial receive buffer
    char inByte = Serial.read();

    //Message coming in (check not terminating character) and guard for over message size
    if (inByte != '\n' && (message_pos < MAX_SERIAL_MESSAGE_LENGTH - 1)) {
      //Add the incoming byte to our message
      message[message_pos] = inByte;
      message_pos++;
    } else {
      //Add null character to string
      message[message_pos] = '\0';

#if SERIAL_LOG_SERIAL_RX_MESSAGES == 1
      Serial.print("SERIAL RX ");
      Serial.println(message);
#endif

      DeserializationError error = deserializeJson(doc, message);
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        message_pos = 0;
        return;
      }

      uint8_t action = doc["action"];

      // Action 0 means "send following message to CAN bus"
      // Example: {"action":0, "address":1644, "p1":128, "p2":20, "p3":76, "p4":85, "p5":9, "p6":66, "p7":108, "p8":117}
      if (action == 0) {
        short address = doc["address"];
        uint8_t p1 = doc["p1"];
        uint8_t p2 = doc["p2"];
        uint8_t p3 = doc["p3"];
        uint8_t p4 = doc["p4"];
        uint8_t p5 = doc["p5"];
        uint8_t p6 = doc["p6"];
        uint8_t p7 = doc["p7"];
        uint8_t p8 = doc["p8"];
        Serial.println(address);

        unsigned char DataToSend[8] = { p1, p2, p3, p4, p5, p6, p7, p8 };
        CAN.sendMsgBuf(address, 0, 8, DataToSend);
      } else if (action == 10) {
        // Used to decode custom protocol from Simhub in the following format:
        // {"action":10, "spe":54, "gea":"2", "rpm":3590, "mrp":7999, "lft":0, "rit":0, "oit":0, "pau":0, "run":0, "fue":0, "hnb":0, "abs":0, "tra":0}
        simhubGame.decodeSerialData(doc);
      } else if (action == 12) {
        // Force cruise control state for bench testing the 0x289 telltale/marker,
        // independent of BeamNG. Example: {"action":12,"active":1,"speed":120}
        // Note: if BeamNG telemetry is also streaming it will overwrite this each
        // packet, so test with the game's telemetry stopped.
        game.cruiseControlActive = ((int)doc["active"] != 0);
        game.cruiseControlSetSpeed = (int)doc["speed"];
        Serial.print("Cruise test: active=");
        Serial.print(game.cruiseControlActive);
        Serial.print(" speed=");
        Serial.println(game.cruiseControlSetSpeed);
      } else if (action == 13) {
        // Live-tune the 0x289 cruise set-speed MARKER bytes to reverse-engineer the
        // green speedo-ring marker without reflashing. Forces cruise on and overrides
        // 0x289 data bytes d1,d2,d4,d5,d6 (decimal). d3 stays 0xE3 (telltale), CRC auto.
        // {"action":13,"on":1,"d1":56,"d2":224,"d4":134,"d5":12,"d6":60}
        // Send {"action":13,"on":0} to drop back to the stable icon-only frame.
        game.cruiseControlActive = true;
        cluster.setCruiseMarkerTest(
          ((int)doc["on"] != 0),
          (uint8_t)(int)doc["d1"], (uint8_t)(int)doc["d2"],
          (uint8_t)(int)doc["d4"], (uint8_t)(int)doc["d5"], (uint8_t)(int)doc["d6"]);
        Serial.println("Cruise marker test updated");
      } else if (action == 14) {
        // Repeat ANY raw CAN frame every cycle, for hunting the speed-ring marker
        // (e.g. 0x287) or other indicators. id decimal, d0..d7 decimal bytes.
        // {"action":14,"on":1,"id":647,"d0":5,"d1":20,"d2":16,"d3":16,"d4":0,"d5":255,"d6":255,"d7":255}
        // 647 = 0x287. Send {"action":14,"on":0} to stop.
        uint8_t rd[8] = {
          (uint8_t)(int)doc["d0"], (uint8_t)(int)doc["d1"], (uint8_t)(int)doc["d2"], (uint8_t)(int)doc["d3"],
          (uint8_t)(int)doc["d4"], (uint8_t)(int)doc["d5"], (uint8_t)(int)doc["d6"], (uint8_t)(int)doc["d7"]
        };
        cluster.setRawTestFrame(((int)doc["on"] != 0), (uint16_t)(int)doc["id"], rd);
        Serial.println("Raw test frame updated");
      } else if (action == 15) {
        // Fuel sender calibration. Forces the 0x349 fuel sender value so you can
        // read the needle at a known input and map the cluster's tank curve.
        // {"action":15,"on":1,"value":2000}   (try 750..9500). on:0 = back to BeamNG.
        cluster.setFuelOverride(((int)doc["on"] != 0), (uint16_t)(int)doc["value"]);
        Serial.print("Fuel override: on=");
        Serial.print((int)doc["on"]);
        Serial.print(" value=");
        Serial.println((int)doc["value"]);
      }

      //Reset for the next message
      message_pos = 0;
    }
  }
}

void readCanBuffer() {
  // Drain ALL pending frames each loop pass. The MCP2515 has only two RX buffers,
  // so reading a single frame per loop iteration (the old behaviour) overflowed the
  // controller during ISO-TP multi-frame bursts — consecutive frames are spaced
  // only a few ms apart, far faster than one loop() pass (web server + cluster TX).
  // Dropped consecutive frames are exactly why UDS multi-frame reads were never
  // fully captured. We now empty both buffers, bounded by a safety cap so a busy
  // bus can never starve the rest of loop().
  uint8_t framesDrained = 0;
  while (CAN.checkReceive() == CAN_MSGAVAIL && framesDrained < BMWF_CAN_RX_DRAIN_LIMIT) {
    INT8U canReadStatus = CAN.readMsgBuf(&canRxId, &canRxLen, canRxBuf);  // len = data length, buf = data byte(s)
    framesDrained++;
    if (canReadStatus == CAN_OK) {
#if SERIAL_LOG_CAN_RX_ALL == 1
      logBMWFCanRxFrame(canRxId, canRxLen, canRxBuf);
#endif
      handleBMWFUDSResponse(canRxId, canRxLen, canRxBuf);  // needed for the VIN/single-read response
    } else {
      Serial.print("CAN RX read failed status=");
      Serial.println(canReadStatus);
      break;
    }
  }
}
