#ifndef BAMBU_STATUS_H
#define BAMBU_STATUS_H

#include <Arduino.h>

struct BambuEffect {
  uint8_t       fx;
  uint8_t       col[3];
  uint8_t       col2[3];
  uint8_t       speed;
  uint8_t       intensity;
  unsigned long duration;
};

#define BAMBU_STATE_COUNT 6
extern const char* BAMBU_STATE_NAMES[BAMBU_STATE_COUNT];

extern String        bambu_ip;
extern bool          bambu_enabled;
extern unsigned long bambu_last_poll;
extern String        bambu_state;
extern BambuEffect   bambu_effects[BAMBU_STATE_COUNT];

void pollBambu();
void applyBambuEffects();
void loadDefaultBambuEffects();
void setupBambuWebRoutes();

#endif
