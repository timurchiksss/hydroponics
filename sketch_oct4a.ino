#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>

// Настройки точки доступа
const char* ap_ssid = "ESP32_Relay_Control";
const char* ap_password = "12348765";

// Адрес PCF8574 (может быть 0x20-0x27 в зависимости от перемычек)
#define PCF8574_ADDRESS 0x20

// Создаем веб-сервер на порту 80
WebServer server(80);

// Состояние реле (0-4 для 5 реле)
bool relayState[5] = {false, false, false, false, false};

// Объявляем функции заранее
void handleRoot();
void handleControl();
void handleStatus();
void handleToggle();
void handleAll();
void writePCF8574(uint8_t data);
uint8_t readPCF8574();
void updateRelays();
void scanI2C();

void setup() {
  Serial.begin(115200);
  
  // Инициализация I2C
  Wire.begin();
  
  // Инициализация PCF8574 - все выходы в HIGH (реле выключены)
  writePCF8574(0xFF);
  
  // Запускаем точку доступа
  WiFi.softAP(ap_ssid, ap_password);
  
  Serial.println("✅ Точка доступа запущена!");
  Serial.print("📶 SSID: ");
  Serial.println(ap_ssid);
  Serial.print("🌐 IP адрес: ");
  Serial.println(WiFi.softAPIP());
  
  // Настраиваем обработчики веб-сервера
  server.on("/", HTTP_GET, handleRoot);
  server.on("/control", HTTP_GET, handleControl);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/toggle", HTTP_GET, handleToggle);
  server.on("/all", HTTP_GET, handleAll);
  
  // Запускаем сервер
  server.begin();
  Serial.println("🚀 HTTP сервер запущен");
  Serial.println("💻 Откройте браузер: http://192.168.4.1");
}

void loop() {
  server.handleClient();
}

// Запись в PCF8574
void writePCF8574(uint8_t data) {
  Wire.beginTransmission(PCF8574_ADDRESS);
  Wire.write(data);
  Wire.endTransmission();
}

// Чтение из PCF8574
uint8_t readPCF8574() {
  Wire.requestFrom(PCF8574_ADDRESS, 1);
  if (Wire.available()) {
    return Wire.read();
  }
  return 0xFF;
}

// Обновить все реле на основе состояний
void updateRelays() {
  uint8_t data = 0xFF; // Все биты в 1 (реле выключены)
  
  for (int i = 0; i < 8; i++) {
    if (relayState[i]) {
      data &= ~(1 << i); // Устанавливаем бит в 0 для включения реле
    }
  }
  
  writePCF8574(data);
  Serial.print("🔄 Обновление реле: ");
  Serial.println(data, BIN);
}

// Главная страница
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>control 5 relyas</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { 
      font-family: Arial; 
      text-align: center; 
      margin: 20px;
      background: #f0f0f0;
    }
    .container {
      max-width: 500px;
      margin: 0 auto;
      background: white;
      padding: 20px;
      border-radius: 10px;
      box-shadow: 0 0 10px rgba(0,0,0,0.1);
    }
    h1 {
      color: #333;
    }
    .relay {
      margin: 15px 0;
      padding: 15px;
      border: 2px solid #ddd;
      border-radius: 8px;
      background: #f9f9f9;
    }
    .button {
      padding: 10px 20px;
      margin: 5px;
      border: none;
      border-radius: 5px;
      cursor: pointer;
      font-size: 16px;
    }
    .on { background: #4CAF50; color: white; }
    .off { background: #f44336; color: white; }
    .toggle { background: #2196F3; color: white; }
    .all { background: #FF9800; color: white; padding: 12px 24px; }
    .status { 
      padding: 8px 16px; 
      border-radius: 4px; 
      font-weight: bold;
      margin: 0 10px;
    }
    .on-status { background: #4CAF50; color: white; }
    .off-status { background: #f44336; color: white; }
  </style>
</head>
<body>
  <div class="container">
    <h1>control 5 relyas</h1>
    <p>ESP32 + PCF8574</p>
    
    <div id="relays">
      <!-- Relay buttons will be inserted here by JavaScript -->
    </div>
    
    <div style="margin: 20px 0;">
      <button class="button all" onclick="controlAll('on')">on all</button>
      <button class="button all" onclick="controlAll('off')">off all</button>
    </div>
    
    <div style="margin-top: 20px; padding: 10px; background: #e7f3ff; border-radius: 5px;">
      <strong>info:</strong><br>
      SSID: ESP32_Relay_Control<br>
      IP: 192.168.4.1
    </div>
  </div>

  <script>
    function updateRelayDisplay() {
      fetch('/status')
        .then(response => response.json())
        .then(data => {
          let html = '';
          for (let i = 0; i < 5; i++) {
            const state = data.relays[i];
            html += `
              <div class="relay">
                <h3>Реле ${i + 1}</h3>
                <span class="status ${state ? 'on-status' : 'off-status'}">
                  ${state ? 'on' : 'off'}
                </span>
                <br><br>
                <button class="button on" onclick="controlRelay(${i}, 'on')">on</button>
                <button class="button off" onclick="controlRelay(${i}, 'off')">off</button>
                <button class="button toggle" onclick="toggleRelay(${i})">switch</button>
              </div>
            `;
          }
          document.getElementById('relays').innerHTML = html;
        });
    }

    function controlRelay(relayNum, action) {
      fetch('/control?relay=' + relayNum + '&action=' + action)
        .then(() => updateRelayDisplay());
    }

    function toggleRelay(relayNum) {
      fetch('/toggle?relay=' + relayNum)
        .then(() => updateRelayDisplay());
    }

    function controlAll(action) {
      fetch('/all?action=' + action)
        .then(() => updateRelayDisplay());
    }

    // Обновляем статус каждые 2 секунды
    setInterval(updateRelayDisplay, 2000);
    
    // Первоначальная загрузка
    updateRelayDisplay();
  </script>
</body>
</html>
)rawliteral";
  
  server.send(200, "text/html", html);
}

// Управление конкретным реле
void handleControl() {
  if (server.hasArg("relay") && server.hasArg("action")) {
    int relayNum = server.arg("relay").toInt();
    String action = server.arg("action");
    
    if (relayNum >= 3 && relayNum < 7) {
      relayState[relayNum] = (action == "on");
      updateRelays();
      
      Serial.print("🔧 Реле ");
      Serial.print(relayNum);
      Serial.print(" -> ");
      Serial.println(action);
      
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Invalid relay number");
    }
  } else {
    server.send(400, "text/plain", "Missing parameters");
  }
}

// Переключение реле
void handleToggle() {
  if (server.hasArg("relay")) {
    int relayNum = server.arg("relay").toInt();
    
    if (relayNum >= 3 && relayNum < 7) {
      relayState[relayNum] = !relayState[relayNum];
      updateRelays();
      
      Serial.print("🔄 Реле ");
      Serial.print(relayNum);
      Serial.print(" переключено -> ");
      Serial.println(relayState[relayNum] ? "ON" : "OFF");
      
      server.send(200, "text/plain", relayState[relayNum] ? "ON" : "OFF");
    } else {
      server.send(400, "text/plain", "Invalid relay number");
    }
  } else {
    server.send(400, "text/plain", "Missing relay parameter");
  }
}

// Управление всеми реле
void handleAll() {
  if (server.hasArg("action")) {
    String action = server.arg("action");
    bool state = (action == "on");
    
    for (int i = 0; i < 8; i++) {
      relayState[i] = state;
    }
    updateRelays();
    
    Serial.print("🎛️ Все реле -> ");
    Serial.println(action);
    
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Missing action parameter");
  }
}

// Получение статуса всех реле
void handleStatus() {
  String json = "{\"relays\":[";
  for (int i = 0; i < 8; i++) {
    json += relayState[i] ? "true" : "false";
    if (i < 4) json += ",";
  }
  json += "]}";
  
  server.send(200, "application/json", json);
}