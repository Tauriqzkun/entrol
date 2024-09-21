#include <Adafruit_Fingerprint.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#elif defined(ESP32)
#include <WiFi.h>
#include <AsyncTCP.h>
#endif
#include <LiquidCrystal_I2C.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

#if (defined(__AVR__) || defined(ESP8266)) && !defined(__AVR_ATmega2560__)
//SoftwareSerial mySerial(12, 13);
SoftwareSerial mySerial(0,2);
LiquidCrystal_I2C lcd(0x27, 20, 4);
#else
#define mySerial Serial1
#endif

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

const char* ssid = "HP_30";
const char* password = "qwerty235intek";

const String address = "http://172.16.0.212:5555/ID.php";
const int ledPin = 15;
String parsedID;
uint8_t id;
AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);
  while (!Serial);
  delay(100);
  lcd.init();
  lcd.backlight();
  pinMode(ledPin, OUTPUT);

  lcd.setCursor(0, 0);
  lcd.print("START...");
  lcd.setCursor(0, 1);
  lcd.print("PLEASE WAIT...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  lcd.setCursor(0, 2);
  lcd.print("IP: " + WiFi.localIP().toString());
  //  while (WiFi.status() != WL_CONNECTED) {
  //
  //    Serial.println("Connecting to WiFi...");
  //    Serial.print("IP address: ");
  //    Serial.println(WiFi.localIP());
  //    lcd.setCursor(0, 2);
  //    lcd.print("IP: " + WiFi.localIP().toString());
  //
  //    delay(2000);
  //  }

  lcd.setCursor(0, 3);
  lcd.print("           ");
  lcd.print("Connected");
  Serial.println("Connected to WiFi");

  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200, "text/plain", "MODE ENROL, ABSENSI V1.1.0");
  });

  AsyncElegantOTA.begin(&server);    // Start AsyncElegantOTA
  server.begin();

  finger.begin(57600);

  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) {
      delay(1);
    }
  }

  Serial.println(F("Reading sensor parameters"));
  finger.getParameters();
  Serial.print(F("Status: 0x"));
  Serial.println(finger.status_reg, HEX);
  Serial.print(F("Sys ID: 0x"));
  Serial.println(finger.system_id, HEX);
  Serial.print(F("Capacity: "));
  Serial.println(finger.capacity);
  Serial.print(F("Security level: "));
  Serial.println(finger.security_level);
  Serial.print(F("Device address: "));
  Serial.println(finger.device_addr, HEX);
  Serial.print(F("Packet len: "));
  Serial.println(finger.packet_len);
  Serial.print(F("Baud rate: "));
  Serial.println(finger.baud_rate);
  lcd.clear();
}

uint8_t readnumber(void) {
  uint8_t num = 0;
  while (num == 0) {
    while (!Serial.available());
    num = Serial.parseInt();
  }
  return num;
}

String fetchEnrolledData(int enrolledID) {
  WiFiClient client;
  HTTPClient http;

  String urlWithId = address + "?ID=" + String(enrolledID);
  http.begin(client, urlWithId);

  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_BLUE, 10);
    int idIndex = payload.indexOf("\"ID\":\"");
    if (idIndex != -1) {
      int idEndIndex = payload.indexOf("\"", idIndex + 6);
      parsedID = payload.substring(idIndex + 6, idEndIndex);
      Serial.print("ID: ");
      Serial.print(parsedID);
    }

    int namaIndex = payload.indexOf("\"NAMA\":\"");
    if (namaIndex != -1) {
      int namaEndIndex = payload.indexOf("\"", namaIndex + 8);
      String parsedNAMA = payload.substring(namaIndex + 8, namaEndIndex);
      Serial.print(" NAMA: ");
      Serial.println(parsedNAMA);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("MODE ENROL");
      lcd.setCursor(8, 1);
      lcd.print(parsedID);
      lcd.setCursor(0, 2);
      lcd.print(parsedNAMA);
      for (int i = 0; i < 4; ++i) {
        digitalWrite(ledPin, HIGH);
        delay(250);
        digitalWrite(ledPin, LOW);
        delay(250);
      }

      return parsedNAMA;
    }
  } else {
    Serial.print("HTTP Error code: ");
    Serial.println(httpCode);
    Serial.println(http.errorToString(httpCode).c_str());
  }
  lcd.clear();
  http.end();
  return "";
}

uint8_t getFingerprintEnroll() {
  int p = -1;
  Serial.println("Ready to enroll a fingerprint!");
  Serial.println("Please type in the ID # (from 1 to 127) you want to save this finger as...");
  lcd.clear();
  lcd.setCursor(6, 1);
  lcd.print("MODE ENROL");
  lcd.setCursor(4, 2);
  lcd.print( WiFi.localIP().toString());
  finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_YELLOW, 15);
  id = readnumber();

  if (id == 0) {
    return false;
  }

  Serial.print("Enrolling ID #");
  Serial.println(id);

  Serial.print("Waiting for a valid finger to enroll as #");
  Serial.println(id);
  lcd.clear();
  lcd.setCursor(8, 0);
  lcd.print("TEMPEL");
  digitalWrite(ledPin, HIGH);
  delay(75);
  digitalWrite(ledPin, LOW);
  delay(75);

  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_GREEN, 15);
        lcd.clear();
        delay(100);
        Serial.println("Sidik Jari Terdeteksi .... ");
        lcd.setCursor(0, 0);
        lcd.print("PROCESSING...");
        digitalWrite(ledPin, HIGH);
        delay(75);
        digitalWrite(ledPin, LOW);
        delay(75);
        break;
      case FINGERPRINT_NOFINGER:
        finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_BLUE, 5);
        Serial.println(".");
        lcd.clear();
        lcd.setCursor(8, 0);
        lcd.print("TEMPEL");
        digitalWrite(ledPin, HIGH);
        delay(75);
        digitalWrite(ledPin, LOW);
        delay(75);
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        break;
      default:
        Serial.println("Unknown error");
        break;
    }
  }

  // OK success!
  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_PURPLE, 10);
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  Serial.println("Remove finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID ");
  Serial.println(id);
  p = -1;
  Serial.println("Place the same finger again");

  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_RED, 10);
        Serial.print(".");
        lcd.clear();
        lcd.setCursor(4, 0);
        lcd.print("TEMPEL ULANG");
        digitalWrite(ledPin, HIGH);
        delay(75);
        digitalWrite(ledPin, LOW);
        delay(75);
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        break;
      default:
        Serial.println("Unknown error");
        break;
    }
  }

  // OK success!
  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  Serial.print("Creating model for #");
  Serial.println(id);

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  Serial.print("ID ");
  Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");

    // Fetch and display enrolled data from the database
    String enrolledData = fetchEnrolledData(id);
    Serial.print("Enrolled Data: ");
    Serial.println(enrolledData);
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  return true;
}


void loop() {
  // Call the fingerprint enrollment function
  AsyncElegantOTA.loop();
  while (!getFingerprintEnroll());
}
