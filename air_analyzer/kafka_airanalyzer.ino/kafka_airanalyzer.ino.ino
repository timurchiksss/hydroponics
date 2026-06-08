#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_CCS811.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const char* ssid = "iPhone (Timur)";
const char* password = "dddddddd";

const char* serverName = "http://172.20.10.8:5000/log";

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_BME280 bme;
Adafruit_CCS811 ccs;

float t, h, p;
int co2, tvoc;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Wire.begin();

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Подключение к WiFi...");
  }
  Serial.println("Подключено к WiFi!");

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  bme.begin(0x76);
  ccs.begin();
}

void loop() {

  t = bme.readTemperature();
  h = bme.readHumidity();
  p = bme.readPressure() / 133.322;

  if(ccs.available() && !ccs.readData()){
    co2 = ccs.geteCO2();
    tvoc = ccs.getTVOC();
    ccs.setEnvironmentalData(h, t); 
  }


  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.printf("T: %.1f C | H: %.0f%%", t, h);

  display.drawLine(0, 32, 128, 32, WHITE);
  display.setCursor(0,40);
  display.printf("CO2: %d ppm", co2);
  display.setCursor(0,53);
  display.printf("TVOC: %d ppb", tvoc);
  display.display();

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");

    StaticJsonDocument<200> doc;
    doc["temp"] = t;
    doc["co2"] = co2;
    doc["humidity"] = h;

    String requestBody;
    serializeJson(doc, requestBody);

    int httpResponseCode = http.POST(requestBody);

    if (httpResponseCode > 0) {
      Serial.print("Ответ сервера: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("Ошибка отправки: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }
  
  delay(5000); // Отправка каждые 5 секунд
}