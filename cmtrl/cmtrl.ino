#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>

// Настройки точки доступа
const char* ap_ssid = "ESP32_15Relay_Control";
const char* ap_password = "12345678";

// Адреса трёх PCF8574
#define PCF1 0x20
#define PCF2 0x22
#define PCF3 0x24

WebServer server(80);

// 15 реле, все выключены
bool relayState[24] = {false};

// ======================== ФУНКЦИИ ========================
void handleRoot();
void handleControl();
void handleStatus();
void handleAll();
void updateAllPCF();

// ========================================================
void setup() {
  Serial.begin(115200);
  Wire.begin();

  // Инициализация всех PCF: выключаем всё
  for (int addr = PCF1; addr <= PCF3; addr++) {
    Wire.beginTransmission(addr);
    Wire.write(0xFF);
    Wire.endTransmission();
  }

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

// ======================== ВСПОМОГАТЕЛЬНЫЕ ========================

void writePCF8574(uint8_t address, uint8_t data) {
  Wire.beginTransmission(address);
  Wire.write(data);
  Wire.endTransmission();
}

void updateAllPCF() {
  // Каждый PCF управляет 8 линиями, но мы используем только 5 на каждом
  uint8_t data1 = 0xFF;
  uint8_t data2 = 0xFF;
  uint8_t data3 = 0xFF;

  // PCF1 → реле 0–4
  for (int i = 1; i < 9; i++)
    if (relayState[i]) data1 &= ~(1 << i);

  // PCF2 → реле 5–9
  for (int i = 9; i < 17; i++)
    if (relayState[i]) data2 &= ~(1 << (i - 8));

  // PCF3 → реле 10–14
  for (int i = 17; i < 25; i++)
    if (relayState[i]) data3 &= ~(1 << (i - 16));

  writePCF8574(PCF1, data1);
  writePCF8574(PCF2, data2);
  writePCF8574(PCF3, data3);
}

// ======================== ОБРАБОТЧИКИ ========================

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>ESP32 15 Relay Control</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
</head>
<body>
  <center>
    <h1>Control 15 Relays</h1>
    <div id="statusPanel"></div>
    <hr>

    <h2>Turn ON Relays:</h2>
)rawliteral";

  // Кнопки ON
  for (int i = 1; i <= 5; i++) {
    html += "<button onclick=\"controlRelay(" + String(i + 3) + ", 'on')\">Relay " + String(i) + " - ON</button><br><br>";
  }
  for (int i = 9; i <= 13; i++) {
    html += "<button onclick=\"controlRelay(" + String(i + 3) + ", 'on')\">Relay " + String(i) + " - ON</button><br><br>";
  }
  for (int i = 17; i <= 21; i++) {
    html += "<button onclick=\"controlRelay(" + String(i + 3) + ", 'on')\">Relay " + String(i) + " - ON</button><br><br>";
  }

  html += "<hr><h2>Turn OFF Relays:</h2>";

  // Кнопки OFF
  for (int i = 1; i <= 5; i++) {
    html += "<button onclick=\"controlRelay(" + String(i + 3) + ", 'off')\">Relay " + String(i) + " - OFF</button><br><br>";
  }
  for (int i = 9; i <= 13; i++) {
    html += "<button onclick=\"controlRelay(" + String(i + 3) + ", 'off')\">Relay " + String(i) + " - OFF</button><br><br>";
  }
  for (int i = 17; i <= 21; i++) {
    html += "<button onclick=\"controlRelay(" + String(i + 3) + ", 'off')\">Relay " + String(i) + " - OFF</button><br><br>";
  }

  html += R"rawliteral(
    <hr>
    <h2>Control All:</h2>
    <button onclick="controlAll('on')">Turn ON All</button>
    <button onclick="controlAll('off')">Turn OFF All</button>
    <hr>
    <div>
      <strong>Connection Info:</strong><br>
      Wi-Fi: ESP32_15Relay_Control<br>
      IP: 192.168.4.1<br>
      Password: 12345678
    </div>
  </center>

  <script>
    function updateStatusDisplay() {
      fetch('/status')
        .then(response => response.json())
        .then(data => {
          let html = '<h3>Current Relay States:</h3>';
          for (let i = 0; i < 24; i++) {
            html += 'Relay ' + (i + 1) + ': ' + (data.relays[i] ? 'ON' : 'OFF') + '<br>';
          }
          document.getElementById('statusPanel').innerHTML = html;
        });
    }

    function controlRelay(num, action) {
      fetch('/control?relay=' + num + '&action=' + action)
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
    int relayNum = server.arg("relay").toInt() - 1;  // нумерация с 1
    String action = server.arg("action");

    if (relayNum >= 0 && relayNum < 24) {
      relayState[relayNum] = (action == "on");
      updateAllPCF();
      server.send(200, "text/plain", "OK");
    }
  }
}

void handleAll() {
  if (server.hasArg("action")) {
    bool state = (server.arg("action") == "on");
    for (int i = 0; i < 24; i++)
      relayState[i] = state;

    updateAllPCF();
    server.send(200, "text/plain", "OK");
  }
}

void handleStatus() {
  String json = "{\"relays\":[";
  for (int i = 0; i < 24; i++) {
    json += relayState[i] ? "true" : "false";
    if (i < 23) json += ",";
  }
  json += "]}";
  server.send(200, "application/json", json);
}
