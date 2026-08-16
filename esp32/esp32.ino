#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

// ===== WIFI CONFIG =====
const char* ssid = "ConnectESP";
const char* password = "espconnect2";

IPAddress local_IP(192, 168, 254, 200);
IPAddress gateway(192, 168, 254, 1);
IPAddress subnet(255, 255, 255, 0);

WebServer server(80);

// ===== RS485 CONFIG =====
#define RE_DE 4
HardwareSerial mod(2);

// ===== MODBUS COMMANDS =====
byte nitro[] = {0x01,0x03,0x00,0x1e,0x00,0x01,0xe4,0x0c};
byte phos[]  = {0x01,0x03,0x00,0x1f,0x00,0x01,0xb5,0xcc};
byte pota[]  = {0x01,0x03,0x00,0x20,0x00,0x01,0x85,0xc0};

byte values[11];

// ===== GLOBAL VALUES =====
int N_val = 0;
int P_val = 0;
int K_val = 0;
int Moisture_val = 0;

// ===== TIMER =====
unsigned long lastUpdate = 0;
const unsigned long interval = 15000; // 15 seconds

// ===== RAW SENSOR READ =====
int readSensor(byte *command) {

  digitalWrite(RE_DE, HIGH);
  delay(10);

  mod.write(command, 8);
  mod.flush();

  digitalWrite(RE_DE, LOW);
  delay(10);

  int i = 0;
  unsigned long start = millis();

  while (millis() - start < 700) {
    if (mod.available()) {
      values[i++] = mod.read();
    }
  }

  Serial.print("Bytes: ");
  Serial.println(i);

  if (i >= 7) {
    int val = (values[3] << 8) | values[4];
    return val;
  } else {
    return 0; // 🔥 no filter → return 0 if no data
  }
}

// ===== UPDATE SENSOR (NO FILTER) =====
void updateSensors() {
  Serial.println("\n[SENSOR] Reading RAW NPK...");

  N_val = readSensor(nitro);
  P_val = readSensor(phos);
  K_val = readSensor(pota);

  Moisture_val = analogRead(34); // adjust pin if needed

  Serial.print("RAW → ");
  Serial.print("N: "); Serial.print(N_val);
  Serial.print(" P: "); Serial.print(P_val);
  Serial.print(" K: "); Serial.print(K_val);
  Serial.print(" M: "); Serial.println(Moisture_val);

  Serial.println("[SENSOR] Done");
}

// ===== API HANDLER =====
void handleData() {
  StaticJsonDocument<200> doc;

  doc["N"] = N_val;
  doc["P"] = P_val;
  doc["K"] = K_val;
  doc["Moisture"] = Moisture_val;

  String json;
  serializeJson(doc, json);

  server.send(200, "application/json", json);
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);

  pinMode(RE_DE, OUTPUT);
  digitalWrite(RE_DE, LOW);

  mod.begin(4800, SERIAL_8N1, 16, 17);

  Serial.println("\n--- ESP32 RAW NPK SYSTEM START ---");

  WiFi.config(local_IP, gateway, subnet);
  WiFi.begin(ssid, password);

  Serial.print("[WiFi] Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }

  Serial.println("\n[WiFi] Connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  server.on("/data", handleData);
  server.begin();

  // Initial read
  updateSensors();
  lastUpdate = millis();
}

// ===== LOOP =====
void loop() {
  server.handleClient();

  //  Read every 15 seconds
  if (millis() - lastUpdate >= interval) {
    lastUpdate = millis();
    updateSensors();
  }
}