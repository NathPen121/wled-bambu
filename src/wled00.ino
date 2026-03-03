/*
 * WLED-Bambu — Bambu Lab Printer Status LED Controller
 * Drop this folder into your WLED fork and build with PlatformIO.
 *
 * This file replaces (or supplements) wled00.ino in the WLED src/ folder.
 * If WLED's wled00.ino already exists, add the three Bambu calls shown below
 * into the existing setup() and loop() functions instead.
 */

#include "wled.h"
#include "bambu_status.h"
#include "wled_bambu_server.cpp"  // pulls in the web routes

// ── Arduino entry points ──────────────────────────────────────────────────────

void setup() {
  WLED::instance().setup();   // standard WLED init (WiFi, LED strip, web server, etc.)
  setupBambuWebRoutes();         // register /bambu/* endpoints
  loadDefaultBambuEffects();     // load saved config from SPIFFS
}

void loop() {
  WLED::instance().loop();    // standard WLED loop (effects, OTA, etc.)
  pollBambu();                   // check printer state every 2 s
  applyBambuEffects();           // push state → LED effect
}
