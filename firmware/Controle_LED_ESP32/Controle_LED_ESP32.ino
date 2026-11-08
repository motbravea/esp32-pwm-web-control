/**
 * Controle de luminosite d'une DEL par PWM et serveur HTTP - ESP32
 * Arduino ESP32 3.x | GPIO18, 5000 Hz, 8 bits
 *
 * Montage : GPIO18 -> resistance 220 ohms -> anode DEL -> cathode -> GND
 * Identifiants Wi-Fi : config.h (non publie)
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "config.h"

constexpr uint8_t LED_PIN = 18;
constexpr uint32_t PWM_FREQUENCE_HZ = 5000;
constexpr uint8_t PWM_RESOLUTION_BITS = 8;
constexpr unsigned long RECONNEXION_MS = 10000;

WebServer server(80);
int luminositePourcent = 0;
unsigned long dernierEssaiWifi = 0;
bool etaitConnecte = false;

// Convertit un pourcentage de 0 a 100 en valeur PWM 0 a 255.
uint8_t dutyVersValeur(int p) {
  return static_cast<uint8_t>((p * 255 + 50) / 100);
}

float tensionMoyenne() {
  return luminositePourcent * 3.3f / 100.0f;
}

void journaliser(const String& route, int codeHttp) {
  Serial.printf("GET %s | HTTP %d | PWM %d%% | Valeur %u/255 | Tension %.2f V\n",
                route.c_str(), codeHttp, luminositePourcent,
                dutyVersValeur(luminositePourcent), tensionMoyenne());
}

void appliquerPourcentage(int p) {
  luminositePourcent = constrain(p, 0, 100);
  ledcWrite(LED_PIN, dutyVersValeur(luminositePourcent));
}

String pageWeb() {
  String html = R"HTML(
<!DOCTYPE html>
<html lang="fr">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Controle LED ESP32</title>
  <style>
    * { box-sizing: border-box; }
    body { margin: 0; min-height: 100vh; background: #0d172a; color: #f8fafc;
           font-family: Arial, Helvetica, sans-serif; display: flex;
           align-items: center; justify-content: center; padding: 25px; }
    main { width: min(540px, 100%); text-align: center; background: #162338;
           border: 1px solid #304056; padding: 30px 18px; border-radius: 20px; }
    h1 { margin: 2px 0 25px; font-size: 26px; }
    .etat { font-size: 29px; font-weight: bold; color: #93c5fd; margin-bottom: 9px; }
    .detail { color: #cbd5e1; line-height: 1.8; }
    nav { margin-top: 22px; display: flex; gap: 10px; flex-wrap: wrap; justify-content: center; }
    a { display: inline-block; min-width: 84px; padding: 15px 12px; color: white;
        text-decoration: none; background: #2563eb; border-radius: 10px; font-size: 17px; }
    a:hover, a:focus { background: #1d4ed8; }
    .sub { margin-top: 9px; }
  </style>
</head>
<body>
  <main>
    <h1>Controle LED ESP32</h1>
    <div class="etat">Luminosite : {{POURCENT}}%</div>
    <div class="detail">Valeur PWM : {{PWM}} / 255</div>
    <div class="detail">Tension moyenne : {{VOLT}} V</div>
    <nav aria-label="Reglage de la luminosite">
      <a href="/pwm?duty=0">0%</a>
      <a href="/pwm?duty=25">25%</a>
      <a href="/pwm?duty=50">50%</a>
      <a href="/pwm?duty=75">75%</a>
      <a href="/pwm?duty=100">100%</a>
    </nav>
    <nav class="sub">
      <a href="/moins">-10%</a>
      <a href="/plus">+10%</a>
    </nav>
  </main>
</body>
</html>
)HTML";
  html.replace("{{POURCENT}}", String(luminositePourcent));
  html.replace("{{PWM}}", String(dutyVersValeur(luminositePourcent)));
  html.replace("{{VOLT}}", String(tensionMoyenne(), 2));
  return html;
}

void envoyerEtat(const String& route) {
  journaliser(route, 200);
  server.send(200, "text/html; charset=utf-8", pageWeb());
}

void erreur400(const String& route) {
  journaliser(route, 400);
  server.send(400, "text/plain; charset=utf-8",
              "Parametre duty invalide : entier attendu entre 0 et 100.");
}

// Refuse une valeur absente, non numerique ou hors des bornes.
void gererPWM() {
  String route = "/pwm";
  if (!server.hasArg("duty")) {
    erreur400(route);
    return;
  }

  const String valeurTexte = server.arg("duty");
  route += "?duty=" + valeurTexte;

  if (valeurTexte.length() < 1 || valeurTexte.length() > 3) {
    erreur400(route);
    return;
  }
  for (unsigned int i = 0; i < valeurTexte.length(); ++i) {
    if (valeurTexte[i] < '0' || valeurTexte[i] > '9') {
      erreur400(route);
      return;
    }
  }

  int p = valeurTexte.toInt();
  if (p < 0 || p > 100) {
    erreur400(route);
    return;
  }

  appliquerPourcentage(p);
  envoyerEtat(route);
}

void gererPlus() {
  appliquerPourcentage(luminositePourcent + 10);
  envoyerEtat("/plus");
}

void gererMoins() {
  appliquerPourcentage(luminositePourcent - 10);
  envoyerEtat("/moins");
}

void installerRoutes() {
  server.on("/", HTTP_GET, []() { envoyerEtat("/"); });
  server.on("/pwm", HTTP_GET, gererPWM);
  server.on("/plus", HTTP_GET, gererPlus);
  server.on("/moins", HTTP_GET, gererMoins);
  server.onNotFound([]() {
    journaliser(server.uri(), 404);
    server.send(404, "text/plain; charset=utf-8", "Page introuvable.");
  });
}

void connexionWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  dernierEssaiWifi = millis();
  Serial.println("Connexion Wi-Fi en cours...");
}

void surveillerWiFi() {
  bool connecte = WiFi.status() == WL_CONNECTED;
  if (connecte && !etaitConnecte) {
    Serial.print("Wi-Fi connecte. Adresse IP : ");
    Serial.println(WiFi.localIP());
  } else if (!connecte && etaitConnecte) {
    Serial.println("Wi-Fi perdu. LED conserve sa luminosite.");
  }
  etaitConnecte = connecte;

  if (!connecte && millis() - dernierEssaiWifi >= RECONNEXION_MS) {
    dernierEssaiWifi = millis();
    Serial.println("Nouvelle tentative de connexion Wi-Fi...");
    WiFi.reconnect();
  }
}

void setup() {
  Serial.begin(115200);
  if (!ledcAttach(LED_PIN, PWM_FREQUENCE_HZ, PWM_RESOLUTION_BITS)) {
    Serial.println("ERREUR : initialisation PWM impossible.");
  }
  appliquerPourcentage(0);
  installerRoutes();
  server.begin();
  Serial.println("Serveur HTTP sur le port 80 pret.");
  connexionWiFi();
}

void loop() {
  surveillerWiFi();
  if (WiFi.status() == WL_CONNECTED) {
    server.handleClient();
  }
}
