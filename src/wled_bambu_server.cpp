#include "wled.h"
#include "bambu_status.h"
#include <ArduinoJson.h>
#include <SPIFFS.h>

static const char* STATE_NAMES[] = {
  "printing","heating","cooling","idle","downloading","error"
};

void setupBambuWebRoutes() {

  // ── GET /bambu/status ── returns current state + config as JSON ───────────────
  server.on("/bambu/status", HTTP_GET, [](AsyncWebServerRequest* request) {
    DynamicJsonDocument doc(1024);
    doc["state"]      = bambu_state;
    doc["enabled"]    = bambu_enabled;
    doc["printer_ip"] = bambu_ip;

    JsonObject effects = doc.createNestedObject("effects");
    for (int i = 0; i < 6; i++) {
      JsonObject e = effects.createNestedObject(STATE_NAMES[i]);
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

  // ── POST /bambu/config ── save config from browser editor ────────────────────
  server.on("/bambu/config", HTTP_POST, [](AsyncWebServerRequest* request) {
    request->send(400, "text/plain", "Use body");
  },
  nullptr,
  [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t, size_t) {
    DynamicJsonDocument doc(2048);
    if (deserializeJson(doc, data, len)) {
      request->send(400, "text/plain", "Bad JSON");
      return;
    }

    if (doc.containsKey("printer_ip")) bambu_ip      = doc["printer_ip"].as<String>();
    if (doc.containsKey("enabled"))    bambu_enabled  = doc["enabled"].as<bool>();

    if (doc.containsKey("effects")) {
      JsonObject effects = doc["effects"];
      for (int i = 0; i < 6; i++) {
        const char* name = STATE_NAMES[i];
        if (!effects.containsKey(name)) continue;
        JsonObject e = effects[name];
        if (e.containsKey("fx"))        bambu_effects[i].fx          = e["fx"];
        if (e.containsKey("col") && e["col"].is<JsonArray>()) {
          bambu_effects[i].col[0] = e["col"][0];
          bambu_effects[i].col[1] = e["col"][1];
          bambu_effects[i].col[2] = e["col"][2];
        }
        if (e.containsKey("col2") && e["col2"].is<JsonArray>()) {
          bambu_effects[i].col2[0] = e["col2"][0];
          bambu_effects[i].col2[1] = e["col2"][1];
          bambu_effects[i].col2[2] = e["col2"][2];
        }
        if (e.containsKey("speed"))     bambu_effects[i].speed       = e["speed"];
        if (e.containsKey("intensity")) bambu_effects[i].intensity   = e["intensity"];
        if (e.containsKey("duration"))  bambu_effects[i].duration    = e["duration"];
      }
    }

    // Persist to SPIFFS
    if (SPIFFS.begin(true)) {
      File f = SPIFFS.open("/bambu_default.json", "w");
      if (f) {
        DynamicJsonDocument save(2048);
        JsonObject p1s = save.createNestedObject("p1s");
        p1s["enabled"]    = bambu_enabled;
        p1s["printer_ip"] = bambu_ip;
        JsonObject eff = p1s.createNestedObject("effects");
        for (int i = 0; i < 6; i++) {
          JsonObject e = eff.createNestedObject(STATE_NAMES[i]);
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
        serializeJson(save, f);
        f.close();
      }
    }

    request->send(200, "text/plain", "OK");
  });

  // ── Serve the editor UI ────────────────────────────────────────────────────
  server.serveStatic("/bambu", SPIFFS, "/bambu.htm");
}
