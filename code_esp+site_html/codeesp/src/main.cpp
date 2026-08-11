#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Wire.h>
#include "ADE9000API.h"
#include "ADE9000CalibrationInputs.h"

// --- Hardware Pin Configuration ---
#define CS_PIN    5
#define SCK_PIN   18
#define MISO_PIN  19
#define MOSI_PIN  23
#define SPI_SPEED 2000000 // 2 MHz SPI clock speed

// --- Wi-Fi Credentials ---
const char* WIFI_SSID = "SESCO_Plus";
const char* WIFI_PASS = "/My@SeSco/TunisiA2014/";

// --- HiveMQ Cloud MQTT Configuration ---
const char* MQTT_SERVER = "19116538ee4141cfb8ae80def4474ba4.s1.eu.hivemq.cloud"; 
const int   MQTT_PORT   = 8883; // SSL/TLS Port
const char* MQTT_USER   = "ali";
const char* MQTT_PASS   = "123456789";

// --- MQTT Topics ---
const char* TOPIC_PHASE_A = "energy/phaseA";
const char* TOPIC_PHASE_B = "energy/phaseB";
const char* TOPIC_PHASE_C = "energy/phaseC";
const char* TOPIC_NEUTRAL = "energy/neutral";

// --- Global Objects ---
ADE9000Class ade9000;
WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

// Data Structure Instances
VoltageRMSTbl      vRmsData;
CurrentRMSTbl      iRmsData;
ActivePowerRegsTbl avgActivePower;
PowerFactorTbl     powerFactorData;
PeriodTbl          periodData;

// Timer variable for non-blocking MQTT loop execution
unsigned long lastMsgTime = 0;
const long interval = 2000; // Publish every 2 seconds

void connectToWiFi() {
  Serial.printf("\n[WiFi] Connexion a %s...\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  for (int i = 0; i < 20; i++) {
    delay(1000);
    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("[WiFi] ✅ Connecte ! IP : %s\n",
        WiFi.localIP().toString().c_str());
      return;
    }
    Serial.printf("  [%ds] status=%d\n", i+1, WiFi.status());
  }

  Serial.println("[WiFi] ❌ Echec — redemarrage...");
  ESP.restart();
}

void connectToMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("[MQTT] Connecting to HiveMQ Cloud...");
    String clientId = "ESP32-EnergyMeter-" + String(random(0xffff), HEX);

    if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
      Serial.println(" Connected successfully!");
    } else {
      Serial.print(" Failed! rc=");
      Serial.print(mqttClient.state());
      Serial.println(" Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== ESP32 Energy Meter ===");

  // ── 1. Initialize ADE9000 First (Before Wi-Fi power spike) ──
  Wire.begin(21, 22);
  delay(100);

  ade9000.SPI_Init(SPI_SPEED, CS_PIN, SCK_PIN, MISO_PIN, MOSI_PIN);
  ade9000.SetupADE9000();
  
  // Direct register start (RUN register 0x0480)
  ade9000.SPI_Write_16(0x0480, 0x0001); 
  delay(500); // Give internal DSP time to settle

  // Quick read check on Phase B Voltage register to verify communication
  ade9000.ReadVoltageRMSRegs(&vRmsData);
  float testV = (vRmsData.phase[PHASE_B] * CAL_VRMS_CC) / 1000000.0f;
  Serial.printf("[ADE] ✅ Initialized! Phase B startup check: %.2f V\n", testV);

  // ── 2. Initialize WiFi ──
  connectToWiFi();

  // ── 3. Initialize MQTT ──
  espClient.setInsecure();
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setBufferSize(512);
  connectToMQTT();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }
  if (!mqttClient.connected()) {
    connectToMQTT();
  }
  mqttClient.loop();

  unsigned long now = millis();
  if (now - lastMsgTime > interval) {
    lastMsgTime = now;

    // --- 1. Read Registers from ADE9000 ---
    ade9000.ReadVoltageRMSRegs(&vRmsData);
    ade9000.ReadCurrentRMSRegs(&iRmsData);
    ade9000.ReadPowerFactorRegsnValues(&powerFactorData);
    ade9000.ReadPeriodRegsnValues(&periodData);
    ade9000.ReadAvgActivePowerRegs(&avgActivePower);

    // --- 2. Phase Conversions ---
    float vRmsA  = (vRmsData.phase[PHASE_A] * CAL_VRMS_CC) / 1000000.0f;
    float iRmsA  = (iRmsData.phase[PHASE_A] * CAL_IRMS_CC) / 1000000.0f;
    float powerA = (avgActivePower.phase[PHASE_A] * CAL_POWER_CC) / 1000.0f;
    float pfA    = powerFactorData.value[PHASE_A];
    float freqA  = periodData.freq[PHASE_A];

    float vRmsB  = (vRmsData.phase[PHASE_B] * CAL_VRMS_CC) / 1000000.0f;
    float iRmsB  = (iRmsData.phase[PHASE_B] * CAL_IRMS_CC) / 1000000.0f;
    float powerB = (avgActivePower.phase[PHASE_B] * CAL_POWER_CC) / 1000.0f;
    float pfB    = powerFactorData.value[PHASE_B];
    float freqB  = periodData.freq[PHASE_B];

    float vRmsC  = (vRmsData.phase[PHASE_C] * CAL_VRMS_CC) / 1000000.0f;
    float iRmsC  = (iRmsData.phase[PHASE_C] * CAL_IRMS_CC) / 1000000.0f;
    float powerC = (avgActivePower.phase[PHASE_C] * CAL_POWER_CC) / 1000.0f;
    float pfC    = powerFactorData.value[PHASE_C];
    float freqC  = periodData.freq[PHASE_C];

    float iRmsN  = (iRmsData.phase[PHASE_N] * CAL_IRMS_CC) / 1000000.0f;

    // --- 3. Print Local Diagnostics ---
    Serial.println("==================================================");
    Serial.printf("Phase A: %.2f V | %.3f A | %.2f W | PF: %.2f | Freq: %.2f Hz\n", vRmsA, iRmsA, powerA, pfA, freqA);
    Serial.printf("Phase B: %.2f V | %.3f A | %.2f W | PF: %.2f | Freq: %.2f Hz\n", vRmsB, iRmsB, powerB, pfB, freqB);
    Serial.printf("Phase C: %.2f V | %.3f A | %.2f W | PF: %.2f | Freq: %.2f Hz\n", vRmsC, iRmsC, powerC, pfC, freqC);
    Serial.printf("Neutral Current: %.3f A\n", iRmsN);

    // --- 4. Construct JSON Payloads & Publish over MQTT ---
    char payload[180];

    snprintf(payload, sizeof(payload), 
             "{\"vRms\":%.2f,\"iRms\":%.3f,\"power\":%.2f,\"pf\":%.2f,\"freq\":%.2f}", 
             vRmsA, iRmsA, powerA, pfA, freqA);
    mqttClient.publish(TOPIC_PHASE_A, payload);

    snprintf(payload, sizeof(payload), 
             "{\"vRms\":%.2f,\"iRms\":%.3f,\"power\":%.2f,\"pf\":%.2f,\"freq\":%.2f}", 
             vRmsB, iRmsB, powerB, pfB, freqB);
    mqttClient.publish(TOPIC_PHASE_B, payload);

    snprintf(payload, sizeof(payload), 
             "{\"vRms\":%.2f,\"iRms\":%.3f,\"power\":%.2f,\"pf\":%.2f,\"freq\":%.2f}", 
             vRmsC, iRmsC, powerC, pfC, freqC);
    mqttClient.publish(TOPIC_PHASE_C, payload);

    snprintf(payload, sizeof(payload), "{\"iRmsN\":%.3f}", iRmsN);
    mqttClient.publish(TOPIC_NEUTRAL, payload);

    Serial.println("[MQTT] Telemetry published to HiveMQ Cloud.");
  }
}