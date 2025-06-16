#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <Wire.h> // Librería para comunicación I2C
#include <LiquidCrystal_I2C.h> // Librería para la pantalla LCD

// --- Configuración WiFi ---
const char* ssid = "caliope";
const char* password = "sinlugar";

// --- Configuración Open-Meteo ---
String openMeteoUrl = "http://api.open-meteo.com/v1/forecast?latitude=-34.898&longitude=-54.952&hourly=weather_code&timezone=America/Montevideo";

// --- Array para 7 días de pronóstico ---
int dailyWeatherCodes[7];
const char* dayNames[7] = {"Hoy", "Manana", "En 2 dias", "En 3 dias", "En 4 dias", "En 5 dias", "En 6 dias"};

// --- Configuración de la Pantalla LCD ---
// La dirección I2C suele ser 0x27 o 0x3F. Si no funciona,
// puedes usar un "I2C Scanner Sketch" para encontrar la dirección correcta.
LiquidCrystal_I2C lcd(0x27, 16, 2); // Dirección 0x27, 16 caracteres, 2 filas

// --- CAMBIO DE PINES PARA LEDS ---
// D1 y D2 ahora se usan para la pantalla LCD (I2C)
const int ledPinSoleado = D0;      // Movido de D1
const int ledPinParcialmente = D8; // Movido de D2
const int ledPinNublado = D3;      // Se mantiene
const int ledPinNiebla = D4;       // Se mantiene
const int ledPinLluvia = D5;       // Se mantiene
const int ledPinTormenta = D6;     // Se mantiene

const int ledPins[] = {ledPinSoleado, ledPinParcialmente, ledPinNublado, ledPinNiebla, ledPinLluvia, ledPinTormenta};
const int numLeds = 6;

// --- Definiciones de Pines para IR ---
const int irRecvPin = D7;
IRrecv irrecv(irRecvPin);
decode_results results;

// --- CÓDIGOS IR DE EJEMPLO (¡DEBES CAMBIARLOS POR LOS DE TU CONTROL!) ---
#define IR_BUTTON_1 0xFF30CF
#define IR_BUTTON_2 0xFF18E7
#define IR_BUTTON_3 0xFF7A85
#define IR_BUTTON_4 0xFF10EF
#define IR_BUTTON_5 0xFF38C7
#define IR_BUTTON_6 0xFF5AA5
#define IR_BUTTON_7 0xFF42BD

// --- Intervalo de actualización de clima ---
unsigned long previousMillis = 0;
const long interval = 21600000; // 6 horas

void setup() {
  Serial.begin(115200);
  delay(100);

  // Inicializar Pantalla LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Panel del Clima");
  lcd.setCursor(0, 1);
  lcd.print("Iniciando...");
  Serial.println("Pantalla LCD iniciada.");

  // Inicializar receptor IR
  irrecv.enableIRIn(); 
  Serial.println("Receptor IR iniciado.");

  // Inicializar LEDs
  for (int i = 0; i < numLeds; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
  }
  Serial.println("LEDs configurados.");

  // Inicializar array de climas
  for (int i = 0; i < 7; i++) {
    dailyWeatherCodes[i] = -1;
  }

  // Conectar a WiFi
  Serial.println("\nConectando a WiFi...");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Conectando WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    lcd.print(".");
  }
  Serial.println("\nWiFi conectado!");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());

  // Obtener el clima por primera vez
  getWeatherData();
}

void loop() {
  // Actualizar el clima periódicamente
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    getWeatherData();
  }

  // Escuchar comandos del control remoto
  if (irrecv.decode(&results)) {
    if (results.value != 0xFFFFFFFFFFFFFFFF) {
        Serial.print("Código IR recibido: 0x");
        Serial.println(results.value, HEX);

        switch (results.value) {
          case IR_BUTTON_1: updateDisplaysForDay(0); break;
          case IR_BUTTON_2: updateDisplaysForDay(1); break;
          case IR_BUTTON_3: updateDisplaysForDay(2); break;
          case IR_BUTTON_4: updateDisplaysForDay(3); break;
          case IR_BUTTON_5: updateDisplaysForDay(4); break;
          case IR_BUTTON_6: updateDisplaysForDay(5); break;
          case IR_BUTTON_7: updateDisplaysForDay(6); break;
        }
    }
    irrecv.resume();
  }
}

void updateDisplaysForDay(int dayIndex) {
  // Apagar todos los LEDs
  for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], LOW);
  }

  if (dayIndex < 0 || dayIndex > 6) return;
  
  int code = dailyWeatherCodes[dayIndex];
  Serial.printf("Botón presionado para Día %d. Código de clima: %d\n", dayIndex, code);

  // 1. Actualizar Pantalla LCD
  lcd.clear();
  lcd.setCursor(0, 0); // Fila 1
  lcd.print("Dia: ");
  lcd.print(dayNames[dayIndex]);
  
  lcd.setCursor(0, 1); // Fila 2
  lcd.print("Clima: ");
  lcd.print(getWeatherDescriptionString(code));

  // 2. Actualizar LED
  switch (code) {
    case 0: digitalWrite(ledPinSoleado, HIGH); break;
    case 1:
    case 2: digitalWrite(ledPinParcialmente, HIGH); break;
    case 3: digitalWrite(ledPinNublado, HIGH); break;
    case 45:
    case 48: digitalWrite(ledPinNiebla, HIGH); break;
    case 51: case 53: case 55:
    case 61: case 63: case 65:
    case 80: case 81: case 82: digitalWrite(ledPinLluvia, HIGH); break;
    case 95: case 96: case 99: digitalWrite(ledPinTormenta, HIGH); break;
    default: break;
  }
}

String getWeatherDescriptionString(int code) {
  switch (code) {
    case 0: return "Soleado";
    case 1: return "Principalmente";
    case 2: return "Parc. Nublado";
    case 3: return "Nublado";
    case 45:
    case 48: return "Niebla";
    case 51:
    case 53:
    case 55: return "Llovizna";
    case 61:
    case 63:
    case 65: return "Lluvia";
    case 80: case 81: case 82: return "Chubascos";
    case 95: case 96: case 99: return "Tormenta";
    default: return "Desconocido";
  }
}

void getWeatherData() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;

    Serial.println("\n[HTTP] Iniciando solicitud horaria a Open-Meteo...");
    if (http.begin(client, openMeteoUrl)) {
      int httpCode = http.GET();
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        DynamicJsonDocument doc(16384);
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
          if (doc.containsKey("hourly") && doc["hourly"].containsKey("time") && doc["hourly"].containsKey("weather_code")) {
            JsonArray timestamps = doc["hourly"]["time"];
            JsonArray codes = doc["hourly"]["weather_code"];
            int dayIndex = 0;

            for (int i = 0; i < timestamps.size(); i++) {
              String timestamp = timestamps[i];
              if (timestamp.endsWith("T12:00")) {
                if (dayIndex < 7) {
                  dailyWeatherCodes[dayIndex] = codes[i];
                  dayIndex++;
                } else {
                  break;
                }
              }
            }
            Serial.println("\n--- Pronostico de 7 dias actualizado ---");
            for (int i = 0; i < 7; i++) {
              Serial.printf("Dia %d: Codigo %d -> ", i, dailyWeatherCodes[i]);
              printWeatherDescription(dailyWeatherCodes[i]);
            }
            Serial.println("----------------------------------------");
            
            // Mostrar mensaje de listo en el LCD
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Pronostico OK!");
            lcd.setCursor(0, 1);
            lcd.print("Usa el control");

          }
        }
      } else {
        Serial.printf("[HTTP] Solicitud GET fallo, error: %s\n", http.errorToString(httpCode).c_str());
      }
      http.end();
    }
  }
}

void printWeatherDescription(int code) {
  // Esta función ahora es solo para el Monitor Serie
  Serial.println(getWeatherDescriptionString(code));
}