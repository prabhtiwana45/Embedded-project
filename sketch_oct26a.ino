#include <Arduino_LSM6DS3.h>  // Built-in accelerometer
#include <Wire.h>
#include <Adafruit_GPS.h>

Adafruit_GPS GPS(&Wire);
#define gsmSerial Serial1

const float crashThreshold = 2.5; // g-force threshold (~2.5g)
bool crashDetected = false;
const unsigned long smsInterval = 60000; // optional SMS interval
unsigned long lastSmsTime = 0;

void setup() {
  Serial.begin(9600);
  gsmSerial.begin(9600);
  Wire.begin();

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }
  Serial.println("IMU ready.");

  // GPS init
  GPS.begin(9600);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);

  // GSM init
  gsmSerial.println("AT");
  delay(1000);
  gsmSerial.println("AT+CMGF=1");
  delay(1000);
  gsmSerial.println("AT+CNMI=1,2,0,0,0"); 
  delay(1000);
  Serial.println("GSM ready.");
}

void loop() {
  // Read GPS
  GPS.read();

  // Read accelerometer
  float x, y, z;
  IMU.readAcceleration(x, y, z);
  float magnitude = sqrt(x*x + y*y + z*z); // total g-force

  if (magnitude > crashThreshold && !crashDetected) {
    crashDetected = true;
    Serial.println("Crash detected! Sending SMS...");
    sendAlert();
    crashDetected = false;
  }

  // Optional: periodic SMS even without crash
  if (millis() - lastSmsTime > smsInterval) {
    lastSmsTime = millis();
    sendAlert();
  }

  delay(100);
}

void sendAlert() {
  String msg;
  if (GPS.fix) {
    msg = "Crash detected! Location: https://maps.google.com/?q=";
    msg += String(GPS.latitude, 6);
    msg += ",";
    msg += String(GPS.longitude, 6);
  } else {
    msg = "Crash detected! GPS not fixed yet.";
  }

  Serial.println("Sending SMS...");
  gsmSerial.println("AT+CMGF=1");
  delay(1000);
  gsmSerial.print("AT+CMGS=+919915767225\r"); 
  delay(1000);
  gsmSerial.print(msg);
  delay(500);
  gsmSerial.write(26); // Ctrl+Z
  delay(5000);
  Serial.println("SMS sent!");
}
