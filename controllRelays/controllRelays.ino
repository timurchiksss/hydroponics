#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>

// Настройки точки доступа
const char* ap_ssid = "2ESP32_5Relay_Control";
const char* ap_password = "12345678";

// Адрес PCF8574
#define PCF8574_ADDRESS 0x20

WebServer server(80);
bool relayState[8] = {false, false, false, false, false, false, false, false};

// Объявляем функции
void handleRoot();
void handleControl();
void handleStatus();
void handleAll();
void writePCF8574(uint8_t data);
void updateRelays();

void setup() {
  Serial.begin(115200);
  
  Wire.begin();
  writePCF8574(0xFF);
  
  WiFi.softAP(ap_ssid, ap_password);
  
  Serial.println("Точка доступа запущена!");
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());
  
  server.on("/", HTTP_GET, handleRoot);
  server.on("/control", HTTP_GET, handleControl);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/all", HTTP_GET, handleAll);
  
  server.begin();
  Serial.println("HTTP сервер запущен");
}

void loop() {
  server.handleClient();
}

void writePCF8574(uint8_t data) {
  Wire.beginTransmission(PCF8574_ADDRESS);
  Wire.write(data);
  Wire.endTransmission();
}

void updateRelays() {
  uint8_t data = 0xFF;
  for (int i = 0; i < 8; i++) {
    if (relayState[i]) {
      data &= ~(1 << i);
    }
  }
  writePCF8574(data);
}

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>controll 5 relays</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
</head>
<body>
  <center>
    <h1>controll 5 relays</h1>
    
    <div id="statusPanel">
      <h3>curr state:</h3>
    </div>
    
    <hr>
    <h2>ON relay:</h2>
    <button onclick="controlRelay(3, 'on')">Relay 1 - on</button><br><br>
    <button onclick="controlRelay(4, 'on')">Relay 2 - on</button><br><br>
    <button onclick="controlRelay(5, 'on')">Relay 3 - on</button><br><br>
    <button onclick="controlRelay(6, 'on')">Relay 4 - on</button><br><br>
    <button onclick="controlRelay(7, 'on')">Relay 5 - on</button>
    
    <hr>
    <h2>OFF relay:</h2>
    <button onclick="controlRelay(3, 'off')">Relay 1 - off</button><br><br>
    <button onclick="controlRelay(4, 'off')">Relay 2 - off</button><br><br>
    <button onclick="controlRelay(5, 'off')">Relay 3 - off</button><br><br>
    <button onclick="controlRelay(6, 'off')">Relay 4 - off</button><br><br>
    <button onclick="controlRelay(7, 'off')">Relay 5 - off</button>
    
    <hr>
    <h2>controll all relays:</h2>
    <button onclick="controlAll('on')">ON all</button>
    <button onclick="controlAll('off')">OFF all</button>
    
    <hr>
    <div>
      <strong>connection:</strong><br>
      Wi-Fi: ESP32_5Relay_Control<br>
      IP: 192.168.4.1<br>
      password: 12345678
    </div>
  </center>

  <script>
    function updateStatusDisplay() {
      fetch('/status')
        .then(response => response.json())
        .then(data => {
          let statusHTML = '<h3>current state:</h3>';
          for (let i = 0; i < 5; i++) {
            const state = data.relays[i];
            statusHTML += 'Relay ' + (i + 1) + ': ' + (state ? 'on' : 'off') + '<br>';
          }
          document.getElementById('statusPanel').innerHTML = statusHTML;
        });
    }

    function controlRelay(relayNum, action) {
      fetch('/control?relay=' + relayNum + '&action=' + action)
        .then(() => updateStatusDisplay());
    }

    function controlAll(action) {
      fetch('/all?action=' + action)
        .then(() => updateStatusDisplay());
    }

    setInterval(updateStatusDisplay, 2000);
    updateStatusDisplay();
  </script>
</body>
</html>
)rawliteral";
  
  server.send(200, "text/html", html);
}

void handleControl() {
  if (server.hasArg("relay") && server.hasArg("action")) {
    int relayNum = server.arg("relay").toInt();
    String action = server.arg("action");
    
    if (relayNum >= 0 && relayNum < 8) {
      relayState[relayNum] = (action == "on");
      updateRelays();
      server.send(200, "text/plain", "OK");
    }
  }
}

void handleAll() {
  if (server.hasArg("action")) {
    String action = server.arg("action");
    bool state = (action == "on");
    for (int i = 0; i < 8; i++) {
      relayState[i] = state;
    }
    updateRelays();
    server.send(200, "text/plain", "OK");
  }
}

void handleStatus() {
  String json = "{\"relays\":[";
  for (int i = 0; i < 8; i++) {
    json += relayState[i] ? "true" : "false";
    if (i < 7) json += ",";
  }
  json += "]}";
  server.send(200, "application/json", json);
}