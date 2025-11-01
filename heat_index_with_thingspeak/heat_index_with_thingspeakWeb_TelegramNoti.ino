#include <WiFiS3.h>
#include <WiFiSSLClient.h>
#include <ArduinoHttpClient.h>
#include <DHT.h>
#include "ThingSpeak.h"
#include <LiquidCrystal_I2C.h>

// ---------- DHT22 ----------
#define DHTPIN 2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// ---------- LCD ----------
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------- Wi-Fi ----------
char ssid[] = "AIS_SUPERSLOW";
char pass[] = "touch5949";

// ---------- ThingSpeak ----------
WiFiClient client;
unsigned long myChannelNumber = 3133698;
const char* myWriteAPIKey = "TMMJM2FZUOMWV22P";

// ---------- Telegram ----------
String botToken = "8506912412:AAFb-uh8mAc6cPkwGBKwcsi2CMKjplsHw3A";  // ใส่ token ของคุณ
String chatID   = "-1003242974830";  // ใส่ chat ID
WiFiSSLClient sslClient;
HttpClient http(sslClient, "api.telegram.org", 443);

// ---------- ตัวแปรช่วยส่ง Telegram ไม่ spam ----------
int current_status=0;
int prev_status=-1;

// ---------- สถานะระบบ ----------
bool systemActive = true;

// ---------- ฟังก์ชันส่ง Telegram พร้อม retry ----------
bool sendTelegram(String message) {
  if(WiFi.status() != WL_CONNECTED) return false;

  String url = "/bot" + botToken + "/sendMessage";
  String payload = "chat_id=" + chatID + "&text=" + message;

  for(int attempt=0; attempt<3; attempt++) { // retry 3 ครั้ง
    http.beginRequest();
    http.post(url);
    http.sendHeader("Content-Type", "application/x-www-form-urlencoded");
    http.sendHeader("Content-Length", payload.length());
    http.beginBody();
    http.print(payload);
    http.endRequest();

    int statusCode = http.responseStatusCode();
    String response = http.responseBody();
    Serial.print("Telegram status: "); Serial.println(statusCode);
    if(statusCode == 200) {
      Serial.println("Notify sent to Telegram successfully!");
      http.stop();
      return true;
    }
    http.stop();
    delay(2000); // รอ 2 วินาทีก่อน retry
  }
  http.stop();
  return false;
}

void setup() {
  Serial.begin(9600);
  while(!Serial);

  dht.begin();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0); lcd.print("HI-Alert");
  lcd.setCursor(0,1); lcd.print("Project");
  delay(2000);

  // เชื่อม Wi-Fi
  lcd.clear(); lcd.print("Connecting");
  lcd.setCursor(0,1); lcd.print("to WiFi...");
  WiFi.begin(ssid, pass);
  Serial.print("Connecting to WiFi...");
  while(WiFi.status() != WL_CONNECTED){
    delay(1000); Serial.print(".");
  }
  Serial.println("\nWi-Fi connected!");
  lcd.clear(); lcd.print("WiFi Connected!");
  delay(2000);
  Serial.print("IP: "); Serial.println(WiFi.localIP());

  // ThingSpeak
  ThingSpeak.begin(client);
  lcd.clear(); lcd.print("ThingSpeak");lcd.setCursor(0,1);lcd.print("Connected");
  delay(2000);
}

void loop() {
  lcd.clear();
    // ---------- อ่านคำสั่งจาก Serial ----------
  if(Serial.available()){
    String command = Serial.readStringUntil('\n');
    command.trim();
    if(command.equalsIgnoreCase("STOP")){
      systemActive = false;
      Serial.println("System paused. Type START to resume.");
      lcd.clear();
      lcd.setCursor(0,0); lcd.print("System Paused");
    } else if(command.equalsIgnoreCase("START")){
      systemActive = true;
      Serial.println("System resumed.");
      lcd.clear();
      lcd.setCursor(0,0); lcd.print("System Active");
    }
  }
  if(!systemActive){
    delay(500);
    return;
  }
  lcd.print("Reading form");lcd.setCursor(0,1);lcd.print("DHT22");
  Serial.println("Reading from DHT22...");
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  float heatIndex = dht.computeHeatIndex(temperature, humidity, false);
  Serial.print("Temp: "); Serial.print(temperature);
  Serial.print(" °C | Hum: "); Serial.print(humidity);
  Serial.print(" % | HI: "); Serial.println(heatIndex);

  if(isnan(humidity) || isnan(temperature)){
    Serial.println("Failed to read from DHT22!");
    lcd.clear();
    lcd.setCursor(0,0); lcd.print("Failed to read");
    lcd.setCursor(0,1); lcd.print("Temp & Humid");
    delay(3000);
    return;
  }

  // ส่งขึ้น ThingSpeak
  ThingSpeak.setField(1, temperature);
  ThingSpeak.setField(2, humidity);
  ThingSpeak.setField(3, heatIndex);
  int status = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if(status==200) Serial.println("Data sent to ThingSpeak successfully!");
  else Serial.println("Error sending data: " + String(status));

  // แสดงผล LCD
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("T:"); lcd.print(temperature,1); lcd.print((char)223); lcd.print("C");
  lcd.setCursor(9,0);
  lcd.print("H:"); lcd.print(humidity,0); lcd.print("%");
  lcd.setCursor(0,1);
  lcd.print("HI:"); lcd.print(heatIndex,1); lcd.print((char)223); lcd.print("C");

  // ---------- ระบบแจ้งเตือน Telegram แบบไม่ spam ----------
  String alertMsg = "";
  if(heatIndex > 53) current_status=4;
  else if(heatIndex > 43) current_status=3;
  else if(heatIndex > 35) current_status=2;
  else if(heatIndex > 30) current_status=1;
  else                    current_status=0;
  if(prev_status!=current_status) {
    if(current_status==4) alertMsg = "⚠️ อันตรายสามารถเกิด Heat Stroke ได้ตลอดเวลา อันตรายถึงชีวิต :ดัชนีความร้อน =" + String(heatIndex,1) + "°C";
    else if(current_status==3) alertMsg = "🔥 อากาศร้อนควรหลีกเลี่ยงทำกิจกรรมกลางแดด เพื่อหลีกเลี่ยง Heat Stroke : ดัชนีความร้อน =" + String(heatIndex,1) + "°C";
    else if(current_status==2) alertMsg = "☀️ อากาศค่อนข้างร้อน หมั่นดื่มน้ำ และควรหลีกเลี่ยงทำกิจกรรมกลางแดดติดต่อกันเป็นเวลานาน: ดัชนีความร้อน =" + String(heatIndex,1) + "°C";
    else if(current_status==1) alertMsg = "⛅️ อากาศสบาย แต่ก็ควรดื่มน้ำและป้องกันแสงแดดระหว่างวัน: ดัชนีความร้อน =" + String(heatIndex,1) + "°C";
    else if(current_status==0) alertMsg = "❄️ อากาศค่อนข้างเย็น ไม่มีอะไรต้องกังวล: ดัชนีความร้อน =" + String(heatIndex,1) + "°C";
    prev_status=current_status;
  }
  if(alertMsg != "") sendTelegram(alertMsg);
  delay(20000); // วน loop ทุก 20 วินาที
}
