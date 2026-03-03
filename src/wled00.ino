/*
 * WLED Bambu — Bambu Lab Printer Status LED Controller
 *
 * Integration hooks for WLED. The patch_wled.py script adds these calls
 * into wled_main.cpp automatically during the GitHub Actions build.
 *
 * If integrating manually, add to wled_main.cpp:
 *   In setup(): setupBambuWebRoutes(); loadDefaultBambuEffects();
 *   In loop():  pollBambu(); applyBambuEffects();
 */

#include "wled.h"
#include "bambu_status.h"

// These are only used if this file IS the main sketch entry point.
// In normal WLED builds, patch_wled.py injects the calls into wled_main.cpp instead.

void setup() {
  WLED::instance().setup();
  setupBambuWebRoutes();
  loadDefaultBambuEffects();
}

void loop() {
  WLED::instance().loop();
  pollBambu();
  applyBambuEffects();
}
