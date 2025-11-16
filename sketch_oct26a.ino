#include <Arduino_LSM6DS3.h>   // Built-in IMU
#include <TinyGPS++.h>         // GPS library

TinyGPSPlus gps;

#define GPS_PORT Serial2       // GPS module connected to Serial2
HardwareSerial &sim800 = Serial1;   // SIM800L connected to Serial1

float x, y, z;
bool sent = false;

void setup() {
  Serial.begin(9600);
  sim800.begin(9600);      // GSM
  GPS_PORT.begin(9600);    // GPS

  // Start IMU (Gyroscope & Accelerometer)
  if (!IMU.begin()) {
    Serial.println("IMU NOT FOUND!");
    while (1);
  }

  delay(2000);
  Serial.println("Initializing SIM800L...");

  // Basic GSM initialization
  sim800.println("AT");
  delay(1000);
  sim800.println("AT+CMGF=1");   // TEXT mode

  Serial.println("Setup done.");
}

void loop() {
  // Read GPS incoming data
  while (GPS_PORT.available()) {
    gps.encode(GPS_PORT.read());
  }

  // Accident (fall) detection using gyroscope
  if (IMU.gyroscopeAvailable()) {
    IMU.readGyroscope(x, y, z);

    // Threshold accident detection
    if (abs(x) > 120 || abs(y) > 120 || abs(z) > 120) {
      if (!sent) {
        Serial.println("ACCIDENT DETECTED!");
        sendLocationSMS("+919915767225");
        sent = true;    // Prevents repeat messages
      }
    } else {
      sent = false;     // Reset when stable
    }
  }
}

void sendLocationSMS(String number) {

  // Wait for GPS fix
  while (!gps.location.isValid()) {
    Serial.println("Waiting for GPS fix...");
    while (GPS_PORT.available()) gps.encode(GPS_PORT.read());
    delay(1000);
  }

  float lat = gps.location.lat();
  float lon = gps.location.lng();

  // Google Maps link
  String link = "https://maps.google.com/?q=" + String(lat, 6) + "," + String(lon, 6);

  // START SMS
  sim800.println("AT+CMGS=\"" + number + "\"");
  delay(1000);

  // *** MESSAGE FORMAT LIKE YOUR PHOTO ***
  sim800.println("⚠️ CRASH DETECTED!");
  sim800.println("Smart Helmet Alert");
  sim800.println("Location:");
  sim800.println(link);
  sim800.println("Please check immediately!");

  sim800.write(26);   // CTRL+Z = send SMS
  delay(2000);

  Serial.println("SMS SENT!");
}
