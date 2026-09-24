#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>

// Wi-Fi Access Point Configuration
const char* AP_SSID = "ALPHA-S_GCS";
const char* AP_PASS = "G9090";  

// Live Tuning Controller Gain Registers
float PRatePitch = 3.50f, IRatePitch = 0.08f, DRatePitch = 0.002f;[cite: 21]
float PAnglePitch = 3.00f, IAnglePitch = 0.10f, DAnglePitch = 0.000f;[cite: 21]

AsyncWebServer gcsServer(80);

// Embedded HTML UI template stored in flash memory
const char GCS_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
<title>ALPHA-S APEX GCS</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
  body { background:#0a0a0a; color:#00ff66; font-family:monospace; padding:20px; }
  h2 { border-bottom: 2px solid #00ff66; padding-bottom: 5px; }
  .card { background:#151515; border:1px solid #00ff66; padding:15px; margin-bottom:15px; border-radius:4px; }
  input[type=number] { width:90px; padding:6px; background:#222; color:#00ff66; border:1px solid #00ff66; border-radius:3px; }
  input[type=submit] { background:#00ff66; color:#000; font-weight:bold; padding:8px 15px; border:none; cursor:pointer; }
</style>
</head><body>
<h2>ALPHA-S TELEMETRY & TUNER</h2>
<div class="card">
  <form action="/get" target="hidden-frame">
    <h3>PITCH AXIS GAINS</h3>
    Rate P: <input type="number" step="any" name="pRP" value="%pRP%">
    Rate I: <input type="number" step="any" name="iRP" value="%iRP%">
    Rate D: <input type="number" step="any" name="dRP" value="%dRP%"><br><br>
    Angle P: <input type="number" step="any" name="pAP" value="%pAP%">
    Angle I: <input type="number" step="any" name="iAP" value="%iAP%">
    Angle D: <input type="number" step="any" name="dAP" value="%dAP%"><br><br>
    <input type="submit" value="INJECT TO FLASH & RAM" onclick="setTimeout(function(){location.reload();}, 500)">
  </form>
</div>
<iframe style="display:none" name="hidden-frame"></iframe>
</body></html>
)rawliteral";

String readFlashString(fs::FS &fs, const char * path) {
  File file = fs.open(path, "r");
  if (!file) return String();
  String content = file.readString();
  file.close();
  return content;
}

void writeFlashString(fs::FS &fs, const char * path, const char * message) {
  File file = fs.open(path, "w");
  if (file) {
    file.print(message);
    file.close();
  }
}

String htmlTemplateProcessor(const String& var) {
  if (var == "pRP") return String(PRatePitch, 4);
  if (var == "iRP") return String(IRatePitch, 4);
  if (var == "dRP") return String(DRatePitch, 4);
  if (var == "pAP") return String(PAnglePitch, 4);
  if (var == "iAP") return String(IAnglePitch, 4);
  if (var == "dAP") return String(DAnglePitch, 4);
  return String();
}

void initGCSWebServer() {
  if (SPIFFS.begin(true)) {
    String val;
    val = readFlashString(SPIFFS, "/pRP.txt"); if (val.length()) PRatePitch = val.toFloat();[cite: 21]
    val = readFlashString(SPIFFS, "/iRP.txt"); if (val.length()) IRatePitch = val.toFloat();[cite: 21]
    val = readFlashString(SPIFFS, "/dRP.txt"); if (val.length()) DRatePitch = val.toFloat();[cite: 21]
    val = readFlashString(SPIFFS, "/pAP.txt"); if (val.length()) PAnglePitch = val.toFloat();[cite: 21]
    val = readFlashString(SPIFFS, "/iAP.txt"); if (val.length()) IAnglePitch = val.toFloat();[cite: 21]
    val = readFlashString(SPIFFS, "/dAP.txt"); if (val.length()) DAnglePitch = val.toFloat();[cite: 21]
  }

  WiFi.softAP(AP_SSID, AP_PASS);[cite: 18, 21]

  gcsServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", GCS_HTML, htmlTemplateProcessor);[cite: 18, 21]
  });

  gcsServer.on("/get", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("pRP")) { String v = request->getParam("pRP")->value(); writeFlashString(SPIFFS, "/pRP.txt", v.c_str()); PRatePitch = v.toFloat(); }[cite: 18, 21]
    if (request->hasParam("iRP")) { String v = request->getParam("iRP")->value(); writeFlashString(SPIFFS, "/iRP.txt", v.c_str()); IRatePitch = v.toFloat(); }[cite: 18, 21]
    if (request->hasParam("dRP")) { String v = request->getParam("dRP")->value(); writeFlashString(SPIFFS, "/dRP.txt", v.c_str()); DRatePitch = v.toFloat(); }[cite: 18, 21]
    if (request->hasParam("pAP")) { String v = request->getParam("pAP")->value(); writeFlashString(SPIFFS, "/pAP.txt", v.c_str()); PAnglePitch = v.toFloat(); }[cite: 18, 21]
    if (request->hasParam("iAP")) { String v = request->getParam("iAP")->value(); writeFlashString(SPIFFS, "/iAP.txt", v.c_str()); IAnglePitch = v.toFloat(); }[cite: 18, 21]
    if (request->hasParam("dAP")) { String v = request->getParam("dAP")->value(); writeFlashString(SPIFFS, "/dAP.txt", v.c_str()); DAnglePitch = v.toFloat(); }[cite: 18, 21]
    request->send(200, "text/plain", "Parameters Injected Successfully.");[cite: 18, 21]
  });

  gcsServer.begin();[cite: 18, 21]
}

void setup() {
  Serial.begin(115200);
  initGCSWebServer();
  Serial.println("[SYSTEM]: Ground Control Station Active. Connect to Wi-Fi AP 'ALPHA-S_GCS'");
}

void loop() {
  // Flight controller operations run uninterrupted in loop
}
