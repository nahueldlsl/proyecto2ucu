#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <time.h>

// --- Configuración WiFi ---
const char* ssid = "caliope";
const char* password = "sinlugar";

// --- Configuración Open-Meteo ---
String openMeteoUrl = "http://api.open-meteo.com/v1/forecast?latitude=-34.898&longitude=-54.952&hourly=weather_code&timezone=America/Montevideo";

// --- Array para 7 días de pronóstico ---
int dailyWeatherCodes[7];
const char* dayNames[7] = {"Domingo", "Lunes", "Martes", "Miercoles", "Jueves", "Viernes", "Sabado"};

// --- Configuración NTP (Servidor de Hora) ---
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -3 * 3600;
const int   daylightOffset_sec = 0;

// --- Configuración de la Pantalla LCD ---
LiquidCrystal_I2C lcd(0x27, 16, 2);

// --- Definiciones de Pines para LEDs (5 LEDs) ---
const int ledPinSoleado = D0;
const int ledPinParcialmente = D8;
const int ledPinNublado = D3;
// const int ledPinNiebla = D4; // Pin para Niebla eliminado
const int ledPinLluvia = D5;
const int ledPinTormenta = D6;
// Se actualiza el array de LEDs para reflejar el cambio.
const int ledPins[] = {ledPinSoleado, ledPinParcialmente, ledPinNublado, ledPinLluvia, ledPinTormenta};
const int numLeds = 5; // Ahora son 5 LEDs activos

// --- Definiciones de Pines para IR ---
const int irRecvPin = D7;
IRrecv irrecv(irRecvPin);
decode_results results;

// --- CÓDIGOS IR DE EJEMPLO ---
#define IR_BUTTON_1 0xFF30CF
#define IR_BUTTON_2 0xFF18E7
#define IR_BUTTON_3 0xFF7A85
#define IR_BUTTON_4 0xFF10EF
#define IR_BUTTON_5 0xFF38C7
#define IR_BUTTON_6 0xFF5AA5

// --- Intervalo de actualización de clima ---
unsigned long previousMillis = 0;
const long interval = 21600000; // 6 horas

void setup() {
  Serial.begin(115200);
  delay(100);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Panel del Clima");
  lcd.setCursor(0, 1);
  lcd.print("Iniciando...");
  
  irrecv.enableIRIn();
  // Se ajusta el bucle para el nuevo número de LEDs
  for (int i = 0; i < numLeds; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
  }
  // El pin D4 que era del LED de niebla ahora no se usa para esto.
  pinMode(D4, OUTPUT);
  digitalWrite(D4, LOW);

  for (int i = 0; i < 7; i++) {
    dailyWeatherCodes[i] = -1;
  }

  // Conexión a WiFi
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Conectando WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    lcd.print(".");
  }
  
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("\nWiFi conectado y hora sincronizada!");
  
  getWeatherData();
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    getWeatherData();
  }

  if (irrecv.decode(&results)) {
    if (results.value != 0xFFFFFFFFFFFFFFFF) {
        switch (results.value) {
          case IR_BUTTON_1: updateDisplaysForDay(0); break;
          case IR_BUTTON_2: updateDisplaysForDay(1); break;
          case IR_BUTTON_3: updateDisplaysForDay(2); break;
          case IR_BUTTON_4: updateDisplaysForDay(3); break;
          case IR_BUTTON_5: updateDisplaysForDay(4); break;
          case IR_BUTTON_6: updateDisplaysForDay(5); break;
        }
    }
    irrecv.resume();
  }
}

void updateDisplaysForDay(int dayOffset) {
  // Apagar todos los LEDs
  for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], LOW);
  }
  digitalWrite(D4, LOW); // Apagar también el pin del LED que ya no se usa.

  if (dayOffset < 0 || dayOffset > 6) return;
  
  int code = dailyWeatherCodes[dayOffset];
  
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    return;
  }
  int currentDayOfWeek = timeinfo.tm_wday;
  int forecastDayIndex = (currentDayOfWeek + dayOffset) % 7;
  
  Serial.printf("Boton presionado para offset %d. Clima: %d. Dia: %s\n", dayOffset, code, dayNames[forecastDayIndex]);

  // Actualizar Pantalla LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Dia: ");
  lcd.print(dayNames[forecastDayIndex]);
  
  lcd.setCursor(0, 1);
  lcd.print("Clima: ");
  lcd.print(getWeatherDescriptionString(code));

  // Actualizar LED
  switch (code) {
    case 0:
      digitalWrite(ledPinSoleado, HIGH);
      break;
    case 1:
    case 2:
      digitalWrite(ledPinParcialmente, HIGH);
      break;
    case 3:
      digitalWrite(ledPinNublado, HIGH);
      break;
    // --- LÓGICA PARA NIEBLA ELIMINADA ---
    // case 45:
    // case 48:
    //   digitalWrite(ledPinNiebla, HIGH);
    //   break;
    case 51: case 53: case 55:
    case 61: case 63: case 65:
    case 80: case 81: case 82:
      digitalWrite(ledPinLluvia, HIGH);
      break;
    case 95:
    case 96:
    case 99:
      digitalWrite(ledPinTormenta, HIGH);
      break;
    default:
      // Para códigos no definidos (como Niebla ahora), no se enciende ningún LED.
      break;
  }
}

String getWeatherDescriptionString(int code) {
  switch (code) {
    case 0: return "Soleado";
    case 1: return "Prin. Nublado";
    case 2: return "Parc. Nublado";
    case 3: return "Nublado";
    case 45: case 48: return "Niebla";
    case 51: case 53: case 55: return "Llovizna";
    case 61: case 63: case 65: return "Lluvia";
    case 80: case 81: case 82: return "Chubascos";
    case 95: case 96: case 99: return "Tormenta";
    default: return "No disponible";
  }
}

void getWeatherData() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;

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
              if (String(timestamps[i]).endsWith("T12:00")) {
                if (dayIndex < 7) {
                  dailyWeatherCodes[dayIndex] = codes[i];
                  dayIndex++;
                } else { break; }
              }
            }
            Serial.println("\n--- Pronostico de 7 dias actualizado ---");
            for (int i = 0; i < 7; i++) {
              Serial.printf("Dia %d (offset): Codigo %d -> ", i, dailyWeatherCodes[i]);
              printWeatherDescription(dailyWeatherCodes[i]);
            }
            Serial.println("----------------------------------------");
            
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Pronostico OK!");
            lcd.setCursor(0, 1);
            lcd.print("Usa el control");
          }
        } else {
            Serial.print(F("deserializeJson() falló: "));
            Serial.println(error.f_str());
        }
      } else {
        Serial.printf("[HTTP] GET fallo, error: %s\n", http.errorToString(httpCode).c_str());
      }
      http.end();
    }
  }
}

void printWeatherDescription(int code) {
  Serial.println(getWeatherDescriptionString(code));
}
