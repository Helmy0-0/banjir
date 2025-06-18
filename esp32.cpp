#include <WiFi.h>
#include <HTTPClient.h>
#include <DHTesp.h>

//Konstanta
#define SOUND_SPEED     0.034f  
#define CM_TO_INCH      0.393701
#define WIFI_CHANNEL    6        



//Konfigurasi WiFi
const char* SSID = "IPONG";
const char* PASS = "apingcantik";

//api url
const char* URL = "https://esp-project-gray.vercel.app/api/whatsapp";

//pin
const int TRIG_PIN    = 32;
const int WATER_SENSOR_PIN = 34; // D34
const int ECHO_PIN    = 35;
const int BUZZER_PIN  = 27;

const int LIMIT_CM = 400;  //limit

//variabel global
DHTesp dht;
float distanceCm = 0;
int waterAnalogValue = 0;

const unsigned long interval = 5000;
unsigned long previousMillis = 0;

unsigned long lastAlertWaspada = 0;
unsigned long lastAlertBahaya = 0;
bool alreadyInBahaya = false;

//setup
void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  WiFi.begin(SSID, PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("WiFi Connected!");
  Serial.println(WiFi.localIP());
}

//baca ultrasonic
float getDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // timeout 30ms
  return duration * SOUND_SPEED / 2.0f;
}

// mengirim pesan
void sendAlert(String status, float waterLevel) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi belum tersambung ‑ alert tidak dikirim.");
    return;
  }

  HTTPClient http;
  http.begin(URL);
  http.addHeader("Content-Type", "application/json");

  String payload = "{";
  payload += "\"status\":\"" + status + "\",";
  payload += "\"ketinggianAir\":\"" + String(waterLevel, 1) + "\"";
  payload += "}";

  int httpResponseCode = http.POST(payload);
  Serial.print("HTTP response: ");
  Serial.println(httpResponseCode);
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Response:");
    Serial.println(response);
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

void handleFloodLevel() {
  unsigned long now = millis();
  if (distanceCm < LIMIT_CM / 3 && waterAnalogValue > 2000) { // bahaya
    tone(BUZZER_PIN, 1000);

    if (!alreadyInBahaya || (now - lastAlertBahaya >= 7200000)) { // 2 jam
      sendAlert("BAHAYA", distanceCm);
      lastAlertBahaya = now;
    }

    alreadyInBahaya = true;

  } else if (distanceCm < (2 * LIMIT_CM / 3 && waterAnalogValue <= 2000)) { //waspada
    noTone(BUZZER_PIN);


    if (now - lastAlertWaspada >= 7200000) { // 2 jam
      sendAlert("WASPADA", distanceCm);
      lastAlertWaspada = now;
    }

    alreadyInBahaya = false;

  } else { // aman
    noTone(BUZZER_PIN);

    alreadyInBahaya = false;
  }
}

int getWaterLevelAnalog() {
  return analogRead(WATER_SENSOR_PIN); // Baca nilai analog 0-4095
}



void loop() {
  if (millis() - previousMillis > interval) {
    distanceCm = getDistanceCm();
    waterAnalogValue = getWaterLevelAnalog();
    Serial.printf("Nilai analog water level: %d\n", waterAnalogValue);
    Serial.printf("Jarak air: %.2f cm\n", distanceCm);

    handleFloodLevel();
    previousMillis = millis();
  }
}
