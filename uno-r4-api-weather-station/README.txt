# 🌦️ Station Météo IoT — Arduino UNO R4 WiFi

[![Arduino](https://shields.io)](https://arduino.cc)
[![API](https://shields.io)](https://open-meteo.com)
[![Protocol](https://shields.io)](https://mqtt.org)

Une station météo connectée et autonome construite avec un **Arduino UNO R4 WiFi**, sans aucun capteur externe requis. Le projet interroge l'API météo open-source **Open-Meteo** en HTTPS, affiche des icônes météo dynamiques (jour/nuit) sur la **matrice LED 12x8 intégrée**, et publie les relevés en temps réel via le protocole **MQTT**.

---

## ✨ Fonctionnalités

* **Zéro capteur externe :** Récupère les données météorologiques en direct (température, humidité, code météo WMO, cycle jour/nuit) via l'API REST gratuite **Open-Meteo** (sans clé API requise).
* **Affichage dynamique sur Matrice LED 12x8 :**
  * Icônes graphiques personnalisées adaptées au temps et à l'heure : ☀️ Soleil, 🌙 Lune & étoiles, ⛅ Soleil/Nuage, ☁️ Nuages, 🌧️ Pluie, ❄️ Neige et ⚡ Orage.
  * Défilement fluide de la température (°C) et du taux d'humidité (%).
* **Télémétrie IoT via MQTT :** Publication automatique des mesures au format JSON toutes les 60 secondes sur un broker MQTT.
* **Sécurité des identifiants :** Gestion des accès Wi-Fi séparée via un fichier `arduino_secrets.h` ignoré par Git.

---

## 🛠️ Matériel & Prérequis

### Matériel
* 1x Carte **Arduino UNO R4 WiFi**
* 1x Câble USB-C pour l'alimentation et le téléversement

### Bibliothèques Arduino requises
Installez ces bibliothèques depuis le **Gestionnaire de bibliothèques** de l'IDE Arduino :
1. **`ArduinoJson`** (par Benoit Blanchon)
2. **`PubSubClient`** (par Nick O'Leary)
3. **`ArduinoGraphics`** (par Arduino)
*(Note : `WiFiS3` et `Arduino_LED_Matrix` sont incluses nativement avec le pack de cartes Arduino UNO R4).*

---

## 📂 Structure du projet

```text
uno-r4-api-weather-station/
├── src/
│   ├── src.ino                   # Code principal (HTTPS, JSON, Matrice LED, MQTT)
│   └── arduino_secrets.h.example # Modèle de configuration réseau
├── .gitignore                    # Exclut les identifiants sensibles de Git
└── README.md                     # Documentation du projet
```

---

## 🚀 Installation et Configuration

1. **Cloner le dépôt :**
   ```bash
   git clone https://github.com
   cd uno-r4-api-weather-station
   ```

2. **Configurer vos identifiants Wi-Fi :**
   * Dupliquez le fichier `src/arduino_secrets.h.example` et renommez-le en `src/arduino_secrets.h`.
   * Renseignez vos informations réseau :
     ```cpp
     #define SECRET_SSID "Votre_Nom_WiFi"
     #define SECRET_PASS "Votre_Mot_De_Passe_WiFi"
     #define MQTT_BROKER "://hivemq.com"
     ```

3. **Personnaliser votre localisation :**
   * Ouvrez `src/src.ino` et modifiez les coordonnées GPS correspondant à votre ville :
     ```cpp
     String latitude = "00.000";
     String longitude = "0.000";
     ```

4. **Téléverser :**
   * Sélectionnez la carte **Arduino UNO R4 WiFi** dans l'IDE Arduino et téléversez le programme.
   * *(Optionnel)* Si la connexion HTTPS échoue, mettez à jour le firmware radio de la carte via **Outils > Mise à jour du firmware** dans l'IDE Arduino.

---

## 📡 Format des données MQTT

Les relevés sont publiés toutes les 60 secondes sur le topic `unor4/meteo/live` au format JSON :

```json
{
  "temperature": 14.5,
  "humidity": 72,
  "is_day": 1,
  "weather_code": 2
}
```

Vous pouvez visualiser ce flux en direct à l'aide d'un client MQTT comme **MQTTX** ou **HiveMQ WebSocket Client** en vous abonnant au topic `unor4/meteo/live`.

---

## 📄 Licence

Ce projet est distribué sous licence **MIT**. Vous êtes libre de le réutiliser, le modifier et le partager.
