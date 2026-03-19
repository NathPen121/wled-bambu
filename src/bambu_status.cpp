#include "wled.h"
#include "bambu_status.h"

// Embedded UI - avoids needing a separate filesystem flash
static const char BAMBU_HTML[] PROGMEM = R"=====(
<!DOCTYPE html><html lang="en"><head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Bambu LED Config</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:sans-serif;background:#1a1a2e;color:#eee;padding:20px}
h1{color:#00d4ff;margin-bottom:4px}
.sub{color:#888;font-size:13px;margin-bottom:20px}
.card{background:#16213e;border-radius:10px;padding:16px;margin-bottom:16px;border:1px solid #0f3460}
.card h2{font-size:14px;text-transform:uppercase;letter-spacing:1px;color:#00d4ff;margin-bottom:12px}
.row{display:flex;align-items:center;gap:10px;margin-bottom:10px;flex-wrap:wrap}
label{font-size:12px;color:#aaa;min-width:80px}
input[type=text],input[type=number],select{background:#0f3460;border:1px solid #1a6090;color:#eee;border-radius:6px;padding:6px 10px;font-size:13px;flex:1;min-width:80px}
input[type=color]{width:44px;height:34px;border:none;border-radius:6px;cursor:pointer;padding:2px}
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(270px,1fr));gap:16px}
.sc{background:#0f3460;border-radius:8px;padding:14px;border-left:4px solid #00d4ff}
.sc h3{font-size:13px;text-transform:capitalize;margin-bottom:10px;color:#00d4ff}
.printing{border-color:#fff}.heating{border-color:#ff8c00}.cooling{border-color:#0050ff}
.idle{border-color:#ffc864}.downloading{border-color:#00ffc8}.error{border-color:#ff2020}
button{background:#00d4ff;color:#000;border:none;border-radius:8px;padding:10px 24px;font-size:14px;font-weight:bold;cursor:pointer;margin-top:10px}
button:hover{background:#00b8d9}
#status{margin-top:12px;font-size:13px;color:#0f0}
.badge{display:inline-block;padding:2px 10px;border-radius:99px;font-size:11px;font-weight:bold;text-transform:uppercase}
.badge.printing{background:#fff;color:#000}.badge.heating{background:#ff8c00;color:#000}
.badge.cooling{background:#0050ff;color:#fff}.badge.idle{background:#ffc864;color:#000}
.badge.downloading{background:#00ffc8;color:#000}.badge.error{background:#ff2020;color:#fff}
</style></head><body>
<h1>&#x1F5A8; Bambu LED Controller</h1>
<p class="sub">WLED &middot; Bambu Lab Integration</p>
<div class="card"><h2>Printer Connection</h2>
  <div class="row"><label>Printer IP</label><input type="text" id="ip" placeholder="192.168.1.x"></div>
  <div class="row"><label>Access Code</label><input type="text" id="ac" placeholder="from printer screen"></div>
  <div class="row"><label>Serial No.</label><input type="text" id="sn" placeholder="e.g. 01S00C123456789"></div>
  <div class="row"><label>Enabled</label><input type="checkbox" id="en" style="width:20px;height:20px;cursor:pointer"></div>
  <div class="row"><label>State</label><span id="st" class="badge idle">loading&hellip;</span></div>
</div>
<div class="card"><h2>State Effects</h2><div class="grid" id="grid"></div></div>
<button onclick="save()">&#x1F4BE; Save &amp; Apply</button>
<div id="status"></div>
<script>
const STATES=["printing","heating","cooling","idle","downloading","error"];
const FX=[[0,"Solid"],[2,"Breathe"],[1,"Blink"],[12,"Fade"],[15,"Running"],[6,"Sweep"],[9,"Rainbow"],[17,"Twinkle"],[57,"Lightning"],[42,"Fireworks"],[88,"Candle"],[66,"Fire 2012"],[100,"Heartbeat"],[20,"Sparkle"],[48,"Police"],[76,"Meteor"]];
let cfg={};
async function load(){
  try{
    const r=await fetch('/bambu/status');
    cfg=await r.json();
    document.getElementById('ip').value=cfg.ip||'';
    document.getElementById('ac').value=cfg.ac||'';
    document.getElementById('sn').value=cfg.sn||'';
    document.getElementById('en').checked=cfg.enabled||false;
    upd(cfg.state||'idle');
    buildGrid();
  }catch(e){document.getElementById('status').textContent='Could not reach device';}
}
function upd(s){const b=document.getElementById('st');b.textContent=s;b.className='badge '+s;}
function buildGrid(){
  const g=document.getElementById('grid');g.innerHTML='';
  STATES.forEach(s=>{
    const e=(cfg.effects||{})[s]||{};
    const c1=toHex(e.col||[255,255,255]);
    const c2=toHex(e.col2||[0,0,0]);
    const opts=FX.map(([id,n])=>`<option value="${id}"${e.fx==id?' selected':''}>${id}: ${n}</option>`).join('');
    g.innerHTML+=`<div class="sc ${s}"><h3>${s}</h3>
      <div class="row"><label>Effect</label><select id="${s}_fx">${opts}</select></div>
      <div class="row"><label>Color 1</label><input type="color" id="${s}_c1" value="${c1}"></div>
      <div class="row"><label>Color 2</label><input type="color" id="${s}_c2" value="${c2}"></div>
      <div class="row"><label>Speed</label><input type="number" id="${s}_sp" min="0" max="255" value="${e.speed??128}"></div>
      <div class="row"><label>Intensity</label><input type="number" id="${s}_in" min="0" max="255" value="${e.intensity??128}"></div>
    </div>`;
  });
}
function toHex(rgb){return '#'+rgb.map(v=>v.toString(16).padStart(2,'0')).join('');}
function toRgb(h){return[parseInt(h.slice(1,3),16),parseInt(h.slice(3,5),16),parseInt(h.slice(5,7),16)];}
async function save(){
  const fx={};
  STATES.forEach(s=>{
    fx[s]={
      fx:parseInt(document.getElementById(s+'_fx').value),
      col:toRgb(document.getElementById(s+'_c1').value),
      col2:toRgb(document.getElementById(s+'_c2').value),
      speed:parseInt(document.getElementById(s+'_sp').value),
      intensity:parseInt(document.getElementById(s+'_in').value),duration:0
    };
  });
  const p={ip:document.getElementById('ip').value.trim(),
           ac:document.getElementById('ac').value.trim(),
           sn:document.getElementById('sn').value.trim(),
           enabled:document.getElementById('en').checked,effects:fx};
  try{
    await fetch('/bambu/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(p)});
    document.getElementById('status').textContent='Saved!';
  }catch(e){document.getElementById('status').textContent='Save failed: '+e;}
}
setInterval(async()=>{
  try{const r=await fetch('/bambu/status');const d=await r.json();upd(d.state);}catch(_){}
},3000);
load();
</script></body></html>
)=====";

static const char* STATE_NAMES[BAMBU_STATE_COUNT] = {
  "printing","heating","cooling","idle","downloading","error"
};

// Static instance pointer for PubSubClient callback
BambuUsermod* BambuUsermod::instance = nullptr;

void BambuUsermod::mqttCallback(char* topic, byte* payload, unsigned int len) {
  if (instance) instance->_mqttMessage(topic, payload, len);
}

void BambuUsermod::setup() {
  instance = this;
  _defaultEffects();
}

void BambuUsermod::loop() {
  if (!_routesDone) {
    _registerRoutes();
    _routesDone = true;
  }
  _poll();
  _applyEffect();
}

void BambuUsermod::_registerRoutes() {
  // Serve embedded HTML
  server.on("/bambu", HTTP_GET, [](AsyncWebServerRequest* req) {
    AsyncWebServerResponse* resp = req->beginResponse_P(200, "text/html", (const uint8_t*)BAMBU_HTML, strlen_P(BAMBU_HTML));
    req->send(resp);
  });

  server.on("/bambu/status", HTTP_GET, [this](AsyncWebServerRequest* req) {
    String json = "{\"state\":\"" + _state
                + "\",\"ip\":\"" + _ip
                + "\",\"ac\":\"" + _ac
                + "\",\"sn\":\"" + _sn
                + "\",\"enabled\":" + (_enabled ? "true" : "false") + "}";
    req->send(200, "application/json", json);
  });

  // POST /bambu/config - use a proper body handler
  AsyncCallbackWebHandler* handler = new AsyncCallbackWebHandler();
  handler->setUri("/bambu/config");
  handler->setMethod(HTTP_POST);
  handler->onRequest([](AsyncWebServerRequest* req) {
    req->send(200, "text/plain", "OK");
  });
  handler->onBody([](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
    BambuUsermod* self = BambuUsermod::instance;
    if (!self) return;
    DynamicJsonDocument doc(4096);
    if (deserializeJson(doc, data, len)) return;
    if (doc.containsKey("ip"))      self->_ip      = doc["ip"].as<String>();
    if (doc.containsKey("ac"))      self->_ac      = doc["ac"].as<String>();
    if (doc.containsKey("sn"))      self->_sn      = doc["sn"].as<String>();
    if (doc.containsKey("enabled")) self->_enabled = doc["enabled"].as<bool>();
    if (doc.containsKey("effects")) {
      JsonObject effects = doc["effects"];
      for (int i = 0; i < BAMBU_STATE_COUNT; i++) {
        if (!effects.containsKey(STATE_NAMES[i])) continue;
        JsonObject e = effects[STATE_NAMES[i]];
        self->_fx[i].fx        = e["fx"]        | self->_fx[i].fx;
        self->_fx[i].col[0]    = e["col"][0]    | self->_fx[i].col[0];
        self->_fx[i].col[1]    = e["col"][1]    | self->_fx[i].col[1];
        self->_fx[i].col[2]    = e["col"][2]    | self->_fx[i].col[2];
        self->_fx[i].col2[0]   = e["col2"][0]   | self->_fx[i].col2[0];
        self->_fx[i].col2[1]   = e["col2"][1]   | self->_fx[i].col2[1];
        self->_fx[i].col2[2]   = e["col2"][2]   | self->_fx[i].col2[2];
        self->_fx[i].speed     = e["speed"]     | self->_fx[i].speed;
        self->_fx[i].intensity = e["intensity"] | self->_fx[i].intensity;
      }
    }
    serializeConfig();
    self->_lastPoll = 0;
  });
  server.addHandler(handler);
}

// ---- MQTT ----
void BambuUsermod::_mqttConnect() {
  if (_ip.length() < 7 || _ac.isEmpty() || _sn.isEmpty()) return;
  if (_mqttClient.connected()) _mqttClient.disconnect();

  // Bambu uses TLS with a self-signed cert - skip verification
  _wifiClient.setInsecure();
  _wifiClient.setTimeout(5000);  // 5 seconds

  _mqttClient.setServer(_ip.c_str(), 8883);
  _mqttClient.setBufferSize(4096);  // Bambu payloads are large
  _mqttClient.setCallback(BambuUsermod::mqttCallback);
  _mqttClient.setKeepAlive(60);
  _mqttClient.setSocketTimeout(5);

  String clientId = "wled_" + _sn;
  _mqttClient.connect(clientId.c_str(), "bblp", _ac.c_str());
  if (_mqttClient.connected()) {
    String topic = "device/" + _sn + "/report";
    _mqttClient.subscribe(topic.c_str());
  }
}

void BambuUsermod::_mqttMessage(char* topic, byte* payload, unsigned int len) {
  DynamicJsonDocument doc(4096);
  if (deserializeJson(doc, payload, len)) return;
  if (!doc.containsKey("print")) return;

  JsonObject pr = doc["print"];
  if (!pr.containsKey("gcode_state")) return;

  String s = pr["gcode_state"] | "IDLE";
  s.toLowerCase();

  if      (s == "running")  _state = "printing";
  else if (s == "prepare")  _state = "heating";
  else if (s == "pause")    _state = "heating";
  else if (s == "finish")   _state = "idle";
  else if (s == "failed")   _state = "error";
  else if (s == "slicing")  _state = "downloading";
  else                      _state = "idle";

  float hotend = pr["nozzle_temper"] | 0.0f;
  if (_state == "idle" && hotend > 40.0f) _state = "heating";
}

void BambuUsermod::_poll() {
  if (!_enabled) return;
  if (WiFi.status() != WL_CONNECTED) return;

  // Reconnect MQTT if dropped
  if (!_mqttClient.connected()) {
    if (millis() - _lastPoll > 10000) {
      _lastPoll = millis();
      _mqttConnect();
    }
  }
  _mqttClient.loop();
}

void BambuUsermod::_applyEffect() {
  if (!_enabled) return;
  if (_state == _lastApplied) return;
  _lastApplied = _state;

  int idx = _stateIndex(_state);
  BambuEffect* fx = &_fx[idx];

  Segment& seg = strip.getSegment(0);
  seg.setOption(SEG_OPTION_ON, true);
  seg.mode      = fx->fx;
  seg.speed     = fx->speed;
  seg.intensity = fx->intensity;
  seg.colors[0] = ((uint32_t)fx->col[0] << 16) | ((uint32_t)fx->col[1] << 8) | fx->col[2];
  seg.colors[1] = ((uint32_t)fx->col2[0] << 16) | ((uint32_t)fx->col2[1] << 8) | fx->col2[2];
  colorUpdated(CALL_MODE_DIRECT_CHANGE);
}

int BambuUsermod::_stateIndex(const String& s) {
  for (int i = 0; i < BAMBU_STATE_COUNT; i++)
    if (s == STATE_NAMES[i]) return i;
  return 3;
}

void BambuUsermod::_defaultEffects() {
  _fx[0] = {2,  {255,255,255}, {0,0,0}, 128, 128, 0};
  _fx[1] = {2,  {255,120,  0}, {0,0,0}, 200, 200, 0};
  _fx[2] = {2,  {  0, 50,255}, {0,0,0}, 100, 100, 0};
  _fx[3] = {0,  {255,200,150}, {0,0,0},   0,   0, 0};
  _fx[4] = {15, {  0,255,200}, {0,0,0}, 128, 128, 0};
  _fx[5] = {2,  {255,  0,  0}, {0,0,0}, 255, 255, 0};
}

void BambuUsermod::addToJsonInfo(JsonObject& root) {
  JsonObject user = root["u"];
  if (user.isNull()) user = root.createNestedObject("u");
  JsonArray arr = user.createNestedArray("Bambu");
  arr.add(_state);
}

void BambuUsermod::addToConfig(JsonObject& root) {
  JsonObject top = root.createNestedObject("Bambu");
  top["ip"]      = _ip;
  top["ac"]      = _ac;
  top["sn"]      = _sn;
  top["enabled"] = _enabled;
}

bool BambuUsermod::readFromConfig(JsonObject& root) {
  JsonObject top = root["Bambu"];
  if (top.isNull()) return false;
  _ip      = top["ip"]      | _ip;
  _ac      = top["ac"]      | _ac;
  _sn      = top["sn"]      | _sn;
  _enabled = top["enabled"] | _enabled;
  // Reconnect with loaded credentials once WiFi is up
  _lastPoll = 0;
  return true;
}
