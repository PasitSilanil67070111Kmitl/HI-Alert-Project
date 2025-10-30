#include <WiFiS3.h>
#include <DHT.h>
#include "ThingSpeak.h"
#include <LiquidCrystal_I2C.h>

#define DHTPIN 2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

char ssid[] = "AIS_SUPERSLOW";       // << ใส่ชื่อ WiFi >>
char pass[] = "touch5949";   // << ใส่รหัสผ่าน WiFi >>

WiFiClient client;

// เอา WRITE API KEY จาก ThingSpeak มาวางตรงนี้ ↓
unsigned long myChannelNumber = 3133698;   // ตัวเลขช่อง เช่น 2543998
const char * myWriteAPIKey = "TMMJM2FZUOMWV22P";     // เช่น "ABCD1234XYZ5678"

void setup() {
  Serial.begin(9600);
  dht.begin();
  WiFi.begin(ssid, pass);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("HI-Alert");
  lcd.setCursor(0, 1);
  lcd.print("Project");
  delay(2000);
  lcd.clear();
  lcd.print("Connecting");
  lcd.setCursor(0, 1);
  lcd.print("to WiFi...");
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  delay(2000);
  lcd.clear();
  Serial.println("\nWiFi connected!");
  lcd.print("WiFi");
  lcd.setCursor(0, 1);
  lcd.print("Connected!");
  delay(2000);
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  lcd.clear();
  ThingSpeak.begin(client);
  lcd.print("ThingSpeak");
  lcd.setCursor(0, 1);
  lcd.print("Connected!");
  delay(2000);
}

void loop() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature(); // Celsius
  float heatIndex = dht.computeHeatIndex(temperature, humidity, false);
  lcd.clear();
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT22!");
    lcd.setCursor(0,0);
    lcd.print("Failed to read");
    lcd.setCursor(0,1);
    lcd.print("Temp & Humid");
    return;
  }else{
    Serial.println("Reading from DHT22...");
    lcd.setCursor(0,0);
    lcd.print("Reading");
    lcd.setCursor(0,1);
    lcd.print("Temp & Humid");
  }
  delay(3000);
  Serial.println("Sending data to ThingSpeak...");
  Serial.print("Temp: "); Serial.print(temperature);
  Serial.print(" °C | Hum: "); Serial.print(humidity);
  Serial.print(" % | HI: "); Serial.println(heatIndex);
/*
  // ส่งค่าขึ้นแต่ละ Field
  ThingSpeak.setField(1, temperature);
  ThingSpeak.setField(2, humidity);
  ThingSpeak.setField(3, heatIndex);

  int status = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

  if (status == 200) {
    Serial.println("✅ Data sent to ThingSpeak successfully!");
  } else {
    Serial.print("❌ Error sending data. HTTP code: ");
    Serial.println(status);
  }
*/
  for(int i=0;i<9;++i){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.print(temperature, 1);
    lcd.print((char)223);
    lcd.print("C");

    lcd.setCursor(9, 0);
    lcd.print("H:");
    lcd.print(humidity, 0);
    lcd.print("%");

    lcd.setCursor(0, 1);
    lcd.print("HI:");
    lcd.print(heatIndex, 1);
    lcd.print((char)223);
    lcd.print("C");
    delay(3000); //เว้น 3 วินาทีเพื่อสลับระหว่างข้อมูลและคำเตือน
    if(temperature>45){
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Error Too Hot");
      lcd.setCursor(0, 1);
      lcd.print("Might be Fire");
    }
    else if (heatIndex<=30){
      
    }else if(heatIndex<35){
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Stay Hydrated"); 
    }else if(heatIndex<43){
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Cauldon"); 
    }else if(heatIndex<53){
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Extra Cauldon"); 
    }else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Danger");  
    }
    delay(3000); //เว้น 3 วินาทีเพื่อสลับระหว่างข้อมูลและคำเตือน
  }

}
