#include "bambu_status.h"
#include "wled.h"
#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>

// ── Globals ───────────────────────────────────────────────────────────────────
String       bambu_ip          = "";
bool         bambu_enabled     = false;
unsigned long bambu_last_poll  = 0;
unsigned long bambu_error_start = 0;
String       bambu_state       = "idle";
BambuEffect    bambu_effects[6];

// State index helpers
// 0=printing 1=heating 2=cooling 3=idle 4=downloading 5=error
static const char* STATE_NAMES[] = {
  "printing","heating","cooling","idle","downloading","error"
};

static int stateIndex(const String& s) {
  for (int i = 0; i < 6; i++)
    if (s == STATE_NAMES[i]) return i;
  return 3; // default idle
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
  }
  client.stop();

  // Strip HTTP headers
  int bodyIndex = payload.indexOf("\r\n\r\n");
  if (bodyIndex < 0) return;
  String json = payload.substring(bodyIndex + 4);

  DynamicJsonDocument doc(1024);
  if (deserializeJson(doc, json)) return;
  if (!doc.containsKey("print")) return;

  JsonObject print = doc["print"];
  String state = print["gcode_state"] | "IDLE";
  state.toLowerCase();

  // Map Bambu states → our slots
  if      (state == "running" || state == "printing") bambu_state = "printing";
  else if (state == "prepare" || state == "heating")  bambu_state = "heating";
  else if (state == "finish"  || state == "finish")   bambu_state = "idle";   // finished → idle glow
  else if (state == "failed"  || state == "error")    bambu_state = "error";
  else if (state == "pause")                          bambu_state = "heating"; // treat pause like heating
  else                                                bambu_state = "idle";

  // Extra: if temperatures are climbing and state is idle → heating
  float hotend = print["nozzle_temper"] | 0.0f;
  if (bambu_state == "idle" && hotend > 40.0f) bambu_state = "heating";
}

// ── Apply current state's effect to WLED ─────────────────────────────────────
void applyBambuEffects() {
  if (!bambu_enabled) return;

  int idx = stateIndex(bambu_state);
  BambuEffect* fx = &bambu_effects[idx];

  // Set WLED segment 0
  Segment& seg = strip.getSegment(0);
  seg.setOption(SEG_OPTION_ON, true);
  seg.fx        = fx->fx;
  seg.speed     = fx->speed;
  seg.intensity = fx->intensity;
  seg.colors[0] = ((uint32_t)fx->col[0]  << 16) |
                  ((uint32_t)fx->col[1]  <<  8) |
                   (uint32_t)fx->col[2];
  seg.colors[1] = ((uint32_t)fx->col2[0] << 16) |
                  ((uint32_t)fx->col2[1] <<  8) |
                   (uint32_t)fx->col2[2];

  strip.trigger();
  stateChanged = true;
}

// ── Load defaults from SPIFFS or hard-code ────────────────────────────────────
void loadDefaultBambuEffects() {
  // Hard-coded fallbacks
  // printing  – white breathe
  bambu_effects[0] = {2,  {255,255,255}, {0,0,0},       128, 128, 0};
  // heating   – orange pulse
  bambu_effects[1] = {2,  {255,120,  0}, {0,0,0},       200, 200, 0};
  // cooling   – blue
  bambu_effects[2] = {2,  {  0, 50,255}, {0,0,0},       100, 100, 0};
  // idle      – warm white solid
  bambu_effects[3] = {0,  {255,200,150}, {0,0,0},         0,   0, 0};
  // downloading – cyan chase
  bambu_effects[4] = {15, {  0,255,200}, {0,0,0},       128, 128, 0};
  // error     – red flash
  bambu_effects[5] = {2,  {255,  0,  0}, {0,0,0},       255, 255, 0};

  // Try to load overrides from SPIFFS
  if (!SPIFFS.begin(true)) return;
  File f = SPIFFS.open("/bambu_default.json", "r");
  if (!f) return;

  DynamicJsonDocument doc(2048);
  if (deserializeJson(doc, f)) { f.close(); return; }
  f.close();

  if (!doc.containsKey("p1s")) return;
  JsonObject p1s = doc["p1s"];
  if (!p1s.containsKey("effects")) return;

  JsonObject effects = p1s["effects"];
  for (int i = 0; i < 6; i++) {
    const char* name = STATE_NAMES[i];
    if (!effects.containsKey(name)) continue;
    JsonObject e = effects[name];
    bambu_effects[i].fx        = e["fx"]        | bambu_effects[i].fx;
    bambu_effects[i].col[0]    = e["col"][0]    | bambu_effects[i].col[0];
    bambu_effects[i].col[1]    = e["col"][1]    | bambu_effects[i].col[1];
    bambu_effects[i].col[2]    = e["col"][2]    | bambu_effects[i].col[2];
    bambu_effects[i].col2[0]   = e["col2"][0]   | bambu_effects[i].col2[0];
    bambu_effects[i].col2[1]   = e["col2"][1]   | bambu_effects[i].col2[1];
    bambu_effects[i].col2[2]   = e["col2"][2]   | bambu_effects[i].col2[2];
    bambu_effects[i].speed      = e["speed"]     | bambu_effects[i].speed;
    bambu_effects[i].intensity  = e["intensity"] | bambu_effects[i].intensity;
    bambu_effects[i].duration   = e["duration"]  | bambu_effects[i].duration;
  }

  if (p1s.containsKey("printer_ip"))
    bambu_ip = p1s["printer_ip"].as<String>();
  if (p1s.containsKey("enabled"))
    bambu_enabled = p1s["enabled"].as<bool>();
}
