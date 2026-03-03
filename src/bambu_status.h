#ifndef BAMBU_STATUS_H
#define BAMBU_STATUS_H

#include <Arduino.h>
#include <WiFi.h>

// ── Printer connection ────────────────────────────────────────────────────────
extern String bambu_ip;
extern bool   bambu_enabled;
extern unsigned long bambu_last_poll;
extern unsigned long bambu_error_start;
extern String bambu_state;

// ── Effect descriptor ─────────────────────────────────────────────────────────
struct BambuEffect {
  uint8_t  fx;           // WLED effect ID
  uint8_t  col[3];       // primary colour  R,G,B
  uint8_t  col2[3];      // secondary colour R,G,B
  uint8_t  speed;
  uint8_t  intensity;
  unsigned long duration; // ms; 0 = hold forever
};

// 6 slots: printing / heating / cooling / idle / downloading / error
extern BambuEffect bambu_effects[6];

// ── Public API ────────────────────────────────────────────────────────────────
void pollBambu();
void applyBambuEffects();
void loadDefaultBambuEffects();

#endif // BAMBU_STATUS_H
