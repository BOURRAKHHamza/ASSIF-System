#define BLYNK_TEMPLATE_ID "TMPLxxxxxx" // Remplacer par votre ID
#define BLYNK_TEMPLATE_NAME "ESP32C3"
#define BLYNK_AUTH_TOKEN "VOTRE_TOKEN_BLYNK_ICI"

#include <WiFi.h>
#include <WiFiMulti.h>
#include <BlynkSimpleEsp32.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "time.h"

// --- Définition des broches (ESP32-C3) ---
#define ONE_WIRE_BUS 5      // Sonde DS18B20
#define TDS_PIN 3           // Capteur TDS (ADC)
#define RELAY_UV_PIN 10     // Relais lampe UV
#define PUMP_PIN 7          // Transistor de la pompe
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// --- Objets ---
WiFiMulti wifiMulti;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
BlynkTimer timer;

// --- Variables globales ---
float temperature = 25.0;
float tdsValue = 0;
float ecValue = 0;
bool isSterilizing = false;
bool manualOverride = false; 

// Paramètres NTP (Heure internet pour le Maroc)
const char* ntpServer = "pool.ntp.org";
const char* time_zone = "WET0WEST,M3.5.0,M10.5.0/3"; 

void setup() {
  Serial.begin(115200);

  // Initialisation de la pompe (Éteinte par défaut)
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, LOW);     

  // Initialisation du relais (Astuce 3.3V : INPUT = relais éteint)
  pinMode(RELAY_UV_PIN, INPUT); 

  // Initialisation I2C pour l'écran OLED (SDA=8, SCL=9 sur C3)
  Wire.begin(8, 9);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Échec SSD1306"));
    for(;;);
  }
  display.clearDisplay();
  display.setTextColor(WHITE);
  
  // Initialisation DS18B20
  sensors.begin();

  // Configuration multi Wi-Fi
  Serial.println("Connexion au Wi-Fi...");
  wifiMulti.addAP("VOTRE_NOM_DE_WIFI_1", "VOTRE_MOT_DE_PASSE_1");
  wifiMulti.addAP("VOTRE_NOM_DE_WIFI_2", "VOTRE_MOT_DE_PASSE_2");
  wifiMulti.addAP("VOTRE_NOM_DE_WIFI_3", "VOTRE_MOT_DE_PASSE_3");

  while (wifiMulti.run() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connecté !");
  
  // Configuration de l'heure NTP
  configTzTime(time_zone, ntpServer);

  // Configuration Blynk
  Blynk.config(BLYNK_AUTH_TOKEN);

  // Minuteries
  timer.setInterval(2000L, readSensors); 
  timer.setInterval(2000L, updateScreen); 
  timer.setInterval(5000L, sendToBlynk);  
  timer.setInterval(60000L, checkSchedule); 
}

void loop() {
  if (wifiMulti.run() == WL_CONNECTED) {
    Blynk.run();
  }
  timer.run();
}

// --- Fonction de lecture des capteurs ---
void readSensors() {
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);
  if (tempC != DEVICE_DISCONNECTED_C) {
    temperature = tempC;
  }

  int analogValue = analogRead(TDS_PIN);
  float voltage = analogValue * (3.3 / 4095.0); 
  
  float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
  float compensationVoltage = voltage / compensationCoefficient;
  
  tdsValue = (133.42 * pow(compensationVoltage, 3) - 255.86 * pow(compensationVoltage, 2) + 857.39 * compensationVoltage) * 0.5;
  ecValue = tdsValue * 2.0; 
}

// --- Fonction d'affichage OLED ---
void updateScreen() {
  display.clearDisplay();
  
  display.setTextSize(1);
  display.setCursor(0, 0);
  if (wifiMulti.run() == WL_CONNECTED) {
    display.print("WiFi: OK");
  } else {
    display.print("WiFi: OFF");
  }

  display.setCursor(0, 15);
  display.setTextSize(2);
  display.print("T: ");
  display.print(temperature, 1);
  display.print(" C");

  display.setCursor(0, 35);
  display.print("E:");
  display.print((int)ecValue); 
  display.setTextSize(1);
  display.print(" uS/cm");

  display.setCursor(0, 55);
  display.print("UV/Pompe: ");
  display.print(isSterilizing ? "ACTIVE" : "REPOS");

  display.display();
}

// --- Fonction d'envoi vers Blynk ---
void sendToBlynk() {
  if (wifiMulti.run() == WL_CONNECTED) {
    Blynk.virtualWrite(V1, temperature);
    Blynk.virtualWrite(V2, ecValue); 
    Blynk.virtualWrite(V3, isSterilizing ? 1 : 0); 
  }
}

// --- Fonction de minuterie (Stérilisation quotidienne) ---
void checkSchedule() {
  if (manualOverride == true) {
    return; // Annule l'auto si manuel activé
  }

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Erreur de récupération de l'heure");
    return;
  }

  int currentHour = timeinfo.tm_hour;
  int currentMin = timeinfo.tm_min;

  if (currentHour == 14 && currentMin < 30) {
    if (!isSterilizing) {
      // ON : On passe en OUTPUT et on active (LOW)
      pinMode(RELAY_UV_PIN, OUTPUT);
      digitalWrite(RELAY_UV_PIN, LOW);
      digitalWrite(PUMP_PIN, HIGH); // Pompe ON
      isSterilizing = true;
      Serial.println("Début du cycle de stérilisation.");
    }
  } else {
    if (isSterilizing) {
      // OFF : On passe en INPUT (Haute impédance) pour bloquer le courant
      pinMode(RELAY_UV_PIN, INPUT);
      digitalWrite(PUMP_PIN, LOW); // Pompe OFF
      isSterilizing = false;
      Serial.println("Fin du cycle de stérilisation.");
    }
  }
}

// --- Fonction exécutée quand on appuie sur le bouton Blynk (V4) ---
BLYNK_WRITE(V4) {
  int buttonState = param.asInt(); 
  
  if (buttonState == 1) {
    // ON MANUEL : On passe en OUTPUT et on active (LOW)
    pinMode(RELAY_UV_PIN, OUTPUT);
    digitalWrite(RELAY_UV_PIN, LOW);
    digitalWrite(PUMP_PIN, HIGH); // Pompe ON
    isSterilizing = true;
    manualOverride = true; 
    Serial.println("Activation manuelle via Blynk !");
  } else {
    // OFF MANUEL : On passe en INPUT pour bloquer le courant
    pinMode(RELAY_UV_PIN, INPUT);
    digitalWrite(PUMP_PIN, LOW); // Pompe OFF
    isSterilizing = false;
    manualOverride = false; 
    Serial.println("Arrêt manuel via Blynk.");
  }
}