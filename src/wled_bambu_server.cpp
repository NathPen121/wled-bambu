#include "wled.h"
#include "bambu_status.h"
#include <ArduinoJson.h>

static const char* STATE_NAMES[] = {
  "printing","heating","cooling","idle","downloading","error"
};

// ── Helper: save config to filesystem ────────────────────────────────────────
static void saveBambuConfig() {
  DynamicJsonDocument doc(2048);
  doc["ip"]      = bambu_ip;
  doc["enabled"] = bambu_enabled;

  JsonObject effects = doc.createNestedObject("effects");
  for (int i = 0; i < 6; i++) {
    JsonObject e   = effects.createNestedObject(STATE_NAMES[i]);
    e["fx"]        = bambu_effects[i].fx;
    JsonArray col  = e.createNestedArray("col");
    col.add(bambu_effects[i].col[0]);
    col.add(bambu_effects[i].col[1]);
    col.add(bambu_effects[i].col[2]);
    JsonArray col2 = e.createNestedArray("col2");
    col2.add(bambu_effects[i].col2[0]);
    col2.add(bambu_effects[i].col2[1]);
    col2.add(bambu_effects[i].col2[2]);
    e["speed"]     = bambu_effects[i].speed;
    e["intensity"] = bambu_effects[i].intensity;
    e["duration"]  = bambu_effects[i].duration;
  }

  if (WLED_FS.begin()) {
    File f = WLED_FS.open("/bambu.json", "w");
    if (f) { serializeJson(doc, f); f.close(); }
  }
}

// ── Register web routes ───────────────────────────────────────────────────────
void setupBambuWebRoutes() {

  // GET /bambu/status
  server.on("/bambu/status", HTTP_GET, [](AsyncWebServerRequest* request) {
    DynamicJsonDocument doc(1024);
    doc["state"]   = bambu_state;
    doc["enabled"] = bambu_enabled;
    doc["ip"]      = bambu_ip;

    JsonObject effects = doc.createNestedObject("effects");
    for (int i = 0; i < 6; i++) {
      JsonObject e   = effects.createNestedObject(STATE_NAMES[i]);
      e["fx"]        = bambu_effects[i].fx;
      JsonArray col  = e.createNestedArray("col");
      col.add(bambu_effects[i].col[0]);
      col.add(bambu_effects[i].col[1]);
      col.add(bambu_effects[i].col[2]);
      JsonArray col2 = e.createNestedArray("col2");
      col2.add(bambu_effects[i].col2[0]);
      col2.add(bambu_effects[i].col2[1]);
      col2.add(bambu_effects[i].col2[2]);
      e["speed"]     = bambu_effects[i].speed;
      e["intensity"] = bambu_effects[i].intensity;
      e["duration"]  = bambu_effects[i].duration;
    }

    String out;
    serializeJson(doc, out);
    request->send(200, "application/json", out);
  });

  // POST /bambu/config
  server.on("/bambu/config", HTTP_POST,
    [](AsyncWebServerRequest* request) {
      request->send(400, "text/plain", "Send JSON body");
    },
    nullptr,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t, size_t) {
      DynamicJsonDocument doc(2048);
      if (deserializeJson(doc, data, len)) {
        request->send(400, "text/plain", "Bad JSON");
        return;
      }

      if (doc.containsKey("ip"))      bambu_ip      = doc["ip"].as<String>();
      if (doc.containsKey("enabled")) bambu_enabled = doc["enabled"].as<bool>();

      if (doc.containsKey("effects")) {
        JsonObject effects = doc["effects"];
        for (int i = 0; i < 6; i++) {
          if (!effects.containsKey(STATE_NAMES[i])) continue;
          JsonObject e = effects[STATE_NAMES[i]];
          if (e.containsKey("fx"))        bambu_effects[i].fx        = e["fx"];
          if (e.containsKey("speed"))     bambu_effects[i].speed     = e["speed"];
          if (e.containsKey("intensity")) bambu_effects[i].intensity = e["intensity"];
          if (e.containsKey("duration"))  bambu_effects[i].duration  = e["duration"];
          if (e["col"].is<JsonArray>()) {
            bambu_effects[i].col[0] = e["col"][0];
            bambu_effects[i].col[1] = e["col"][1];
            bambu_effects[i].col[2] = e["col"][2];
          }
          if (e["col2"].is<JsonArray>()) {
            bambu_effects[i].col2[0] = e["col2"][0];
            bambu_effects[i].col2[1] = e["col2"][1];
            bambu_effects[i].col2[2] = e["col2"][2];
          }
        }
      }

      saveBambuConfig();
      request->send(200, "text/plain", "OK");
    }
  );

  // Serve editor UI
  server.serveStatic("/bambu", WLED_FS, "/bambu.htm");
}
