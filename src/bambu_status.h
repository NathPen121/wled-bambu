#ifndef BAMBU_STATUS_H
#define BAMBU_STATUS_H

#include <Arduino.h>

// ── Effect descriptor ─────────────────────────────────────────────────────────
struct BambuEffect {
  uint8_t       fx;
  uint8_t       col[3];
  uint8_t       col2[3];
  uint8_t       speed;
  uint8_t       intensity;
  unsigned long duration;
};

// ── Globals ───────────────────────────────────────────────────────────────────
extern String        bambu_ip;
extern bool          bambu_enabled;
extern unsigned long bambu_last_poll;
extern String        bambu_state;
extern BambuEffect   bambu_effects[6];

// ── Public API ────────────────────────────────────────────────────────────────
void pollBambu();
void applyBambuEffects();
void loadDefaultBambuEffects();
void setupBambuWebRoutes();

#endif // BAMBU_STATUS_H
