#include <ArduinoJson.h>
StaticJsonDocument<200> doc;

#include <WiFi.h>
#include <HTTPClient.h>

#include <HC_SR04.h>
#define trigPinDistance 19
#define echoPinDistance 18
#define trigPinOrganic 17
#define echoPinOrganic 16
#define trigPinInOrganic 27
#define echoPinInOrganic 26

#define ssid "178 HOANGDIEU - T4B"
#define pass "68686868"
#define apiSend "http://192.168.14.105:8000/api/trash/1/"
#define apiGet "http://192.168.14.105:8000/api/trashtype/1/"

#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

#include <ESP32_Servo.h>
Servo myServo1;
Servo myServo2;

int servoPin1 = 4;
int servoPin2 = 2;
int led = 15;

String data;
String trashType;

void wifiSetup() {
  WiFi.begin(ssid, pass);
  Serial.print("Connecting to ");
  Serial.print(ssid);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.print("Wifi IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}

void setup() {
  wifiSetup();

  Serial.begin(115200);
  pinMode(trigPinDistance, OUTPUT);
  pinMode(echoPinDistance, INPUT);

  pinMode(trigPinOrganic, OUTPUT);
  pinMode(echoPinOrganic, INPUT);
  pinMode(trigPinInOrganic, OUTPUT);
  pinMode(echoPinInOrganic, INPUT);

  pinMode(led, OUTPUT);
  myServo1.attach(servoPin1);
  myServo2.attach(servoPin2);

  lcd.init();
  lcd.display();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Organic: ");
  lcd.setCursor(0, 1);
  lcd.print("InOrganic: ");
}

void sendPUTRequest(int organic, int inOrganic) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(apiSend);
    http.addHeader("Content-Type", "application/json");

    String data = "{\"organic\": " + String(organic) + ", \"inOrganic\": " + String(inOrganic) + "}";

    // Send HTTP PUT request with the JSON data
    int httpResponseCode = http.PUT(data);

    if (httpResponseCode > 0) {
      Serial.println("-------START PUT-------");
      String response = http.getString();
      Serial.println("PUT data: " + response);
      Serial.println("-------END PUT-------");
      Serial.println();
      Serial.println();
    } else {
      Serial.println("Error in sending PUT request");
      Serial.println();
    }
    delay(500);
    http.end();
  } else {
    Serial.println("WiFi Disconnected");
    Serial.println();
  }
}

void getData() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(apiGet);

    unsigned httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      Serial.println("-------START GET-------");
      data = http.getString();
      Serial.println("GET data: " + data);
      DeserializationError error = deserializeJson(doc, data);
      trashType = doc["name"].as<const char*>();
      Serial.println("-------END GET-------");
      Serial.println();
      Serial.println();
    } else {
      Serial.println("Error in GET data");
      Serial.println();
    }
    delay(500);
    http.end();
  } else {
    Serial.println("WiFi Disconnected");
    Serial.println();
  }
}

int readDistance() {
  digitalWrite(trigPinDistance, LOW);
  delayMicroseconds(100);
  digitalWrite(trigPinDistance, HIGH);
  delayMicroseconds(100);
  digitalWrite(trigPinDistance, LOW);
  long duration1 = pulseIn(echoPinDistance, HIGH);
  return duration1 / 2 * 0.034;
}

int readOrganic() {
  digitalWrite(trigPinOrganic, LOW);
  delayMicroseconds(100);
  digitalWrite(trigPinOrganic, HIGH);
  delayMicroseconds(100);
  digitalWrite(trigPinOrganic, LOW);
  long duration2 = pulseIn(echoPinOrganic, HIGH);
  return duration2 / 2 * 0.034;
}

int readInOrganic() {
  digitalWrite(trigPinInOrganic, LOW);
  delayMicroseconds(100);
  digitalWrite(trigPinInOrganic, HIGH);
  delayMicroseconds(100);
  digitalWrite(trigPinInOrganic, LOW);
  long duration3 = pulseIn(echoPinInOrganic, HIGH);
  return duration3 / 2 * 0.034;
}


void loop() {
  Serial.println("-------START-------");
  Serial.print("Distance: ");
  int distance = readDistance();
  Serial.println(distance);

  int organic = 100 - readOrganic();
  int inOrganic = 100 - readInOrganic();

  if (distance < 20) {
    Serial.println("OPEN LID");
    Serial.println();

    myServo1.write(0);

    sendPUTRequest(organic, inOrganic);   //PUT trash amount data to server
    getData();  //GET trash type data from server
  } else {
    Serial.println("CLOSE LID");
    Serial.println();

    myServo1.write(180);
    digitalWrite(led, HIGH);
  }

  if (trashType == "organic") {
    myServo2.write(0);
  }
  if (trashType == "inOrganic") {
    myServo2.write(180);
  }

  Serial.print("Organic trash: ");
  Serial.print(organic);
  Serial.println("%");

  Serial.print("Inorganic trash: ");
  Serial.print(inOrganic);
  Serial.println("%");
  Serial.println();

  Serial.print("Trash type: ");
  Serial.println(trashType);

  trashType = "";

  lcd.setCursor(9, 0);
  lcd.print(organic);
  lcd.setCursor(11, 1);
  lcd.print(inOrganic);

  if (organic < 10) {
    lcd.setCursor(10, 0);
    lcd.print("%");
    lcd.setCursor(11, 0);
    lcd.print("  ");
  }
  if (organic < 100) {
    lcd.setCursor(11, 0);
    lcd.print("%");
  }
  // Hien thi % rac vo co
  if (inOrganic < 10) {
    lcd.setCursor(12, 1);
    lcd.print("%");
    lcd.setCursor(13, 0);
    lcd.print("  ");
  }
  if (inOrganic < 100) {
    lcd.setCursor(13, 1);
    lcd.print("%");
  }
  delay(1000);
}
