#pragma once
#include "wled.h"

// NOTE: No PubSubClient or WiFiClientSecure here.
// ESP8266 with 80KB RAM cannot reliably run TLS MQTT alongside WLED.
// State is set via the web UI manually, or via HTTP GET /bambu/set?state=printing

#define BAMBU_STATE_COUNT 6

extern const char* BAMBU_STATE_NAMES[BAMBU_STATE_COUNT];

struct BambuEffect {
  uint8_t fx;
  uint8_t col[3];
  uint8_t col2[3];
  uint8_t speed;
  uint8_t intensity;
};

class BambuUsermod : public Usermod {
public:
  void setup()   override;
  void loop()    override;
  void addToJsonInfo(JsonObject& root)  override;
  void addToConfig(JsonObject& root)    override;
  bool readFromConfig(JsonObject& root) override;
  uint16_t getId() override { return 0xB4B0; }

  static BambuUsermod* instance;

  String      _state   = "idle";
  bool        _enabled = false;
  BambuEffect _fx[BAMBU_STATE_COUNT];

private:
  bool _routesDone = false;

  void _registerRoutes();
  void _applyEffect();
  int  _stateIndex(const String& s);
  void _defaultEffects();
};
