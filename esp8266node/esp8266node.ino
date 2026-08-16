#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "heinrich";
const char* password = "heinrich";

// ===== STATIC IP CONFIG =====
IPAddress local_IP(192, 168, 254, 200);   
IPAddress gateway(192, 168, 254, 1);      
IPAddress subnet(255, 255, 255, 0);

// Create a web server on port 80
ESP8266WebServer server(80);

// ===== HANDLE DATA =====
void handleData() {
  // Fake sensor values
  int N = random(20, 150);
  int P = random(20, 100);
  int K = random(20, 100);
  int Moisture = random(10, 80);

  // Serial Monitor Log
  Serial.println("\n[!] Request Received at /data");
  Serial.print(">> Sending Values - N: "); Serial.print(N);
  Serial.print(" P: "); Serial.print(P);
  Serial.print(" K: "); Serial.print(K);
  Serial.print(" Moisture: "); Serial.println(Moisture);

  // Create JSON response
  StaticJsonDocument<200> doc;
  doc["N"] = N;
  doc["P"] = P;
  doc["K"] = K;
  doc["Moisture"] = Moisture;

  String json;
  serializeJson(doc, json);

  server.send(200, "application/json", json);
  Serial.println(">> JSON sent successfully.");
}

// ===== SETUP =====
void setup() {
  Serial.begin(9600);
  delay(500);
  
  Serial.println("\n\n--- Project NPK Hardware Starting ---");

  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("[ERROR] Failed to configure Static IP");
  }

  Serial.print("[WiFi] Connecting to: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n[WiFi] Connected Successfully!");
  Serial.print("[WiFi] IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/data", handleData);
  server.begin();
  Serial.println("[SERVER] ESP8266 Server is listening...");
}

// ===== LOOP (This was missing!) =====
void loop() {
  server.handleClient(); // This keeps the server running
}