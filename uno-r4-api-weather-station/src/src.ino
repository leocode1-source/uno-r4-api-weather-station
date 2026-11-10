#include <WiFiS3.h>
#include <WiFiSSLClient.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"
#include "arduino_secrets.h"

// --- CONFIGURATION RÉSEAU & LOCALISATION ---
const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;
const char* mqtt_server = MQTT_BROKER;
const char* mqtt_topic = "unor4/meteo/live";

// Coordonnées GPS : 
String latitude = "00.000";
String longitude = "0.000";
const char* api_host = "://open-meteo.com";

WiFiSSLClient sslClient;
WiFiClient mqttWifiClient;
PubSubClient mqttClient(mqttWifiClient);
ArduinoLEDMatrix matrix;

unsigned long lastUpdate = 0;
const unsigned long updateInterval = 60000; // Actualisation toutes les 60 secondes

// Variables météo mémorisées
float currentTemp = 0.0;
int currentHumidity = 0;
int currentWeatherCode = 0;
int isDay = 1; // 1 = Jour, 0 = Nuit
bool hasWeatherData = false;

// --- DESSINS DES ICÔNES MÉTÉO (Matrice 8 lignes x 12 colonnes) ---

// 1A. Soleil (Jour dégagé)
byte icon_sun[8][12] = {
  { 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0 },
  { 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0 },
  { 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0, 0 },
  { 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0 },
  { 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

// 1B. Lune et étoiles (Nuit dégagée)
byte icon_moon[8][12] = {
  { 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0 },
  { 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0 },
  { 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0 },
  { 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0 },
  { 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

// 2A. Soleil + Nuage (Jour peu nuageux)
byte icon_partly_cloudy_day[8][12] = {
  { 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0 },
  { 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0 },
  { 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

// 2B. Lune + Nuage (Nuit peu nuageuse)
byte icon_partly_cloudy_night[8][12] = {
  { 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0 },
  { 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0 },
  { 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0 },
  { 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

// 3. Nuages (Couvert / Brouillard)
byte icon_cloud[8][12] = {
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0 },
  { 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0 },
  { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0 },
  { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

// 4. Pluie (Nuage + gouttes)
byte icon_rain[8][12] = {
  { 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0 },
  { 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0 },
  { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0 },
  { 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

// 5. Neige (Flocon)
byte icon_snow[8][12] = {
  { 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0 },
  { 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0 },
  { 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

// 6. Orage (Nuage + éclair)
byte icon_storm[8][12] = {
  { 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0 },
  { 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 },
  { 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 }
};

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000);
  matrix.begin();

  // Test visuel rapide des icônes au démarrage
  testIcons();

  connectWiFi();
  mqttClient.setServer(mqtt_server, 1883);
  fetchAndPublishWeather();
}

void loop() {
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();

  // Actualisation depuis l'API toutes les 60 secondes
  if (millis() - lastUpdate >= updateInterval) {
    lastUpdate = millis();
    fetchAndPublishWeather();
  }

  // Affichage continu en alternance : Icône météo (4 secondes) -> Température (défilement)
  if (hasWeatherData) {
    displayWeatherIcon(currentWeatherCode, isDay);
    delay(4000);
    scrollText("  " + String(currentTemp, 1) + "C  " + String(currentHumidity) + "%  ");
  }
}

void testIcons() {
  matrix.renderBitmap(icon_sun, 8, 12);
  delay(800);
  matrix.renderBitmap(icon_moon, 8, 12);
  delay(800);
  matrix.renderBitmap(icon_cloud, 8, 12);
  delay(800);
  matrix.renderBitmap(icon_rain, 8, 12);
  delay(800);
}

void displayWeatherIcon(int code, int day) {
  if (code == 0 || code == 1) {
    if (day == 1) {
      matrix.renderBitmap(icon_sun, 8, 12);                  // Soleil (Jour)
    } else {
      matrix.renderBitmap(icon_moon, 8, 12);                 // Lune + étoiles (Nuit)
    }
  } else if (code == 2) {
    if (day == 1) {
      matrix.renderBitmap(icon_partly_cloudy_day, 8, 12);    // Soleil + Nuage (Jour)
    } else {
      matrix.renderBitmap(icon_partly_cloudy_night, 8, 12);  // Lune + Nuage (Nuit)
    }
  } else if (code == 3 || code == 45 || code == 48) {
    matrix.renderBitmap(icon_cloud, 8, 12);                  // Nuages / Brouillard
  } else if ((code >= 71 && code <= 77) || code == 85 || code == 86) {
    matrix.renderBitmap(icon_snow, 8, 12);                   // Neige
  } else if (code >= 95) {
    matrix.renderBitmap(icon_storm, 8, 12);                  // Orage
  } else {
    matrix.renderBitmap(icon_rain, 8, 12);                   // Pluie
  }
}

void connectWiFi() {
  Serial.print("Connexion au Wi-Fi : ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  while (WiFi.localIP() == IPAddress(0, 0, 0, 0)) {
    delay(200);
  }

  Serial.println("\nWi-Fi connecté !");
}

void reconnectMQTT() {
  while (!mqttClient.connected()) {
    String clientId = "UNOR4-Weather-" + String(random(0xffff), HEX);
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("Broker MQTT connecté !");
    } else {
      delay(5000);
    }
  }
}

void scrollText(String text) {
  matrix.beginDraw();
  matrix.stroke(0xFFFFFFFF);
  matrix.textScrollSpeed(65);
  matrix.textFont(Font_5x7);
  matrix.beginText(0, 1, 0xFFFFFF);
  matrix.println(text);
  matrix.endText(SCROLL_LEFT);
  matrix.endDraw();
}

void fetchAndPublishWeather() {
  Serial.println("Mise à jour météo depuis Open-Meteo...");

  if (sslClient.connect(api_host, 443)) {
    String url = "/v1/forecast?latitude=" + latitude + "&longitude=" + longitude + "&current=temperature_2m,relative_humidity_2m,is_day,weather_code";

    sslClient.println("GET " + url + " HTTP/1.0");
    sslClient.println("Host: " + String(api_host));
    sslClient.println("Connection: close");
    sslClient.println();

    unsigned long timeout = millis();
    while (sslClient.available() == 0) {
      if (millis() - timeout > 5000) {
        sslClient.stop();
        return;
      }
    }

    while (sslClient.connected() || sslClient.available()) {
      String line = sslClient.readStringUntil('\n');
      if (line == "\r") break;
    }

    String payload = sslClient.readString();
    sslClient.stop();

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      currentTemp = doc["current"]["temperature_2m"];
      currentHumidity = doc["current"]["relative_humidity_2m"];
      currentWeatherCode = doc["current"]["weather_code"];
      isDay = doc["current"]["is_day"];
      hasWeatherData = true;

      Serial.print("Température : "); Serial.print(currentTemp); Serial.println(" °C");
      Serial.print("Humidité : "); Serial.print(currentHumidity); Serial.println(" %");
      Serial.print("Moment : "); Serial.println(isDay == 1 ? "Jour" : "Nuit");
      Serial.print("Code météo : "); Serial.println(currentWeatherCode);

      String mqttMsg = "{\"temperature\": " + String(currentTemp, 1) + ", \"humidity\": " + String(currentHumidity) + ", \"is_day\": " + String(isDay) + ", \"weather_code\": " + String(currentWeatherCode) + "}";
      mqttClient.publish(mqtt_topic, mqttMsg.c_str());
    }
  }
}
