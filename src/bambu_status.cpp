#include "wled.h"
#include "bambu_status.h"

const char* BAMBU_STATE_NAMES[BAMBU_STATE_COUNT] = {
  "printing","heating","cooling","idle","downloading","error"
};

BambuUsermod* BambuUsermod::instance = nullptr;

// ---------------------------------------------------------------------------
// Embedded HTML UI
// ---------------------------------------------------------------------------
static const char BAMBU_HTML[] PROGMEM = R"=====(<!DOCTYPE html>
<html lang="en"><head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Bambu LED</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:sans-serif;background:#1a1a2e;color:#eee;padding:16px}
h1{color:#00d4ff;margin-bottom:16px;font-size:18px}
.card{background:#16213e;border-radius:8px;padding:14px;margin-bottom:14px;border:1px solid #0f3460}
.card h2{font-size:12px;text-transform:uppercase;letter-spacing:1px;color:#00d4ff;margin-bottom:10px}
.row{display:flex;align-items:center;gap:8px;margin-bottom:8px}
label{font-size:12px;color:#aaa;min-width:90px}
input[type=text],select{background:#0f3460;border:1px solid #1a6090;color:#eee;border-radius:4px;padding:5px 8px;font-size:13px;flex:1}
input[type=number]{background:#0f3460;border:1px solid #1a6090;color:#eee;border-radius:4px;padding:5px 8px;font-size:13px;width:70px}
input[type=color]{width:40px;height:30px;border:none;border-radius:4px;cursor:pointer;padding:1px}
input[type=checkbox]{width:18px;height:18px;cursor:pointer}
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(260px,1fr));gap:12px}
.sc{background:#0f3460;border-radius:6px;padding:12px;border-left:3px solid #00d4ff}
.sc h3{font-size:12px;text-transform:uppercase;margin-bottom:8px;color:#00d4ff}
.printing{border-color:#fff}.heating{border-color:#ff8c00}.cooling{border-color:#0050ff}
.idle{border-color:#ffc864}.downloading{border-color:#00ffc8}.error{border-color:#ff2020}
.btn{background:#00d4ff;color:#000;border:none;border-radius:6px;padding:8px 20px;font-size:13px;font-weight:bold;cursor:pointer;margin-top:8px;margin-right:8px}
.btn:hover{background:#00b8d9}
.btn.sec{background:#444;color:#eee}
#msg{margin-top:10px;font-size:12px;color:#0f0}
.badge{display:inline-block;padding:2px 8px;border-radius:99px;font-size:11px;font-weight:bold;text-transform:uppercase}
.badge.printing{background:#fff;color:#000}.badge.heating{background:#ff8c00;color:#000}
.badge.cooling{background:#0050ff;color:#fff}.badge.idle{background:#ffc864;color:#000}
.badge.downloading{background:#00ffc8;color:#000}.badge.error{background:#ff2020;color:#fff}
</style></head><body>
<h1>&#x1F5A8; Bambu LED</h1>

<div class="card">
  <h2>Status</h2>
  <div class="row"><label>Current State</label><span id="st" class="badge idle">loading</span></div>
  <div class="row"><label>Enabled</label><input type="checkbox" id="en"></div>
  <h2 style="margin-top:10px">Manual Override</h2>
  <div class="row">
    <select id="manstate">
      <option value="printing">Printing</option>
      <option value="heating">Heating</option>
      <option value="cooling">Cooling</option>
      <option value="idle" selected>Idle</option>
      <option value="downloading">Downloading</option>
      <option value="error">Error</option>
    </select>
    <button class="btn sec" onclick="setState()">Set</button>
  </div>
</div>

<div class="card">
  <h2>Effects per State</h2>
  <div class="grid" id="grid"></div>
</div>

<button class="btn" onclick="save()">&#x1F4BE; Save</button>
<div id="msg"></div>

<script>
const STATES=["printing","heating","cooling","idle","downloading","error"];
const FX=[[0,"Solid"],[2,"Breathe"],[1,"Blink"],[12,"Fade"],[15,"Running"],
          [6,"Sweep"],[9,"Rainbow"],[17,"Twinkle"],[57,"Lightning"],
          [42,"Fireworks"],[88,"Candle"],[66,"Fire 2012"],[100,"Heartbeat"],
          [20,"Sparkle"],[48,"Police"],[76,"Meteor"]];
let cfg={fx:{}};

function msg(t,ok){const e=document.getElementById('msg');e.textContent=t;e.style.color=ok?'#0f0':'#f44';}
function upd(s){const b=document.getElementById('st');b.textContent=s;b.className='badge '+s;}

function hex(rgb){return '#'+(rgb||[0,0,0]).map(v=>(v||0).toString(16).padStart(2,'0')).join('');}
function rgb(h){return[parseInt(h.slice(1,3),16),parseInt(h.slice(3,5),16),parseInt(h.slice(5,7),16)];}

function buildGrid(){
  const g=document.getElementById('grid');g.innerHTML='';
  STATES.forEach(s=>{
    const e=(cfg.fx||{})[s]||{};
    const opts=FX.map(([id,n])=>`<option value="${id}"${(e.fx||0)==id?' selected':''}>${id}: ${n}</option>`).join('');
    g.innerHTML+=`<div class="sc ${s}"><h3>${s}</h3>
      <div class="row"><label>Effect</label><select id="${s}_fx">${opts}</select></div>
      <div class="row"><label>Color 1</label><input type="color" id="${s}_c1" value="${hex(e.col)}"></div>
      <div class="row"><label>Color 2</label><input type="color" id="${s}_c2" value="${hex(e.col2)}"></div>
      <div class="row"><label>Speed</label><input type="number" id="${s}_sp" min="0" max="255" value="${e.speed||128}"></div>
      <div class="row"><label>Intensity</label><input type="number" id="${s}_in" min="0" max="255" value="${e.intensity||128}"></div>
    </div>`;
  });
}

async function load(){
  try{
    const r=await fetch('/bambu/status');
    const d=await r.json();
    document.getElementById('en').checked=d.enabled||false;
    cfg.fx=d.fx||{};
    upd(d.state||'idle');
    buildGrid();
  }catch(e){msg('Could not load - is device reachable?',false);}
}

async function save(){
  const fx={};
  STATES.forEach(s=>{
    fx[s]={
      fx:parseInt(document.getElementById(s+'_fx').value),
      col:rgb(document.getElementById(s+'_c1').value),
      col2:rgb(document.getElementById(s+'_c2').value),
      speed:parseInt(document.getElementById(s+'_sp').value),
      intensity:parseInt(document.getElementById(s+'_in').value)
    };
  });
  const body=JSON.stringify({enabled:document.getElementById('en').checked,fx:fx});
  try{
    const r=await fetch('/bambu/config',{method:'POST',headers:{'Content-Type':'application/json'},body:body});
    const t=await r.text();
    msg(t==='OK'?'Saved!':'Error: '+t, t==='OK');
  }catch(e){msg('Save failed: '+e,false);}
}

async function setState(){
  const s=document.getElementById('manstate').value;
  try{
    await fetch('/bambu/state?v='+s);
    upd(s);
    msg('State set to '+s,true);
  }catch(e){msg('Failed: '+e,false);}
}

setInterval(async()=>{
  try{const r=await fetch('/bambu/status');const d=await r.json();upd(d.state);}catch(_){}
},5000);

load();
</script>
</body></html>
)=====";

// ---------------------------------------------------------------------------

void BambuUsermod::setup() {
  instance = this;
  _defaultEffects();
}

void BambuUsermod::loop() {
  if (!_routesDone) {
    _registerRoutes();
    _routesDone = true;
  }
}

void BambuUsermod::_registerRoutes() {
  // GET /bambu - serve embedded UI
  server.on("/bambu", HTTP_GET, [](AsyncWebServerRequest* req) {
    AsyncWebServerResponse* r = req->beginResponse_P(200, "text/html",
      (const uint8_t*)BAMBU_HTML, strlen_P(BAMBU_HTML));
    req->send(r);
  });

  // GET /bambu/status - return current state as JSON
  server.on("/bambu/status", HTTP_GET, [](AsyncWebServerRequest* req) {
    BambuUsermod* self = BambuUsermod::instance;
    if (!self) { req->send(500); return; }

    // Build fx JSON
    String fx = "{";
    for (int i = 0; i < BAMBU_STATE_COUNT; i++) {
      if (i > 0) fx += ",";
      fx += "\""; fx += BAMBU_STATE_NAMES[i]; fx += "\":{";
      fx += "\"fx\":";   fx += self->_fx[i].fx;
      fx += ",\"col\":["; fx += self->_fx[i].col[0]; fx += ","; fx += self->_fx[i].col[1]; fx += ","; fx += self->_fx[i].col[2]; fx += "]";
      fx += ",\"col2\":["; fx += self->_fx[i].col2[0]; fx += ","; fx += self->_fx[i].col2[1]; fx += ","; fx += self->_fx[i].col2[2]; fx += "]";
      fx += ",\"speed\":"; fx += self->_fx[i].speed;
      fx += ",\"intensity\":"; fx += self->_fx[i].intensity;
      fx += "}";
    }
    fx += "}";

    String json = "{\"state\":\"" + self->_state + "\","
                + "\"enabled\":" + (self->_enabled ? "true" : "false") + ","
                + "\"fx\":" + fx + "}";
    req->send(200, "application/json", json);
  });

  // GET /bambu/state?v=printing - manually set state
  server.on("/bambu/state", HTTP_GET, [](AsyncWebServerRequest* req) {
    BambuUsermod* self = BambuUsermod::instance;
    if (!self) { req->send(500); return; }
    if (req->hasParam("v")) {
      self->_state = req->getParam("v")->value();
      self->_applyEffect();
    }
    req->send(200, "text/plain", "OK");
  });

  // POST /bambu/config - save config
  // Use server.on with body handler - simplest reliable approach
  server.on("/bambu/config", HTTP_POST,
    // onRequest - called when headers arrive, before body
    [](AsyncWebServerRequest* req) {
      // Don't send response here - wait for body
    },
    // onUpload - not needed
    nullptr,
    // onBody - called with body data
    [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
      BambuUsermod* self = BambuUsermod::instance;
      if (!self) { req->send(500); return; }

      // Only process when we have the complete body
      if (index + len != total) return;

      DynamicJsonDocument doc(2048);
      if (deserializeJson(doc, data, len)) {
        req->send(400, "text/plain", "Bad JSON");
        return;
      }

      if (doc.containsKey("enabled")) self->_enabled = doc["enabled"].as<bool>();

      if (doc.containsKey("fx")) {
        JsonObject fxObj = doc["fx"];
        for (int i = 0; i < BAMBU_STATE_COUNT; i++) {
          if (!fxObj.containsKey(BAMBU_STATE_NAMES[i])) continue;
          JsonObject e = fxObj[BAMBU_STATE_NAMES[i]];
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
      // Force effect update
      self->_state = self->_state; // touch to trigger reapply
      self->_applyEffect();
      req->send(200, "text/plain", "OK");
    }
  );
}

void BambuUsermod::_applyEffect() {
  int idx = _stateIndex(_state);
  BambuEffect* fx = &_fx[idx];

  // Use WLED's JSON API to set effect - most compatible approach
  // This is how usermods are supposed to change effects in WLED 0.15
  DynamicJsonDocument doc(256);
  JsonObject seg0 = doc.createNestedObject("seg");
  seg0["fx"]  = fx->fx;
  seg0["sx"]  = fx->speed;     // speed
  seg0["ix"]  = fx->intensity; // intensity
  seg0["on"]  = true;
  // Colors as 24-bit packed values
  JsonArray col = seg0.createNestedArray("col");
  JsonArray c1 = col.createNestedArray();
  c1.add(fx->col[0]); c1.add(fx->col[1]); c1.add(fx->col[2]);
  JsonArray c2 = col.createNestedArray();
  c2.add(fx->col2[0]); c2.add(fx->col2[1]); c2.add(fx->col2[2]);

  deserializeState(doc.as<JsonObject>());
}

int BambuUsermod::_stateIndex(const String& s) {
  for (int i = 0; i < BAMBU_STATE_COUNT; i++)
    if (s == BAMBU_STATE_NAMES[i]) return i;
  return 3; // default: idle
}

void BambuUsermod::_defaultEffects() {
  _fx[0] = {2,  {255,255,255}, {0,0,0}, 128, 128};
  _fx[1] = {2,  {255,120,  0}, {0,0,0}, 200, 200};
  _fx[2] = {2,  {  0, 50,255}, {0,0,0}, 100, 100};
  _fx[3] = {0,  {255,200,150}, {0,0,0},   0,   0};
  _fx[4] = {15, {  0,255,200}, {0,0,0}, 128, 128};
  _fx[5] = {2,  {255,  0,  0}, {0,0,0}, 255, 255};
}

void BambuUsermod::addToJsonInfo(JsonObject& root) {
  JsonObject user = root["u"];
  if (user.isNull()) user = root.createNestedObject("u");
  JsonArray info = user.createNestedArray("Bambu");
  info.add(_state);
}

void BambuUsermod::addToConfig(JsonObject& root) {
  JsonObject top = root.createNestedObject("Bambu");
  top["enabled"] = _enabled;
  top["state"]   = _state;
  JsonObject fx = top.createNestedObject("fx");
  for (int i = 0; i < BAMBU_STATE_COUNT; i++) {
    JsonObject e = fx.createNestedObject(BAMBU_STATE_NAMES[i]);
    e["fx"]        = _fx[i].fx;
    JsonArray col  = e.createNestedArray("col");
    col.add(_fx[i].col[0]); col.add(_fx[i].col[1]); col.add(_fx[i].col[2]);
    JsonArray col2 = e.createNestedArray("col2");
    col2.add(_fx[i].col2[0]); col2.add(_fx[i].col2[1]); col2.add(_fx[i].col2[2]);
    e["speed"]     = _fx[i].speed;
    e["intensity"] = _fx[i].intensity;
  }
}

bool BambuUsermod::readFromConfig(JsonObject& root) {
  JsonObject top = root["Bambu"];
  if (top.isNull()) return false;
  _enabled = top["enabled"] | _enabled;
  _state   = top["state"]   | _state;
  if (top.containsKey("fx")) {
    JsonObject fxObj = top["fx"];
    for (int i = 0; i < BAMBU_STATE_COUNT; i++) {
      if (!fxObj.containsKey(BAMBU_STATE_NAMES[i])) continue;
      JsonObject e = fxObj[BAMBU_STATE_NAMES[i]];
      _fx[i].fx        = e["fx"]        | _fx[i].fx;
      _fx[i].col[0]    = e["col"][0]    | _fx[i].col[0];
      _fx[i].col[1]    = e["col"][1]    | _fx[i].col[1];
      _fx[i].col[2]    = e["col"][2]    | _fx[i].col[2];
      _fx[i].col2[0]   = e["col2"][0]   | _fx[i].col2[0];
      _fx[i].col2[1]   = e["col2"][1]   | _fx[i].col2[1];
      _fx[i].col2[2]   = e["col2"][2]   | _fx[i].col2[2];
      _fx[i].speed     = e["speed"]     | _fx[i].speed;
      _fx[i].intensity = e["intensity"] | _fx[i].intensity;
    }
  }
  return true;
}
