#include <ArduinoJson.h>
// Tạo object doc với size 200 bytes
StaticJsonDocument<200> doc;

#include <WiFi.h>
// Khai báo thư viện HTTPClient
#include <HTTPClient.h>
// Khai báo cảm biến khoản cách
#include <HC_SR04.h>
// Khai báo chân cảm biến HC_SR04 đo khoản cách | chân Trig: phát ra sóng âm, Echo: nhận sóng âm
#define trigPinDistance 19
#define echoPinDistance 18
// Khai báo chân cảm biến HC_SR04 đo lượng rác Hữu cơ
#define trigPinOrganic 17
#define echoPinOrganic 16
// Khai báo chân cảm biến HC_SR04 đo lượng rác vô cơ
#define trigPinInOrganic 27
#define echoPinInOrganic 26
// Khai báo wifi
#define ssid "tuanpham"
#define pass "tuanpham"
// Khai báo địa chỉ HTTP để gửi data lên Server | 192.168.14.105 là dchi IPv4 của wifi mà ESP32 kết nối (nó có thể thay đổi tùy wifi mà ESP32 kết nối)
#define apiSend "http://192.168.14.105:8000/api/trash/1/"
// Khai báo địa chỉ HTTP để lấy data loại rác từ Server
#define apiGet "http://192.168.14.105:8000/api/trashtype/1/"
// Khai báo màn hình LCD_I2C
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);
// Khai báo Servo (đóng mở nắp)
#include <ESP32_Servo.h>

// Gán Servo vào myServo1, myServo2
Servo myServo1;
Servo myServo2;
// Khai báo chân cho Servo nắp ngoài
int servoPin1 = 4;
// Khai báo chân cho Servo nắp trong
int servoPin2 = 2;
// Khai báo chân cho đèn led
int led = 15;
// Khai báo data để gửi lên Server
String data;
// Khai báo loại rác nhận từ Server
String trashType;

// Hàm Wifi setup
void wifiSetup() {
  // Kết nối wifi
  WiFi.begin(ssid, pass);
  Serial.print("Connecting to ");
  Serial.print(ssid);
  // Khi wifi != connected thì in ...
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  // In địa chỉ wifi
  Serial.print("Wifi IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}

void setup() {
  // Gọi hàm wifi setup
  wifiSetup();
  // Khai báo cổng giao tiếp, giống như Terminal
  Serial.begin(115200);
  // Gán tigPin là OUTPUT: phát ra sóng âm
  pinMode(trigPinDistance, OUTPUT);
  // Gán echoPin là INPUT: nhận sóng âm
  pinMode(echoPinDistance, INPUT);

  pinMode(trigPinOrganic, OUTPUT);
  pinMode(echoPinOrganic, INPUT);
  pinMode(trigPinInOrganic, OUTPUT);
  pinMode(echoPinInOrganic, INPUT);
  // Khai báo OUTPUT cho led
  pinMode(led, OUTPUT);
  // Gán chân vào Servo
  myServo1.attach(servoPin1);
  myServo2.attach(servoPin2);

  // Khởi tạo màn hình LCD
  lcd.init();
  lcd.display();
  // Đèn lcd
  lcd.backlight();
  // Set con trỏ cho mh LCD | (0, 0) là cột 1, hàng 1
  lcd.setCursor(0, 0);
  // In ra LCD
  lcd.print("Organic: ");
  // (0, 1) là cột 1, hàng 2
  lcd.setCursor(0, 1);
  lcd.print("InOrganic: ");
}

// Hàm gửi ycau PUT nhận 2 tham số: (int organic, int inOrganic)
void sendPUTRequest(int organic, int inOrganic) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    // Gán apiSend vào begin() để bắt đầu kết nối HTTP
    http.begin(apiSend);
    // addHeader nhằm báo cho server biết loại tt đc gửi(Content-Type), body của Request là dữ liệu JSON (Json data là 1key - 1value)
    http.addHeader("Content-Type", "application/json");
    // Tạo JDON data 1key - 1value gồm 2 mục: lượng rác hữu cơ và vô cơ
    String data = "{\"organic\": " + String(organic) + ", \"inOrganic\": " + String(inOrganic) + "}";

    // Send HTTP PUT request with the JSON data, PUT() là đẩy data lên server
    int httpResponseCode = http.PUT(data);
    // httpResponseCode: là status server gửi về
    if (httpResponseCode > 0) {
      Serial.println("-------START PUT-------");
      // Khai báo response để nhận status server gửi về
      String response = http.getString();
      Serial.println("PUT data: " + response);
      Serial.println("-------END PUT-------");
      Serial.println();
      Serial.println();
    } else {
      // In ra lỗi
      Serial.println("Error in sending PUT request");
      Serial.println();
    }
    // Delay 0.5s
    delay(500);
    // Đóng kết nối http
    http.end();
  } else {
    // In ra kh kết nối đc wifi
    Serial.println("WiFi Disconnected");
    Serial.println();
  }
}

// Hàm nhận data: nhận tên rác organic hoặc inOrganic
void getData() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(apiGet);
    // GET data, GET() là lấy data từ server
    unsigned httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      Serial.println("-------START GET-------");
      data = http.getString();
      Serial.println("GET data: " + data);
      // deserializeJson(doc, data): mã hóa JSON data(trả về từ server) thành doc
      DeserializationError error = deserializeJson(doc, data);
      // Gán biến trashType = doc["name"] để lấy value của key "name"
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

// Đọc khoản cách
int readDistance() {
  // Tắt trigPin
  digitalWrite(trigPinDistance, LOW);
  delayMicroseconds(100);
  // Bất trigPin để phát sóng ấm
  digitalWrite(trigPinDistance, HIGH);
  delayMicroseconds(100);
  // Tắt trigPin
  digitalWrite(trigPinDistance, LOW);
  // Hàm pulseIn() đo dộ dài của 1 xung. Hàm này trả về thời lượng xung tính bằng micro giây
  long duration1 = pulseIn(echoPinDistance, HIGH);
  // Trả về cm
  return duration1 / 2 * 0.034;
}

// Đọc lượng rác hữu cơ
int readOrganic() {
  digitalWrite(trigPinOrganic, LOW);
  delayMicroseconds(100);
  digitalWrite(trigPinOrganic, HIGH);
  delayMicroseconds(100);
  digitalWrite(trigPinOrganic, LOW);
  long duration2 = pulseIn(echoPinOrganic, HIGH);
  return duration2 / 2 * 0.034;
}

// Đọc lượng rác vô cơ
int readInOrganic() {
  digitalWrite(trigPinInOrganic, LOW);
  delayMicroseconds(100);
  digitalWrite(trigPinInOrganic, HIGH);
  delayMicroseconds(100);
  digitalWrite(trigPinInOrganic, LOW);
  long duration3 = pulseIn(echoPinInOrganic, HIGH);
  return duration3 / 2 * 0.034;
}

// Hàm loop: lập lại các thao tác
void loop() {
  // in ra Terminal
  Serial.println("-------START-------");
  Serial.print("Distance: ");
  // gán biến distance = hàm readDistance() ở trên
  int distance = readDistance();
  // In k/cach ra terminal
  Serial.println(distance);

  // 100 là chiều dài thùng rác - k/cach từ HC_SR04 tới rác = lượng rác
  int organic = 100 - readOrganic();
  int inOrganic = 100 - readInOrganic();

  // Nếu k/cach < 20cm
  if (distance < 20) {
    Serial.println("OPEN LID");
    Serial.println();
    // Mở nắp ngoài
    myServo1.write(0);
    // Gửi data lên Server
    sendPUTRequest(organic, inOrganic);
    // Nhận data từ Server
    getData();
  } else {  //Nếu k/cach > 20cm
    Serial.println("CLOSE LID");
    Serial.println();
    // Đóng nắp ngoài
    myServo1.write(180);
    // Bất đèn led
    digitalWrite(led, HIGH);
  }

  // Nếu trashType = "organic" => mở nắp trong, ngăn hữu cơ
  if (trashType == "organic") {
    myServo2.write(0);
  }
  // Nếu trashType = "inOrganic" => mở nắp trong, ngăn vô cơ
  if (trashType == "inOrganic") {
    myServo2.write(180);
  }

  // In % rác hữu cơ
  Serial.print("Organic trash: ");
  Serial.print(organic);
  Serial.println("%");

  // In % rác vô cơ
  Serial.print("Inorganic trash: ");
  Serial.print(inOrganic);
  Serial.println("%");
  Serial.println();

  // In loại rác vừa đc bỏ vào thùng
  Serial.print("Trash type: ");
  Serial.println(trashType);

  // Reset tên loại rác cho lần nhận data sau
  trashType = "";

  // In % rác hữu cơ ra mh LCD
  lcd.setCursor(9, 0);
  lcd.print(organic);
  // In % rác vô cơ ra mh LCD
  lcd.setCursor(11, 1);
  lcd.print(inOrganic);

  // In ký tự % đứng sau lượng rác
  if (organic < 10) {   //if lượng rác có 1 số => ký tự % đứng ngay sau
    lcd.setCursor(10, 0);
    lcd.print("%");
    // bỏ ký tự dư khi lần trc in 2 chữ số
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
  // delay 1s
  delay(1000);
}
