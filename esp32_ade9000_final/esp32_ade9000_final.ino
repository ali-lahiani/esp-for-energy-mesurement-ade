// ============================================================
//  ESP32 DevKitC-32UE — ADE9000 Web Server
//  Version test : données simulées (fake data)
//  WiFi : SESCO_Plus
// ============================================================

#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include "esp_wifi.h"

// ─── Configuration WiFi ────────────────────────────────────
const char* WIFI_SSID     = "SESCO_Plus";
const char* WIFI_PASSWORD = "/My@SeSco/TunisiA2014/";

// ─── Pins ──────────────────────────────────────────────────
#define PIN_SS     5
#define PIN_SCLK   18
#define PIN_MISO   19
#define PIN_MOSI   23
#define PIN_RESET  25
#define PIN_IRQ0   32
#define PIN_IRQ1   33

// ─── Serveur Web ───────────────────────────────────────────
WebServer server(80);

// ─── Structure des données ─────────────────────────────────
struct MeasureData {
  float current_A;
  float voltage_V;
  float power_W;
  float power_VAR;
  float power_VA;
  float power_factor;
  float frequency_Hz;
  bool  valid;
};

MeasureData lastMeasure = {0, 0, 0, 0, 0, 0, 50.0, false};
unsigned long lastReadTime = 0;

// ────────────────────────────────────────────────────────────
//  ROUTES DU SERVEUR WEB
// ────────────────────────────────────────────────────────────

void handle_data() {
  if (!lastMeasure.valid) {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(503, "application/json", "{\"error\":\"No valid data\"}");
    return;
  }

  char json[512];
  snprintf(json, sizeof(json),
    "{"
      "\"current\":%.2f,"
      "\"voltage\":%.1f,"
      "\"power\":%.1f,"
      "\"power_var\":%.1f,"
      "\"power_va\":%.1f,"
      "\"pf\":%.3f,"
      "\"frequency\":%.2f,"
      "\"timestamp\":%lu"
    "}",
    lastMeasure.current_A,
    lastMeasure.voltage_V,
    lastMeasure.power_W,
    lastMeasure.power_VAR,
    lastMeasure.power_VA,
    lastMeasure.power_factor,
    lastMeasure.frequency_Hz,
    millis()
  );

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

void handle_status() {
  char json[256];
  snprintf(json, sizeof(json),
    "{"
      "\"ip\":\"%s\","
      "\"uptime_s\":%lu,"
      "\"wifi_rssi\":%d,"
      "\"valid\":%s"
    "}",
    WiFi.localIP().toString().c_str(),
    millis() / 1000,
    WiFi.RSSI(),
    lastMeasure.valid ? "true" : "false"
  );
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

void handle_root() {
  server.sendHeader("Location", "/dashboard", true);
  server.send(302, "text/plain", "");
}

void handle_dashboard() {
  // Ouvrez le fichier esp32_dashboard.html
  // et collez son contenu entre les guillemets ci-dessous
  String html = "<!DOCTYPE html><html><body>"
                "<h2>Dashboard</h2>"
                "<p>Ouvrez le fichier esp32_dashboard.html "
                "dans votre navigateur et changez l IP en : "
                "http://192.168.1.37/data</p>"
                "</body></html>";
  server.send(200, "text/html", html);
}

void handle_not_found() {
  server.send(404, "text/plain", "Not found");
}

// ────────────────────────────────────────────────────────────
//  CONNEXION WIFI
// ────────────────────────────────────────────────────────────

void connectWiFi() {
  Serial.println("\n[WiFi] Connexion...");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(500);

  // IP fixe — toujours la meme adresse
  IPAddress local_IP(192, 168, 1, 100);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.config(local_IP, gateway, subnet);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  for (int i = 0; i < 30; i++) {
    delay(1000);
    int status = WiFi.status();
    Serial.printf("  [%ds] Status = %d", i + 1, status);

    if      (status == 0) Serial.println(" (Idle)");
    else if (status == 1) Serial.println(" (SSID introuvable !)");
    else if (status == 3) Serial.println(" CONNECTE !");
    else if (status == 4) Serial.println(" (Mot de passe incorrect !)");
    else if (status == 6) Serial.println(" (Deconnecte)");
    else                  Serial.println();

    if (status == WL_CONNECTED) {
      Serial.printf("\n[WiFi] IP : http://%s\n",
        WiFi.localIP().toString().c_str());
      Serial.printf("[WiFi] Dashboard : http://%s/data\n",
        WiFi.localIP().toString().c_str());
      return;
    }
  }

  Serial.println("[WiFi] ERREUR — impossible de se connecter");
}

// ────────────────────────────────────────────────────────────
//  SETUP
// ────────────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);
  delay(3000);
  Serial.println("\n=== ESP32 ADE9000 Monitor ===");
  Serial.println("=== Version : Fake Data (test) ===");

  // Pins IRQ
  pinMode(PIN_IRQ0, INPUT);
  pinMode(PIN_IRQ1, INPUT);

  // Connexion WiFi
  connectWiFi();

  // Routes serveur
  server.on("/",          handle_root);
  server.on("/dashboard", handle_dashboard);
  server.on("/data",      handle_data);
  server.on("/status",    handle_status);
  server.onNotFound(handle_not_found);
  server.begin();

  Serial.println("[HTTP] Serveur demarre sur le port 80");
  Serial.println("[HTTP] Ouvrez : http://192.168.1.100/data");
}

// ────────────────────────────────────────────────────────────
//  LOOP — FAKE DATA (remplacer par ade9000_read_all() plus tard)
// ────────────────────────────────────────────────────────────

void loop() {
  // Reconnexion automatique si WiFi perdu
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] Connexion perdue — reconnexion...");
    connectWiFi();
  }

  // Traitement requetes HTTP
  server.handleClient();

  // Simulation données ADE9000 toutes les 1 seconde
  unsigned long now = millis();
  if (now - lastReadTime >= 1000) {
    lastReadTime = now;

    // ── Remplacez ce bloc par ade9000_read_all() ──
    float base = 250.0 + sin(now / 8000.0) * 60.0;
    lastMeasure.current_A    = base + random(-15, 15);
    lastMeasure.voltage_V    = 230.0 + random(-4, 4);
    lastMeasure.power_W      = lastMeasure.current_A * lastMeasure.voltage_V * 0.91;
    lastMeasure.power_VAR    = lastMeasure.power_W * 0.25;
    lastMeasure.power_VA     = lastMeasure.current_A * lastMeasure.voltage_V;
    lastMeasure.power_factor = 0.91 + (random(-3, 3) / 100.0);
    lastMeasure.frequency_Hz = 50.0;
    lastMeasure.valid        = true;
    // ─────────────────────────────────────────────

    Serial.printf("[FAKE] I=%.1fA  V=%.1fV  P=%.0fW  PF=%.2f\n",
      lastMeasure.current_A,
      lastMeasure.voltage_V,
      lastMeasure.power_W,
      lastMeasure.power_factor
    );
  }
}
