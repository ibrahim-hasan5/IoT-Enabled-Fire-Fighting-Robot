#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "DHT.h"

// ---------------- Pin Definitions ----------------
#define IN1 D1
#define IN2 D2
#define IN3 D3
#define IN4 D4

#define FLAME_LEFT D12
#define FLAME_RIGHT D6
#define MQ2_PIN A0
#define DHTPIN D7
#define DHTTYPE DHT11

#define RELAY1_PIN D8   // First Relay
#define RELAY2_PIN D0   // Second Relay
#define BUZZER_PIN D13   

// -------------------------------------------------
DHT dht(DHTPIN, DHTTYPE);
ESP8266WebServer server(80);

String fireStatus = "No Fire Detected 🔒";
int gasValue = 0;
float temperature = 0;
float humidity = 0;
bool relay1State = false;
bool relay2State = false;

// ---------------- Motor Control ----------------
void stopCar() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
}

void moveForward() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

void moveBackward() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
}

void turnLeft() {
  digitalWrite(IN1, HIGH);  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
}

void turnRight() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);  digitalWrite(IN4, LOW);
}

// ---------------- Relay Control ----------------
void relay1On() { digitalWrite(RELAY1_PIN, LOW); relay1State = true; }
void relay1Off() { digitalWrite(RELAY1_PIN, HIGH); relay1State = false; }

void relay2On() { digitalWrite(RELAY2_PIN, LOW); relay2State = true; }
void relay2Off() { digitalWrite(RELAY2_PIN, HIGH); relay2State = false; }

// ---------------- Web Page ----------------
String HTMLPage() {
  String page = R"rawliteral(
<!DOCTYPE html>
<html lang='en'>
<head>
<meta charset='UTF-8'>
<meta name='viewport' content='width=device-width, initial-scale=1.0'>
<title>Fire Boss</title>
<style>
  body {font-family: 'Poppins', sans-serif; background:#0f172a; color:white; text-align:center; margin:0; padding:0;}
  h1 {margin:15px 0; color:#38bdf8;}
  .card {
    background:#1e293b; margin:15px auto; padding:20px; border-radius:20px;
    width:85%; max-width:450px; box-shadow:0 4px 10px rgba(0,0,0,0.4);
  }
  button {
    background:#22c55e; color:white; border:none; border-radius:12px;
    padding:12px 25px; font-size:18px; cursor:pointer; margin:6px; transition:0.25s;
  }
  button:hover {transform:scale(1.05);}
  .danger {background:#ef4444;}
  .move {width:100px; height:50px; font-size:16px;}
  hr {border:0; height:1px; background:#334155; margin:20px 0;}
  .sensor {font-size:18px; margin:6px 0;}
</style>
</head>
<body>
  <h1>🔥 Fire Boss Dashboard</h1>

  <div class='card'>
    <h2>🔥 Fire Sensor</h2>
    <p class='sensor' id='fire'>)rawliteral" + fireStatus + R"rawliteral(</p>
    <hr>
    <h2>💨 MQ-2 Gas Sensor</h2>
    <p class='sensor'>Gas Value: )rawliteral" + String(gasValue) + R"rawliteral(</p>
    <hr>
    <h2>🌡 DHT11 Sensor</h2>
    <p class='sensor'>Temp: )rawliteral" + String(temperature, 1) + R"rawliteral( °C</p>
    <p class='sensor'>Humidity: )rawliteral" + String(humidity, 1) + R"rawliteral( %</p>
  </div>

  <div class='card'>
    <h2>⚙ Relay 1 Control</h2>
    <p>Status: <b>)rawliteral" + (relay1State ? "ON ✅" : "OFF ❌") + R"rawliteral(</b></p>
)rawliteral";

  if (relay1State)
    page += "<a href=\"/relay1Off\"><button class='danger'>Turn OFF</button></a>";
  else
    page += "<a href=\"/relay1On\"><button>Turn ON</button></a>";

  page += R"rawliteral(
    <hr>
    <h2>⚙ Relay 2 Control</h2>
    <p>Status: <b>)rawliteral";
  page += (relay2State ? "ON ✅" : "OFF ❌");
  page += R"rawliteral(</b></p>
)rawliteral";

  if (relay2State)
    page += "<a href=\"/relay2Off\"><button class='danger'>Turn OFF</button></a>";
  else
    page += "<a href=\"/relay2On\"><button>Turn ON</button></a>";

  page += R"rawliteral(
  </div>

  <div class='card'>
    <h2>🎮 Motor Control</h2>
    <div>
      <a href='/forward'><button class='move'>▲</button></a><br>
      <a href='/left'><button class='move'>◀</button></a>
      <a href='/stop'><button class='move danger'>■</button></a>
      <a href='/right'><button class='move'>▶</button></a><br>
      <a href='/backward'><button class='move'>▼</button></a>
    </div>
  </div>

  <p style='color:#94a3b8;'>WiFi AP: <b>Car_AP</b> | Password: <b>12345678</b></p>
</body></html>
)rawliteral";

  return page;
}

// ---------------- Setup ----------------
void setup() {
  Serial.begin(115200);

  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  stopCar();

  pinMode(FLAME_LEFT, INPUT);
  pinMode(FLAME_RIGHT, INPUT);
  pinMode(MQ2_PIN, INPUT);

  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);   
  digitalWrite(BUZZER_PIN, LOW); // buzzer off initially

  relay1Off();
  relay2Off();

  dht.begin();

  WiFi.softAP("Car_AP", "12345678");
  Serial.println("Access Point Started");
  Serial.print("IP: "); Serial.println(WiFi.softAPIP());

  server.on("/", []() { server.send(200, "text/html", HTMLPage()); });

  // Motor routes
  server.on("/forward", []() { moveForward(); server.send(200, "text/html", HTMLPage()); });
  server.on("/backward", []() { moveBackward(); server.send(200, "text/html", HTMLPage()); });
  server.on("/left", []() { turnLeft(); server.send(200, "text/html", HTMLPage()); });
  server.on("/right", []() { turnRight(); server.send(200, "text/html", HTMLPage()); });
  server.on("/stop", []() { stopCar(); server.send(200, "text/html", HTMLPage()); });

  // Relay routes
  server.on("/relay1On", []() { relay1On(); server.sendHeader("Location", "/"); server.send(303); });
  server.on("/relay1Off", []() { relay1Off(); server.sendHeader("Location", "/"); server.send(303); });
  server.on("/relay2On", []() { relay2On(); server.sendHeader("Location", "/"); server.send(303); });
  server.on("/relay2Off", []() { relay2Off(); server.sendHeader("Location", "/"); server.send(303); });

  server.begin();
  Serial.println("Web Server Started");
}

// ---------------- Loop ----------------
void loop() {
  int leftFlame = digitalRead(FLAME_LEFT);
  int rightFlame = digitalRead(FLAME_RIGHT);

  bool fireDetected = (leftFlame == LOW || rightFlame == LOW);

  if (fireDetected) {
    fireStatus = "🔥 Fire Detected! Take Action!";
    digitalWrite(BUZZER_PIN, HIGH);  // 
  } else {
    fireStatus = "No Fire Detected 🔒";
    digitalWrite(BUZZER_PIN, LOW);   // 
  }

  gasValue = analogRead(MQ2_PIN);
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  Serial.printf("Gas: %d | Temp: %.1f°C | Hum: %.1f%% | Fire: %s | R1: %s | R2: %s\n",
                gasValue, temperature, humidity,
                fireStatus.c_str(),
                relay1State ? "ON" : "OFF",
                relay2State ? "ON" : "OFF");

  server.handleClient();
}