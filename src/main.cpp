#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_TCS34725.h>
#include <BH1750.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <time.h>

// Thông tin Wi-Fi
const char* ssid = "Tang 4";         // Thay bằng tên Wi-Fi của bạn
const char* password = "10101010"; // Thay bằng mật khẩu Wi-Fi của bạn

// Khởi tạo cảm biến
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_614MS, TCS34725_GAIN_1X);
BH1750 lightMeter;

// URL API server (thay bằng server của bạn)
const char* serverName = "http://your-api-server.com/api/sensor";

// Cài đặt NTP (lấy thời gian từ Internet)
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7 * 3600, 60000);  // Múi giờ +7 (Việt Nam)

// Biến lưu trạng thái cảm biến
bool sensorEnabled = false;
unsigned long sensorOnTime = 0;  // Thời điểm bật cảm biến

// Kết nối Wi-Fi
void connectWiFi() {
  Serial.print("Đang kết nối Wi-Fi: ");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Wi-Fi đã kết nối!");
  Serial.print("🔗 Địa chỉ IP: ");
  Serial.println(WiFi.localIP());
}

// Khởi động cảm biến
void enableSensors() {
  if (tcs.begin()) {
    Serial.println("✅ TCS34725 đã bật!");
  } else {
    Serial.println("❌ Lỗi khởi động TCS34725!");
  }

  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire1)) {
    Serial.println("✅ BH1750 đã bật!");
  } else {
    Serial.println("❌ Lỗi khởi động BH1750!");
  }

  sensorEnabled = true;
  sensorOnTime = millis();  // Ghi lại thời gian bật
}

// Tắt cảm biến
void disableSensors() {
  sensorEnabled = false;
  Serial.println("🔴 Cảm biến đã tắt!");
}


void sendSensorData() {
  if (sensorEnabled && WiFi.status() == WL_CONNECTED) {
    uint16_t r, g, b, c;
    tcs.getRawData(&r, &g, &b, &c);
    float lux = lightMeter.readLightLevel();

    timeClient.update();
    unsigned long epochTime = timeClient.getEpochTime();
    struct tm *ptm = gmtime((time_t *)&epochTime);
    char datetimeStr[25];
    snprintf(datetimeStr, sizeof(datetimeStr), "%04d-%02d-%02d %02d:%02d:%02d", 
             ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday,
             ptm->tm_hour, ptm->tm_min, ptm->tm_sec);

    // Tạo JSON
    String jsonData = "{";
    jsonData += "\"r\":" + String(r) + ",";
    jsonData += "\"g\":" + String(g) + ",";
    jsonData += "\"b\":" + String(b) + ",";
    jsonData += "\"c\":" + String(c) + ",";
    jsonData += "\"lux\":" + String(lux, 2) + ",";
    jsonData += "\"datetime\":\"" + String(datetimeStr) + "\"";
    jsonData += "}";

    Serial.println("🔵 Dữ liệu gửi:");
    Serial.println(jsonData);

    HTTPClient http;
    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(jsonData);

    if (httpResponseCode > 0) {
      Serial.printf("✅ Server phản hồi: %d\n", httpResponseCode);
      String response = http.getString();
      Serial.println(response);
    } else {
      Serial.printf("❌ Lỗi gửi dữ liệu: %s\n", http.errorToString(httpResponseCode).c_str());
    }

    http.end();
  }
}

// Đọc dữ liệu cảm biến
void readSensorData() {
  if (sensorEnabled) {
    uint16_t r, g, b, c;
    tcs.getRawData(&r, &g, &b, &c);
    float lux = lightMeter.readLightLevel();

    Serial.println("------ Dữ liệu cảm biến ------");
    Serial.printf("🟢 TCS34725 - R: %d, G: %d, B: %d, C: %d\n", r, g, b, c);
    Serial.printf("🟡 BH1750 - Lux: %.2f\n", lux);
    Serial.println("------------------------------");
  }
}

void setup() {
  Serial.begin(115200);
  connectWiFi();

  // Đồng bộ thời gian từ NTP
  timeClient.begin();
  timeClient.update();

  // Khởi động I2C
  Wire.begin(21, 22);  // TCS34725 (mặc định chân 21: SDA, 22: SCL)
  Wire1.begin(26, 25); // BH1750 (chân 26: SDA, 25: SCL)

  Serial.println("🛠️ Hệ thống đã khởi động!");
}

void loop() {  
  timeClient.update();  // Cập nhật thời gian

  // Lấy thời gian thực
  int currentHour = timeClient.getHours();
  int currentMinute = timeClient.getMinutes();

  // Kiểm tra xem có đến thời điểm bật cảm biến không (mỗi 30 phút)
  if (!sensorEnabled && (currentMinute == 0 || currentMinute == 30)) {
    enableSensors();
    Serial.printf("🕒 Bật cảm biến lúc %02d:%02d\n", currentHour, currentMinute);
  }

  // Tắt cảm biến sau 1 phút
  if (sensorEnabled && millis() - sensorOnTime >= 60 * 1000) {
    disableSensors();
  }

  // Đọc dữ liệu nếu cảm biến đang bật
  readSensorData();

  delay(10000);  // Đọc mỗi 10 giây
}