#pragma once
#include "wled.h"
#include <PubSubClient.h>

#define BAMBU_STATE_COUNT 6

struct BambuEffect {
  uint8_t  fx;
  uint8_t  col[3];
  uint8_t  col2[3];
  uint8_t  speed;
  uint8_t  intensity;
  uint8_t  duration;
};

class BambuUsermod : public Usermod {
public:
  void setup() override;
  void loop() override;
  void addToJsonInfo(JsonObject& root) override;
  void addToConfig(JsonObject& root) override;
  bool readFromConfig(JsonObject& root) override;
  uint16_t getId() override { return USERMOD_ID_RESERVED; }

private:
  String        _ip        = "";
  String        _ac        = "";  // access code
  String        _sn        = "";  // serial number
  bool          _enabled   = false;
  unsigned long _lastPoll  = 0;
  String        _state     = "idle";
  String        _lastApplied = "";
  bool          _routesDone = false;
  BambuEffect   _fx[BAMBU_STATE_COUNT];

  WiFiClient    _wifiClient;
  PubSubClient  _mqttClient{_wifiClient};

  void _registerRoutes();
  void _mqttConnect();
  void _mqttMessage(char* topic, byte* payload, unsigned int len);
  void _poll();
  void _applyEffect();
  int  _stateIndex(const String& s);
  void _defaultEffects();
};
