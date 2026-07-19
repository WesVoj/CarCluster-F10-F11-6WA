// SPDX-FileCopyrightText: 2026 WesVoj
// SPDX-License-Identifier: GPL-3.0-only

#include "HttpWebServer.h"

#include <WebServer.h>
#include <stdarg.h>

#include "../Libs/ArduinoJson/ArduinoJson.h"

namespace {

WebServer server(80);
DashboardStateGetter stateGetter = nullptr;
DashboardStateSetter stateSetter = nullptr;
DashboardActionChecker actionChecker = nullptr;
DashboardActionStarter actionStarter = nullptr;

static const char DASHBOARD_PAGE[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>CarCluster F10/F11</title>
  <style>
    :root{color-scheme:light;--bg:#f4f7fb;--panel:#fff;--line:#dbe3ee;--text:#172033;--muted:#667085;--blue:#1769d2;--blue-soft:#eaf2ff}
    *{box-sizing:border-box}body{font-family:Arial,sans-serif;margin:0;background:var(--bg);color:var(--text)}
    header{background:var(--panel);border-bottom:1px solid var(--line)}.header-inner,main{max-width:920px;margin:0 auto;padding:20px}
    h1{font-size:27px;margin:0 0 6px}h2{font-size:17px;margin:0 0 14px}.lead,.meta{color:var(--muted);line-height:1.45}.lead{margin:0}
    nav{display:flex;flex-wrap:wrap;gap:9px;margin:18px 0 4px}a{color:var(--blue);text-decoration:none;font-weight:700}
    nav a{background:var(--blue-soft);border:1px solid #cfe0fb;border-radius:7px;padding:9px 12px}
    section{background:var(--panel);border:1px solid var(--line);border-radius:9px;padding:17px;margin:14px 0;box-shadow:0 5px 18px rgba(25,42,70,.05)}
    .grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:15px}.field{display:flex;flex-direction:column;gap:7px}
    label{font-size:13px;font-weight:700}.row{display:flex;align-items:center;justify-content:space-between;gap:15px;padding:10px 0;border-top:1px solid #edf1f6}.row:first-child{border-top:0}
    input[type=range],select{width:100%}input[type=range]{accent-color:var(--blue)}select{min-height:40px;padding:8px;border:1px solid #c9d4e2;border-radius:7px;background:#fff;color:var(--text)}
    input[type=checkbox]{width:21px;height:21px;accent-color:var(--blue)}.value{color:var(--blue);font-variant-numeric:tabular-nums}
    button{border:0;border-radius:7px;padding:10px 13px;background:var(--blue);color:#fff;font-weight:700;cursor:pointer}.buttons{display:flex;flex-wrap:wrap;gap:9px}
    #status{min-height:18px;margin:12px 0 0;color:var(--muted);font-size:13px}#status.ok{color:#18794e}#status.error{color:#c53030}
    @media(max-width:650px){.header-inner,main{padding:15px}.grid{grid-template-columns:1fr}}
  </style>
</head>
<body>
<header><div class="header-inner"><h1>CarCluster F10/F11 6WA</h1><p class="lead">Direct control of a verified BMW F10/F11 6WA diesel instrument cluster.</p></div></header>
<main>
  <nav><a href="/bmw-f-controls">BMW F controls</a><a href="/test">Experimental tests</a><a href="/test/vu">Spotify / Voicemeeter VU</a></nav>
  <section><h2>Needles and gauges</h2><div class="grid">
    <div class="field"><label for="speed">Speed: <span id="speedValue" class="value">0 km/h</span></label><input id="speed" type="range" min="0" max="260" value="0"></div>
    <div class="field"><label for="rpm">RPM: <span id="rpmValue" class="value">0 RPM</span></label><input id="rpm" type="range" min="0" max="6000" step="100" value="0"></div>
    <div class="field"><label for="fuel">Fuel: <span id="fuelValue" class="value">0%</span></label><input id="fuel" type="range" min="0" max="100" value="0"></div>
    <div class="field"><label for="backlight">Backlight: <span id="backlightValue" class="value">0%</span></label><input id="backlight" type="range" min="0" max="100" value="0"></div>
    <div class="field"><label for="coolant_temp">Coolant: <span id="coolantValue" class="value">0 C</span></label><input id="coolant_temp" type="range" min="50" max="150" value="90"></div>
    <div class="field"><label for="outdoor_temp">Outside temperature: <span id="outdoorValue" class="value">0 C</span></label><input id="outdoor_temp" type="range" min="-30" max="50" value="20"></div>
    <div class="field"><label for="gear">Gear</label><select id="gear"><option>P</option><option>R</option><option>N</option><option>D</option><option>S</option><option>1</option><option>2</option><option>3</option><option>4</option><option>5</option><option>6</option><option>7</option><option>8</option></select></div>
    <div class="field"><label for="drive_mode">Drive mode</label><select id="drive_mode"><option>Traction</option><option>Comfort</option><option>Sport</option><option>Sport+</option><option>DSC off</option><option>Eco pro</option></select></div>
  </div></section>
  <section><h2>Lights and status</h2><div id="toggles"></div></section>
  <section><h2>Steering-wheel actions</h2><div class="buttons"><button data-action="1">Button 1</button><button data-action="2">Button 2</button><button data-action="3">Button 3</button></div><p class="meta">Availability depends on the cluster coding and the enabled test frames.</p></section>
  <p id="status">Loading cluster state...</p>
</main>
<script>
(() => {
  'use strict';
  const $=id=>document.getElementById(id),status=$('status');
  const toggleFields=[['high_beam','High beam'],['main_lights','Main lights'],['left_indicator','Left indicator'],['right_indicator','Right indicator'],['fog_front','Front fog light'],['fog_rear','Rear fog light'],['door_open','Door open'],['dsc','DSC / traction warning'],['abs','ABS'],['handbrake','Parking brake'],['ignition','Ignition'],['indicators_blink','Firmware-controlled indicator blinking']];
  const setStatus=(message,kind='')=>{status.textContent=message;status.className=kind};
  const toggles=$('toggles');
  toggleFields.forEach(([id,label])=>{const row=document.createElement('label');row.className='row';row.innerHTML=`<span>${label}</span><input id="${id}" type="checkbox">`;toggles.appendChild(row)});
  async function write(patch){const response=await fetch('/api/state',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(patch)});if(!response.ok)throw new Error('HTTP '+response.status);setStatus('Saved.','ok')}
  function bind(id,type='number',output=null,suffix=''){const input=$(id);input.addEventListener('change',()=>write({[id]:type==='bool'?input.checked:type==='number'?Number(input.value):input.value}).catch(error=>setStatus(error.message,'error')));if(output)input.addEventListener('input',()=>{$(output).textContent=input.value+suffix})}
  bind('speed','number','speedValue',' km/h');bind('rpm','number','rpmValue',' RPM');bind('fuel','number','fuelValue','%');bind('backlight','number','backlightValue','%');bind('coolant_temp','number','coolantValue',' C');bind('outdoor_temp','number','outdoorValue',' C');bind('gear','string');bind('drive_mode','string');toggleFields.forEach(([id])=>bind(id,'bool'));
  document.querySelectorAll('[data-action]').forEach(button=>button.addEventListener('click',()=>fetch('/api/steering_button_pressed',{method:'POST',headers:{'Content-Type':'text/plain'},body:button.dataset.action}).then(()=>setStatus('Action sent.','ok')).catch(error=>setStatus(error.message,'error'))));
  fetch('/api/state',{cache:'no-store'}).then(response=>response.json()).then(state=>{Object.entries(state).forEach(([key,value])=>{const input=$(key);if(!input)return;if(input.type==='checkbox')input.checked=Boolean(value);else input.value=value});$('speed').max=state.maximumSpeed||260;$('rpm').max=state.maximumRPM||6000;$('coolant_temp').min=state.minimumCoolantTemp||50;$('coolant_temp').max=state.maximumCoolantTemp||150;['speed','rpm','fuel','backlight','coolant_temp','outdoor_temp'].forEach(id=>$(id).dispatchEvent(new Event('input')));setStatus('Ready.','ok')}).catch(error=>setStatus('Could not load state: '+error.message,'error'));
})();
</script>
</body>
</html>
)HTML";

void addCorsHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Private-Network", "true");
}

void sendStateJson() {
  if (stateGetter == nullptr) {
    server.send(503, "application/json", "{\"error\":\"State handler is not ready\"}");
    return;
  }

  DashboardState state = {};
  stateGetter(&state);
  char response[768];
  snprintf(
    response,
    sizeof(response),
    "{\"speed\":%d,\"maximumSpeed\":%d,\"rpm\":%d,\"maximumRPM\":%d,"
    "\"gear\":\"%s\",\"fuel\":%d,\"backlight\":%d,\"coolant_temp\":%d,"
    "\"maximumCoolantTemp\":%d,\"minimumCoolantTemp\":%d,\"outdoor_temp\":%d,"
    "\"high_beam\":%s,\"main_lights\":%s,\"left_indicator\":%s,\"right_indicator\":%s,"
    "\"fog_front\":%s,\"fog_rear\":%s,\"door_open\":%s,\"dsc\":%s,\"abs\":%s,"
    "\"handbrake\":%s,\"ignition\":%s,\"indicators_blink\":%s,\"drive_mode\":\"%s\"}",
    state.speed, state.maximumSpeed, state.rpm, state.maximumRPM, state.gear, state.fuel,
    state.backlight, state.coolant_temp, state.maximumCoolantTemp, state.minimumCoolantTemp,
    state.outdoor_temp, state.high_beam ? "true" : "false", state.main_lights ? "true" : "false",
    state.left_indicator ? "true" : "false", state.right_indicator ? "true" : "false",
    state.fog_front ? "true" : "false", state.fog_rear ? "true" : "false",
    state.door_open ? "true" : "false", state.dsc ? "true" : "false", state.abs ? "true" : "false",
    state.handbrake ? "true" : "false", state.ignition ? "true" : "false",
    state.indicators_blink ? "true" : "false", state.drive_mode);

  addCorsHeaders();
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", response);
}

template <typename T>
void assignJsonNumber(JsonDocument &document, const char *name, T &target) {
  if (!document[name].isNull()) target = document[name].as<T>();
}

void updateStateFromRequest() {
  if (stateGetter == nullptr || stateSetter == nullptr) {
    server.send(503, "application/json", "{\"error\":\"State handler is not ready\"}");
    return;
  }

  DashboardState state = {};
  stateGetter(&state);
  const String body = server.arg("plain");
  JsonDocument document;
  const DeserializationError error = deserializeJson(document, body);
  if (error) {
    addCorsHeaders();
    server.send(400, "application/json", "{\"error\":\"Invalid JSON body\"}");
    return;
  }

  assignJsonNumber(document, "speed", state.speed);
  assignJsonNumber(document, "rpm", state.rpm);
  assignJsonNumber(document, "fuel", state.fuel);
  assignJsonNumber(document, "backlight", state.backlight);
  assignJsonNumber(document, "coolant_temp", state.coolant_temp);
  assignJsonNumber(document, "outdoor_temp", state.outdoor_temp);
  assignJsonNumber(document, "high_beam", state.high_beam);
  assignJsonNumber(document, "main_lights", state.main_lights);
  assignJsonNumber(document, "left_indicator", state.left_indicator);
  assignJsonNumber(document, "right_indicator", state.right_indicator);
  assignJsonNumber(document, "fog_front", state.fog_front);
  assignJsonNumber(document, "fog_rear", state.fog_rear);
  assignJsonNumber(document, "door_open", state.door_open);
  assignJsonNumber(document, "dsc", state.dsc);
  assignJsonNumber(document, "abs", state.abs);
  assignJsonNumber(document, "handbrake", state.handbrake);
  assignJsonNumber(document, "ignition", state.ignition);
  assignJsonNumber(document, "indicators_blink", state.indicators_blink);
  const char *gear = document["gear"].isNull() ? nullptr : document["gear"].as<const char *>();
  const char *driveMode = document["drive_mode"].isNull() ? nullptr : document["drive_mode"].as<const char *>();
  if (gear != nullptr) strlcpy(state.gear, gear, sizeof(state.gear));
  if (driveMode != nullptr) strlcpy(state.drive_mode, driveMode, sizeof(state.drive_mode));

  state.speed = constrain(state.speed, 0, state.maximumSpeed);
  state.rpm = constrain(state.rpm, 0, state.maximumRPM);
  state.fuel = constrain(state.fuel, 0, 100);
  state.backlight = constrain(state.backlight, 0, 100);
  state.coolant_temp = constrain(state.coolant_temp, state.minimumCoolantTemp, state.maximumCoolantTemp);
  state.outdoor_temp = constrain(state.outdoor_temp, -50, 85);

  stateSetter(&state);
  sendStateJson();
}

void sendDashboard() {
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "text/html; charset=utf-8", DASHBOARD_PAGE);
}

void invokeCustomHandler(HttpEventHandler handler) {
  HttpConnection connection;
  const String body = server.arg("plain");
  HttpMessage message = {
    server.method() == HTTP_POST ? httpString("POST") : httpString("GET"),
    {body.c_str(), body.length()}
  };
  handler(&connection, HTTP_EVENT_REQUEST, &message);
}

void forwardResponseHeaders(const char *headers, String &contentType) {
  if (headers == nullptr) return;
  String remaining(headers);
  int start = 0;
  while (start < remaining.length()) {
    int end = remaining.indexOf("\r\n", start);
    if (end < 0) end = remaining.length();
    String line = remaining.substring(start, end);
    const int colon = line.indexOf(':');
    if (colon > 0) {
      String name = line.substring(0, colon);
      String value = line.substring(colon + 1);
      value.trim();
      if (name.equalsIgnoreCase("Content-Type")) contentType = value;
      else if (!name.equalsIgnoreCase("Connection") && !name.equalsIgnoreCase("Content-Length")) server.sendHeader(name, value);
    }
    start = end + 2;
  }
}

}  // namespace

void webServerSetHttpHandlers(const char *name, DashboardStateGetter getter, DashboardStateSetter setter) {
  if (name != nullptr && strcmp(name, "state") == 0) {
    stateGetter = getter;
    stateSetter = setter;
  }
}

void webServerSetHttpHandlers(const char *name, DashboardActionChecker checker, DashboardActionStarter starter) {
  if (name != nullptr && strcmp(name, "steering_button_pressed") == 0) {
    actionChecker = checker;
    actionStarter = starter;
  }
}

void webServerAddRoute(const char *path, HttpEventHandler handler) {
  server.on(path, HTTP_ANY, [handler]() { invokeCustomHandler(handler); });
}

void webServerInit() {
  server.on("/", HTTP_GET, sendDashboard);
  server.on("/api/state", HTTP_GET, sendStateJson);
  server.on("/api/state", HTTP_POST, updateStateFromRequest);
  server.on("/api/state", HTTP_OPTIONS, []() { addCorsHeaders(); server.send(204); });
  server.on("/api/steering_button_pressed", HTTP_POST, []() {
    if (actionStarter != nullptr) {
      String value = server.arg("plain");
      if (value.length() == 0 && server.args() > 0) value = server.arg(0);
      actionStarter({value.c_str(), value.length()});
    }
    addCorsHeaders();
    server.send(204);
  });
  server.onNotFound([]() { server.send(404, "text/plain; charset=utf-8", "Not found\n"); });
  server.begin();
  Serial.println("HTTP dashboard started on port 80");
}

void webServerPoll() {
  server.handleClient();
}

void webServerNotifyStateChanged() {
  // State is read directly for every request; no cache invalidation is needed.
}

HttpString httpString(const char *value) {
  return {value, value == nullptr ? 0U : strlen(value)};
}

int httpStringCompare(HttpString left, HttpString right) {
  if (left.len != right.len) return left.len < right.len ? -1 : 1;
  return left.len == 0 ? 0 : memcmp(left.buf, right.buf, left.len);
}

int httpGetVar(const HttpString *, const char *name, char *destination, size_t destinationSize) {
  if (name == nullptr || destination == nullptr || destinationSize == 0 || !server.hasArg(name)) return 0;
  const String value = server.arg(name);
  strlcpy(destination, value.c_str(), destinationSize);
  return value.length() > 0 ? value.length() : 1;
}

void httpReply(HttpConnection *, int statusCode, const char *headers, const char *format, ...) {
  String contentType = "text/plain; charset=utf-8";
  forwardResponseHeaders(headers, contentType);

  va_list arguments;
  va_start(arguments, format);
  va_list sizeArguments;
  va_copy(sizeArguments, arguments);
  const int required = vsnprintf(nullptr, 0, format == nullptr ? "" : format, sizeArguments);
  va_end(sizeArguments);

  if (required < 0) {
    va_end(arguments);
    server.send(500, "text/plain", "Response formatting failed\n");
    return;
  }

  char *body = static_cast<char *>(malloc(static_cast<size_t>(required) + 1));
  if (body == nullptr) {
    va_end(arguments);
    server.send(500, "text/plain", "Not enough memory to render the page\n");
    return;
  }
  vsnprintf(body, static_cast<size_t>(required) + 1, format == nullptr ? "" : format, arguments);
  va_end(arguments);

  server.setContentLength(static_cast<size_t>(required));
  server.send(statusCode, contentType.c_str(), "");
  if (required > 0) server.sendContent(body, static_cast<size_t>(required));
  free(body);
}

void httpSendDownload(HttpConnection *, const char *filename, const char *data, size_t length) {
  server.sendHeader("Content-Disposition", String("attachment; filename=\"") + filename + "\"");
  server.sendHeader("Cache-Control", "no-store");
  server.setContentLength(length);
  server.send(200, "text/plain; charset=utf-8", "");
  if (data != nullptr && length > 0) server.sendContent(data, length);
}
