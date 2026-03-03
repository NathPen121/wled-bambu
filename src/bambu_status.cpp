#include "wled.h"
#include "bambu_status.h"
#include <ArduinoJson.h>

// ── Globals ───────────────────────────────────────────────────────────────────
String        bambu_ip        = "";
bool          bambu_enabled   = false;
unsigned long bambu_last_poll = 0;
String        bambu_state     = "idle";
BambuEffect   bambu_effects[6];

static const char* STATE_NAMES[] = {
  "printing","heating","cooling","idle","downloading","error"
};

static int stateIndex(const String& s) {
  for (int i = 0; i < 6; i++)
    if (s == STATE_NAMES[i]) return i;
  return 3; // default: idle
}

// ── HTTP poll ─────────────────────────────────────────────────────────────────
void pollBambu() {
  if (!bambu_enabled || bambu_ip.length() < 7) return;
  if (millis() - bambu_last_poll < 2000) return;
  bambu_last_poll = millis();

  WiFiClient client;
  if (!client.connect(bambu_ip.c_str(), 80)) return;

  client.println("GET /server/info HTTP/1.1");
  client.println("Host: " + bambu_ip);
  client.println("Connection: close");
  client.println();

  String payload = "";
  unsigned long timeout = millis();
  while (client.connected() || client.available()) {
    if (millis() - timeout > 3000) break;
    while (client.available()) {
      payload += char(client.read());
      timeout = millis();
    }
    delay(1);
  }
  client.stop();

  int bodyIndex = payload.indexOf("\r\n\r\n");
  if (bodyIndex < 0) return;
  String json = payload.substring(bodyIndex + 4);

  DynamicJsonDocument doc(1024);
  if (deserializeJson(doc, json)) return;
  if (!doc.containsKey("print")) return;

  JsonObject pr = doc["print"];
  String state = pr["gcode_state"] | "IDLE";
  state.toLowerCase();

  if      (state == "running")  bambu_state = "printing";
  else if (state == "prepare")  bambu_state = "heating";
  else if (state == "pause")    bambu_state = "heating";
  else if (state == "finish")   bambu_state = "idle";
  else if (state == "failed")   bambu_state = "error";
  else if (state == "slicing")  bambu_state = "downloading";
  else                          bambu_state = "idle";

  // Treat idle + hot nozzle as heating
  float hotend = pr["nozzle_temper"] | 0.0f;
  if (bambu_state == "idle" && hotend > 40.0f) bambu_state = "heating";
}

// ── Apply current state's effect to WLED segment 0 ───────────────────────────
void applyBambuEffects() {
  if (!bambu_enabled) return;

  int idx = stateIndex(bambu_state);
  BambuEffect* fx = &bambu_effects[idx];

  Segment& seg = strip.getSegment(0);
  seg.setOption(SEG_OPTION_ON, true);
  seg.fx        = fx->fx;
  seg.speed     = fx->speed;
  seg.intensity = fx->intensity;
  seg.colors[0] = ((uint32_t)fx->col[0]  << 16)
                | ((uint32_t)fx->col[1]  <<  8)
                |  (uint32_t)fx->col[2];
  seg.colors[1] = ((uint32_t)fx->col2[0] << 16)
                | ((uint32_t)fx->col2[1] <<  8)
                |  (uint32_t)fx->col2[2];

  strip.trigger();
  stateChanged = true;
}

// ── Load defaults ─────────────────────────────────────────────────────────────
void loadDefaultBambuEffects() {
  // Hard-coded fallbacks
  bambu_effects[0] = {2,  {255,255,255}, {0,0,0}, 128, 128, 0}; // printing  - white breathe
  bambu_effects[1] = {2,  {255,120,  0}, {0,0,0}, 200, 200, 0}; // heating   - orange pulse
  bambu_effects[2] = {2,  {  0, 50,255}, {0,0,0}, 100, 100, 0}; // cooling   - blue breathe
  bambu_effects[3] = {0,  {255,200,150}, {0,0,0},   0,   0, 0}; // idle      - warm white solid
  bambu_effects[4] = {15, {  0,255,200}, {0,0,0}, 128, 128, 0}; // downloading - cyan running
  bambu_effects[5] = {2,  {255,  0,  0}, {0,0,0}, 255, 255, 0}; // error     - red flash

  // Try to load saved config from LittleFS
  if (!WLED_FS.begin()) return;
  File f = WLED_FS.open("/bambu.json", "r");
  if (!f) return;

  DynamicJsonDocument doc(2048);
  if (deserializeJson(doc, f)) { f.close(); return; }
  f.close();

  if (doc.containsKey("ip"))      bambu_ip      = doc["ip"].as<String>();
  if (doc.containsKey("enabled")) bambu_enabled = doc["enabled"].as<bool>();

  if (!doc.containsKey("effects")) return;
  JsonObject effects = doc["effects"];
  for (int i = 0; i < 6; i++) {
    if (!effects.containsKey(STATE_NAMES[i])) continue;
    JsonObject e = effects[STATE_NAMES[i]];
    bambu_effects[i].fx        = e["fx"]        | bambu_effects[i].fx;
    bambu_effects[i].col[0]    = e["col"][0]    | bambu_effects[i].col[0];
    bambu_effects[i].col[1]    = e["col"][1]    | bambu_effects[i].col[1];
    bambu_effects[i].col[2]    = e["col"][2]    | bambu_effects[i].col[2];
    bambu_effects[i].col2[0]   = e["col2"][0]   | bambu_effects[i].col2[0];
    bambu_effects[i].col2[1]   = e["col2"][1]   | bambu_effects[i].col2[1];
    bambu_effects[i].col2[2]   = e["col2"][2]   | bambu_effects[i].col2[2];
    bambu_effects[i].speed     = e["speed"]     | bambu_effects[i].speed;
    bambu_effects[i].intensity = e["intensity"] | bambu_effects[i].intensity;
    bambu_effects[i].duration  = e["duration"]  | bambu_effects[i].duration;
  }
}
