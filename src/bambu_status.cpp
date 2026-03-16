#include "wled.h"
#include "bambu_status.h"

static const char* STATE_NAMES[BAMBU_STATE_COUNT] = {
  "printing","heating","cooling","idle","downloading","error"
};

void BambuUsermod::setup() {
  _defaultEffects();
  // Routes registered in loop() after server is ready
}

void BambuUsermod::loop() {
  if (!_routesDone) {
    _registerRoutes();
    _routesDone = true;
  }
  _poll();
  _applyEffect();
}

void BambuUsermod::_registerRoutes() {
  server.on("/bambu", HTTP_GET, [this](AsyncWebServerRequest* req) {
    req->send(WLED_FS, "/bambu.htm", "text/html");
  });

  server.on("/bambu/status", HTTP_GET, [this](AsyncWebServerRequest* req) {
    String json = "{\"state\":\"" + _state + "\",\"ip\":\"" + _ip + "\",\"enabled\":" + (_enabled?"true":"false") + "}";
    req->send(200, "application/json", json);
  });

  server.on("/bambu/config", HTTP_POST,
    [](AsyncWebServerRequest* req){},
    NULL,
    [this](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t, size_t) {
      DynamicJsonDocument doc(512);
      if (!deserializeJson(doc, data, len)) {
        if (doc.containsKey("ip"))      _ip      = doc["ip"].as<String>();
        if (doc.containsKey("enabled")) _enabled = doc["enabled"].as<bool>();
        serializeConfig();
      }
      req->send(200, "application/json", "{\"ok\":true}");
    }
  );
}

void BambuUsermod::_poll() {
  if (!_enabled || _ip.length() < 7) return;
  if (millis() - _lastPoll < 2000) return;
  _lastPoll = millis();

  WiFiClient client;
  if (!client.connect(_ip.c_str(), 80)) return;

  client.println("GET /server/info HTTP/1.1");
  client.print("Host: "); client.println(_ip);
  client.println("Connection: close");
  client.println();

  String payload = "";
  unsigned long t = millis();
  while (client.connected() || client.available()) {
    if (millis() - t > 3000) break;
    while (client.available()) { payload += char(client.read()); t = millis(); }
    delay(1);
  }
  client.stop();

  int body = payload.indexOf("\r\n\r\n");
  if (body < 0) return;

  DynamicJsonDocument doc(1024);
  if (deserializeJson(doc, payload.substring(body + 4))) return;
  if (!doc.containsKey("print")) return;

  JsonObject pr = doc["print"];
  String s = pr["gcode_state"] | "IDLE";
  s.toLowerCase();

  if      (s == "running")  _state = "printing";
  else if (s == "prepare")  _state = "heating";
  else if (s == "pause")    _state = "heating";
  else if (s == "finish")   _state = "idle";
  else if (s == "failed")   _state = "error";
  else if (s == "slicing")  _state = "downloading";
  else                      _state = "idle";

  float hotend = pr["nozzle_temper"] | 0.0f;
  if (_state == "idle" && hotend > 40.0f) _state = "heating";
}

void BambuUsermod::_applyEffect() {
  if (!_enabled) return;
  if (_state == _lastApplied) return;
  _lastApplied = _state;

  int idx = _stateIndex(_state);
  BambuEffect* fx = &_fx[idx];

  Segment& seg = strip.getSegment(0);
  seg.setOption(SEG_OPTION_ON, true);
  seg.mode      = fx->fx;
  seg.speed     = fx->speed;
  seg.intensity = fx->intensity;
  seg.colors[0] = ((uint32_t)fx->col[0] << 16) | ((uint32_t)fx->col[1] << 8) | fx->col[2];
  seg.colors[1] = ((uint32_t)fx->col2[0] << 16) | ((uint32_t)fx->col2[1] << 8) | fx->col2[2];
  colorUpdated(CALL_MODE_DIRECT_CHANGE);
}

int BambuUsermod::_stateIndex(const String& s) {
  for (int i = 0; i < BAMBU_STATE_COUNT; i++)
    if (s == STATE_NAMES[i]) return i;
  return 3;
}

void BambuUsermod::_defaultEffects() {
  _fx[0] = {2,  {255,255,255}, {0,0,0}, 128, 128, 0}; // printing
  _fx[1] = {2,  {255,120,  0}, {0,0,0}, 200, 200, 0}; // heating
  _fx[2] = {2,  {  0, 50,255}, {0,0,0}, 100, 100, 0}; // cooling
  _fx[3] = {0,  {255,200,150}, {0,0,0},   0,   0, 0}; // idle
  _fx[4] = {15, {  0,255,200}, {0,0,0}, 128, 128, 0}; // downloading
  _fx[5] = {2,  {255,  0,  0}, {0,0,0}, 255, 255, 0}; // error
}

void BambuUsermod::addToJsonInfo(JsonObject& root) {
  JsonObject user = root["u"];
  if (user.isNull()) user = root.createNestedObject("u");
  JsonArray arr = user.createNestedArray("Bambu");
  arr.add(_state);
}

void BambuUsermod::addToConfig(JsonObject& root) {
  JsonObject top = root.createNestedObject("Bambu");
  top["ip"]      = _ip;
  top["enabled"] = _enabled;
}

bool BambuUsermod::readFromConfig(JsonObject& root) {
  JsonObject top = root["Bambu"];
  if (top.isNull()) return false;
  _ip      = top["ip"]      | _ip;
  _enabled = top["enabled"] | _enabled;
  return true;
}
