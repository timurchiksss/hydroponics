#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_CCS811.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Настройки
const char* ssid = "ESP32_Air_Analyzer";
const char* password = "password123";
const int ledPin = 2;          // Встроенный светодиод
const int thresholdCO2 = 5000; // Порог включения тревоги

WebServer server(80);
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_BME280 bme;
Adafruit_CCS811 ccs;

float t, h, p;
int co2, tvoc;
bool alertActive = false;

void handleRoot() {
  String html = "<!DOCTYPE html><html>";
  html += "<head><meta name='viewport' content='width=device-width, initial-scale=1' charset='UTF-8'>";
  html += "<style>body{font-family:Arial; text-align:center; background:#f0f0f0;}";
  html += ".card{background:white; padding:20px; margin:10px; border-radius:10px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); display:inline-block; width:180px;}";
  html += ".alert{color: white; background: " + String(alertActive ? "#ff4757" : "#2ed573") + "; padding: 15px; margin: 20px; border-radius: 5px; font-weight: bold;}";
  html += ".val{font-size:24px; font-weight:bold; color:#007bff;}</style>";
  html += "<script>setInterval(function(){location.reload();}, 3000);</script></head>";
  html += "<body><h1>ESP32 Air Monitor</h1>";
  
  // Статус на сайте
  if(alertActive) {
    html += "<div class='alert'>ВНИМАНИЕ: Высокий уровень CO2! Проветрите!</div>";
  } else {
    html += "<div class='alert'>Воздух в норме</div>";
  }

  html += "<div class='card'>Temp<br><span class='val'>" + String(t, 1) + " &deg;C</span></div>";
  html += "<div class='card'>Hum<br><span class='val'>" + String(h, 0) + " %</span></div>";
  html += "<div class='card'>CO2<br><span class='val'>" + String(co2) + " ppm</span></div>";
  
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  
  pinMode(ledPin, OUTPUT); // Настраиваем GPIO 2 на выход
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  bme.begin(0x76);
  ccs.begin();

  WiFi.softAP(ssid, password);
  server.on("/", handleRoot);
  server.begin();
}

void loop() {
  server.handleClient();

  t = bme.readTemperature();
  h = bme.readHumidity();
  p = bme.readPressure() / 133.322;

  if(ccs.available() && !ccs.readData()){
    co2 = ccs.geteCO2();
    tvoc = ccs.getTVOC();
    ccs.setEnvironmentalData(h, t); 
  }

  // --- ЛОГИКА ТРЕВОГИ ---
  if (co2 >= thresholdCO2) {
    digitalWrite(ledPin, HIGH); // Включаем светодиод
    alertActive = true;
  } else {
    digitalWrite(ledPin, LOW);  // Выключаем светодиод
    alertActive = false;
  }
  // ----------------------

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.printf("T: %.1f C | H: %.0f%%", t, h);
  
  // Добавим индикатор на дисплей
  display.setCursor(0,20);
  if(alertActive) display.print("!!! VENTILATE !!!");
  else display.print("Air Quality: OK");

  display.drawLine(0, 32, 128, 32, WHITE);
  display.setCursor(0,40);
  display.printf("CO2: %d ppm", co2);
  display.setCursor(0,53);
  display.printf("TVOC: %d ppb", tvoc);
  display.display();

  delay(1000);
}