#include <WiFi.h>                   // WiFi cho ESP32
#include <HTTPClient.h>             // Nếu gửi dữ liệu qua HTTP
#include <Wire.h>                   // I2C giao tiếp với cảm biến
#include <Adafruit_TCS34725.h>      // Cảm biến màu TCS34725
#include <BH1750.h>                 // Cảm biến ánh sáng BH1750
#include <ArduinoJson.h>            // Xử lý JSON nếu gửi dữ liệu dạng JSON

// TCS34725 dùng Wire mặc định
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_614MS, TCS34725_GAIN_1X);

// BH1750 dùng Wire1
BH1750 lightMeter;


void setup() {
  Serial.begin(115200);

  // I2C cho TCS34725 (chân mặc định 21, 22)
  Wire.begin(21, 22);
  if (tcs.begin()) {
    Serial.println("TCS34725 found!");
  } else {
    Serial.println("No TCS34725 found ... check connections");
    while (1);
  }

  // I2C riêng cho BH1750 (D26: SDA, D25: SCL)
  Wire1.begin(26, 25);
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire1)) {  // chỉ định Wire1
    Serial.println("BH1750 started successfully on Wire1");
  } else {
    Serial.println("Error initializing BH1750 on Wire1");
    while (1);
  }
}

void loop() {
  uint16_t r, g, b, c;
  tcs.getRawData(&r, &g, &b, &c);
  uint32_t colorTemp = tcs.calculateColorTemperature(r, g, b);
  uint16_t luxColor = tcs.calculateLux(r, g, b);

  float luxBH1750 = lightMeter.readLightLevel();

  Serial.println("------ Sensor Data ------");
  Serial.print("TCS34725 RGB: ");
  Serial.print("R: "); Serial.print(r);
  Serial.print(" G: "); Serial.print(g);
  Serial.print(" B: "); Serial.print(b);
  Serial.print(" C: "); Serial.println(c);

  Serial.print("TCS34725 Color Temp: ");
  Serial.print(colorTemp); Serial.println(" K");

  Serial.print("TCS34725 Lux (color): ");
  Serial.println(luxColor);

  Serial.print("BH1750 Lux: ");
  Serial.println(luxBH1750);

  Serial.println("-------------------------\n");

  delay(1000);
}
