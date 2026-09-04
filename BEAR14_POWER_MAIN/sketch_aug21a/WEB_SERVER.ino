/* =========================================================================
 *  WEB_SERVER.ino
 * -------------------------------------------------------------------------
 *  Local WiFi control panel for ground testing:
 *    - toggle each IO-extender rail (heater, VTX, 5V bus 1-3, GPIO1/2 en)
 *    - one button ("Enter Flight State") that permanently disables WiFi,
 *      leaving whatever bus state you've manually set exactly as-is
 *
 *  The ESP32 runs its own access point - no launch-site WiFi/internet
 *  needed. Connect a phone or laptop to AP_SSID, then browse to the
 *  address printed on Serial (normally 192.168.4.1).
 *
 *  >>> CHANGE AP_PASSWORD BEFORE FLIGHT. Anyone in range who knows the
 *      default could otherwise connect and toggle your heater / power
 *      rails from their own phone. <<<
 * =========================================================================
 */

#include <WiFi.h>
#include <WebServer.h>

const char *AP_SSID     = "Payload-Control";
const char *AP_PASSWORD = "changeme123";   // >=8 chars for WPA2 - CHANGE THIS

WebServer server(80);
bool flightMode = false;   // true once WiFi has been shut down for flight

// -------------------------------------------------------------------------
// Page (fully self-contained - no CDN fetches, the AP has no internet)
// -------------------------------------------------------------------------
const char INDEX_HTML[] = R"HTMLPAGE(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Payload Power Control</title>
<style>
  :root {
    --mono: ui-monospace, "SF Mono", "Cascadia Code", "Roboto Mono", Menlo, Consolas, monospace;
    --sans: -apple-system, "Segoe UI", Roboto, sans-serif;
    --bg: #0a0e13;
    --panel: #12181f;
    --line: #2a3441;
    --ink: #dce4ec;
    --dim: #6b7a8c;
    --armed: #e8a33d;
    --hazard: #d4432c;
    --hazard-dim: #241511;
  }
  * { box-sizing: border-box; }
  body {
    margin: 0;
    padding: 28px 16px 56px;
    background: radial-gradient(ellipse at top, #0d1219 0%, var(--bg) 60%);
    color: var(--ink);
    font-family: var(--sans);
    -webkit-font-smoothing: antialiased;
  }
  .nameplate { margin-bottom: 22px; }
  .nameplate-title {
    font-family: var(--mono);
    font-size: 1rem;
    font-weight: 600;
    letter-spacing: .12em;
    margin-bottom: 6px;
  }
  .nameplate-sub {
    font-family: var(--mono);
    font-size: .72rem;
    letter-spacing: .06em;
    color: var(--dim);
  }
  .conn {
    display: inline-block;
    width: 7px; height: 7px;
    border-radius: 50%;
    background: var(--dim);
    margin-right: 7px;
  }
  .conn.live { background: var(--armed); box-shadow: 0 0 6px var(--armed); }

  .panel {
    background: var(--panel);
    border: 1px solid var(--line);
    border-radius: 8px;
    padding: 2px;
    margin-bottom: 22px;
    max-width: 400px;
  }
  .bus-row {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
    padding: 13px 15px;
    border-bottom: 1px solid var(--line);
  }
  .bus-row:last-child { border-bottom: none; }
  .bus-label {
    font-family: var(--mono);
    font-size: .82rem;
    letter-spacing: .04em;
  }
  .bus-desc {
    font-family: var(--sans);
    font-size: .72rem;
    color: var(--dim);
    margin-top: 3px;
    max-width: 220px;
    line-height: 1.35;
  }

  .switch {
    width: 44px; height: 25px;
    border-radius: 4px;
    background: linear-gradient(180deg, #1c242e, #10151b);
    border: 1px solid var(--line);
    position: relative;
    cursor: pointer;
    flex-shrink: 0;
    padding: 0;
  }
  .switch::before {
    content: "";
    position: absolute;
    top: 2px; bottom: 2px; left: 2px;
    width: 19px;
    border-radius: 3px;
    background: linear-gradient(180deg, #3a4451, #232b34);
    box-shadow: 0 1px 2px rgba(0,0,0,.4);
    transition: transform .12s ease, background .12s ease;
  }
  .switch.on { border-color: var(--armed); }
  .switch.on::before {
    transform: translateX(19px);
    background: linear-gradient(180deg, var(--armed), #c9861f);
  }

  .danger-zone {
    max-width: 400px;
    border: 1px solid var(--hazard);
    background: var(--hazard-dim);
    border-radius: 8px;
    padding: 18px;
  }
  .dz-label {
    font-family: var(--mono);
    font-size: .72rem;
    letter-spacing: .14em;
    color: var(--hazard);
    margin-bottom: 12px;
  }
  .cover-assembly {
    position: relative;
    height: 52px;
    margin-bottom: 14px;
    perspective: 300px;
  }
  .cover, .flight-switch {
    position: absolute;
    inset: 0;
    width: 100%;
    height: 100%;
    border-radius: 4px;
    border: none;
    font-family: var(--mono);
    font-size: .78rem;
    font-weight: 700;
    letter-spacing: .08em;
    cursor: pointer;
  }
  .flight-switch {
    background: #1a0f0c;
    color: var(--hazard);
    border: 1px dashed var(--hazard);
  }
  .flight-switch:disabled {
    color: var(--dim);
    border-color: var(--line);
    cursor: default;
  }
  .cover {
    background: repeating-linear-gradient(135deg, var(--hazard), var(--hazard) 9px, #7a2418 9px, #7a2418 18px);
    color: #1a0d08;
    transform-origin: top;
    transition: transform .25s ease, opacity .25s ease;
  }
  .cover.lifted {
    transform: rotateX(-100deg);
    opacity: 0;
    pointer-events: none;
  }
  .dz-note {
    font-family: var(--sans);
    font-size: .78rem;
    line-height: 1.45;
    color: var(--dim);
    margin: 0 0 10px;
  }
  #flightStatus {
    font-family: var(--mono);
    font-size: .72rem;
    letter-spacing: .04em;
    color: var(--dim);
    min-height: 1em;
  }
</style>
</head>
<body>

<div class="nameplate">
  <div class="nameplate-title">PAYLOAD POWER CONTROL</div>
  <div class="nameplate-sub"><span class="conn" id="connDot"></span><span id="connText">CONNECTING</span></div>
</div>

<div class="panel" id="buses"></div>

<div class="danger-zone">
  <div class="dz-label">FLIGHT STATE</div>
  <div class="cover-assembly">
    <button class="flight-switch" id="flightBtn" onclick="enterFlightState()" disabled>ENTER FLIGHT STATE</button>
    <button class="cover" id="cover" onclick="liftCover()">LIFT COVER</button>
  </div>
  <p class="dz-note">Disables WiFi permanently, using whatever bus state you've already set below &mdash; nothing here gets reset. No remote way back once you confirm, so set your rails first, on the pad.</p>
  <div id="flightStatus"></div>
</div>

<script>
const BUSES = [
  {key:'heater', label:'HEATER',   desc:'12V Bus 1 \u00b7 auto-controlled by temperature, may revert within ~1s'},
  {key:'vtx',    label:'VTX',      desc:'12V Bus 2'},
  {key:'bus1',   label:'5V BUS 1', desc:''},
  {key:'bus2',   label:'5V BUS 2', desc:''},
  {key:'bus3',   label:'5V BUS 3', desc:''},
  {key:'gpio1',  label:'GPIO1 EN', desc:''},
  {key:'gpio2',  label:'GPIO2 EN', desc:''},
];
let lastStatus = {};

function render(status) {
  lastStatus = status;
  const el = document.getElementById('buses');
  el.innerHTML = '';
  BUSES.forEach(b => {
    const on = !!status[b.key];
    const row = document.createElement('div');
    row.className = 'bus-row';
    row.innerHTML =
      '<div class="bus-info"><div class="bus-label">' + b.label + '</div>' +
      (b.desc ? '<div class="bus-desc">' + b.desc + '</div>' : '') +
      '</div><button class="switch ' + (on ? 'on' : '') + '" data-bus="' + b.key + '"></button>';
    el.appendChild(row);
  });
  el.querySelectorAll('.switch').forEach(btn => {
    btn.addEventListener('click', () => toggleBus(btn.dataset.bus));
  });
  setConn(true);
  if (status.flight) {
    document.getElementById('flightBtn').disabled = true;
    document.getElementById('flightStatus').textContent = 'FLIGHT STATE ACTIVE';
  }
}

function setConn(live) {
  document.getElementById('connDot').className = 'conn' + (live ? ' live' : '');
  document.getElementById('connText').textContent = live ? 'CONNECTED' : 'DISCONNECTED';
}

function refresh() {
  fetch('/status').then(r => r.json()).then(render).catch(() => setConn(false));
}

function toggleBus(bus) {
  fetch('/toggle', {
    method: 'POST',
    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
    body: 'bus=' + encodeURIComponent(bus)
  }).then(r => r.json()).then(render).catch(() => setConn(false));
}

function liftCover() {
  document.getElementById('cover').classList.add('lifted');
  document.getElementById('flightBtn').disabled = false;
}

function enterFlightState() {
  const summary = BUSES.map(b => b.label + ': ' + (lastStatus[b.key] ? 'ON' : 'OFF')).join('\n');
  if (!confirm('Disable WiFi for good, locking in this bus state as-is:\n\n' + summary + '\n\nYou will lose this connection. Continue?')) return;
  document.getElementById('flightBtn').disabled = true;
  document.getElementById('flightStatus').textContent = 'ENTERING FLIGHT STATE \u2014 WIFI DISCONNECTING\u2026';
  fetch('/flight', {method:'POST'}).catch(() => {});
  setTimeout(() => {
    document.getElementById('flightStatus').textContent = 'FLIGHT STATE SENT. CONNECTION SHOULD DROP SHORTLY.';
  }, 600);
}

refresh();
setInterval(refresh, 5000);
</script>
</body>
</html>
)HTMLPAGE";

// -------------------------------------------------------------------------
// Handlers
// -------------------------------------------------------------------------
void sendStatusJSON() {
  String json = "{";
  json += "\"heater\":" + String((busState & (1 << BUS_HEATER)) ? "true" : "false") + ",";
  json += "\"vtx\":"    + String((busState & (1 << BUS_VTX))    ? "true" : "false") + ",";
  json += "\"bus1\":"   + String((busState & (1 << BUS_5V1))    ? "true" : "false") + ",";
  json += "\"bus2\":"   + String((busState & (1 << BUS_5V2))    ? "true" : "false") + ",";
  json += "\"bus3\":"   + String((busState & (1 << BUS_5V3))    ? "true" : "false") + ",";
  json += "\"gpio1\":"  + String((busState & (1 << BUS_GPIO1))  ? "true" : "false") + ",";
  json += "\"gpio2\":"  + String((busState & (1 << BUS_GPIO2))  ? "true" : "false") + ",";
  json += "\"flight\":" + String(flightMode ? "true" : "false");
  json += "}";
  server.send(200, "application/json", json);
}

void handleRoot() {
  server.send(200, "text/html", INDEX_HTML);
}

void handleStatus() {
  sendStatusJSON();
}

void handleToggle() {
  if (!server.hasArg("bus")) {
    server.send(400, "text/plain", "missing bus param");
    return;
  }
  String bus = server.arg("bus");
  int bit = -1;
       if (bus == "heater") bit = BUS_HEATER;
  else if (bus == "vtx")    bit = BUS_VTX;
  else if (bus == "bus1")   bit = BUS_5V1;
  else if (bus == "bus2")   bit = BUS_5V2;
  else if (bus == "bus3")   bit = BUS_5V3;
  else if (bus == "gpio1")  bit = BUS_GPIO1;
  else if (bus == "gpio2")  bit = BUS_GPIO2;

  if (bit < 0) {
    server.send(400, "text/plain", "unknown bus");
    return;
  }

  bool currentlyOn = busState & (1 << bit);
  set_bus(bit, !currentlyOn);
  sendStatusJSON();
}

void handleFlightState() {
  server.send(200, "application/json", "{\"ok\":true}");
  delay(500);   // let the response above actually leave before the radio dies

  // bus state is intentionally left untouched here - whatever you've manually
  // set via the toggles is what flies. (This used to force busState back to
  // nominal_flight()'s 0x38, which could re-power a rail you'd deliberately
  // set otherwise - e.g. kicking an RFIC into a bad state.)
  Serial.println("FLIGHT STATE: disabling WiFi, bus state left exactly as set");

  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  flightMode = true;
}

void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}

// -------------------------------------------------------------------------
// Setup / loop hooks
// -------------------------------------------------------------------------
void setup_webserver() {
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP(AP_SSID, AP_PASSWORD)) {
    Serial.println("WiFi: softAP start failed");
    return;
  }
  Serial.print("WiFi: AP \"");
  Serial.print(AP_SSID);
  Serial.print("\" up, browse to http://");
  Serial.println(WiFi.softAPIP());

  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/toggle", HTTP_POST, handleToggle);
  server.on("/flight", HTTP_POST, handleFlightState);
  server.onNotFound(handleNotFound);
  server.begin();
}

void handle_webserver() {
  if (!flightMode) server.handleClient();
}
