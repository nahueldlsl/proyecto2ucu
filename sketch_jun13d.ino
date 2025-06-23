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
// NUEVO: Se añade "&daily=weather_code,precipitation_probability_mean" para obtener los códigos de clima y la probabilidad de lluvia.
String openMeteoUrl = "http://api.open-meteo.com/v1/forecast?latitude=-34.898&longitude=-54.952&daily=weather_code,precipitation_probability_mean&timezone=America/Montevideo";

// --- Arrays para 7 días de pronóstico ---
int dailyWeatherCodes[7];
int dailyPrecipitation[7]; // NUEVO: Array para guardar la probabilidad de lluvia de cada día.
const char* dayNames[7] = {"Domingo", "Lunes", "Martes", "Miercoles", "Jueves", "Viernes", "Sabado"};

// --- Variables de Estado ---
int currentDayOffset = 0;   // NUEVO: Guarda el último día que se seleccionó (0=hoy, 1=mañana, etc.).
bool showProbability = false; // NUEVO: Controla si se muestra la descripción del clima o la probabilidad de lluvia.

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
const int ledPinLluvia = D5;
const int ledPinTormenta = D6;
const int ledPins[] = {ledPinSoleado, ledPinParcialmente, ledPinNublado, ledPinLluvia, ledPinTormenta};
const int numLeds = 5;

// --- Definiciones de Pines para IR ---
const int irRecvPin = D7;
IRrecv irrecv(irRecvPin);
decode_results results;

// --- CÓDIGOS IR DE EJEMPLO (¡DEBES CAMBIARLOS POR LOS DE TU CONTROL!) ---
#define IR_BUTTON_1 0xFF6897 // Botón "1"
#define IR_BUTTON_2 0xFF9867 // Botón "2"
#define IR_BUTTON_3 0xFFB04F // Botón "3"
#define IR_BUTTON_4 0xFF30CF // Botón "4"
#define IR_BUTTON_5 0xFF18E7 // Botón "5"
#define IR_BUTTON_6 0xFF7A85 // Botón "6"
// NUEVO: Códigos para los botones de navegación vertical.
#define IR_BUTTON_ARRIBA 0xFF629D // Reemplaza con el código de tu botón "arriba" o "CH+"
#define IR_BUTTON_ABAJO  0xFFA857 // Reemplaza con el código de tu botón "abajo" o "CH-"

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

  for (int i = 0; i < numLeds; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
  }
  pinMode(D4, OUTPUT); // Pin D4 (antes Niebla) se mantiene como salida para apagarlo.
  digitalWrite(D4, LOW);

  for (int i = 0; i < 7; i++) {
    dailyWeatherCodes[i] = -1;
    dailyPrecipitation[i] = -1; // NUEVO: Inicializar el array de precipitación.
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
    if (results.value != 0xFFFFFFFFFFFFFFFF) { // Ignorar códigos de repetición
      Serial.print("Código IR recibido: 0x");
      Serial.println(results.value, HEX);
      
      switch (results.value) {
        // Al presionar un día, se muestra la descripción del clima por defecto.
        case IR_BUTTON_1: showProbability = false; updateDisplaysForDay(0); break;
        case IR_BUTTON_2: showProbability = false; updateDisplaysForDay(1); break;
        case IR_BUTTON_3: showProbability = false; updateDisplaysForDay(2); break;
        case IR_BUTTON_4: showProbability = false; updateDisplaysForDay(3); break;
        case IR_BUTTON_5: showProbability = false; updateDisplaysForDay(4); break;
        case IR_BUTTON_6: showProbability = false; updateDisplaysForDay(5); break;

        // NUEVO: Casos para los nuevos botones.
        case IR_BUTTON_ABAJO:
          showProbability = true;      // Cambia el modo a "mostrar probabilidad".
          updateDisplaysForDay(currentDayOffset); // Refresca la pantalla para el día actual.
          break;
        case IR_BUTTON_ARRIBA:
          showProbability = false;     // Cambia el modo a "mostrar descripción".
          updateDisplaysForDay(currentDayOffset); // Refresca la pantalla para el día actual.
          break;
      }
    }
    irrecv.resume();
  }
}

void updateDisplaysForDay(int dayOffset) {
  // NUEVO: Actualizamos la variable global que recuerda el día seleccionado.
  currentDayOffset = dayOffset;
  
  // Apagar todos los LEDs
  for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], LOW);
  }
  digitalWrite(D4, LOW);

  if (dayOffset < 0 || dayOffset > 6) return;
  
  int code = dailyWeatherCodes[dayOffset];
  int precip = dailyPrecipitation[dayOffset]; // NUEVO: Obtener la probabilidad de lluvia.
  
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    return;
  }
  int currentDayOfWeek = timeinfo.tm_wday;
  int forecastDayIndex = (currentDayOfWeek + dayOffset) % 7;
  
  Serial.printf("Offset: %d. Dia: %s. Clima: %d. Prob. Lluvia: %d%%\n", dayOffset, dayNames[forecastDayIndex], code, precip);

  // --- Actualizar Pantalla LCD ---
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Dia: ");
  lcd.print(dayNames[forecastDayIndex]);
  
  lcd.setCursor(0, 1);
  // NUEVO: Lógica para decidir qué mostrar en la segunda línea del LCD.
  if (showProbability) {
    String probStr = "Lluvia: " + String(precip) + "%";
    lcd.print(probStr);
  } else {
    lcd.print(getWeatherDescriptionString(code));
  }

  // --- Actualizar LED (esta lógica no cambia) ---
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
      // Para códigos no definidos (como Niebla), no se enciende ningún LED.
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

    Serial.println("Obteniendo datos del clima...");
    if (http.begin(client, openMeteoUrl)) {
      int httpCode = http.GET();
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        DynamicJsonDocument doc(4096); // Aumentar tamaño por si acaso
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
          // NUEVO: Se verifica que existan los nuevos datos en la respuesta JSON.
          if (doc.containsKey("daily") && doc["daily"].containsKey("time") && doc["daily"].containsKey("weather_code") && doc["daily"].containsKey("precipitation_probability_mean")) {
            JsonArray codes = doc["daily"]["weather_code"];
            JsonArray precipitations = doc["daily"]["precipitation_probability_mean"]; // NUEVO: Se extrae el array de probabilidades.
            
            for (int i = 0; i < 7; i++) {
                if (i < codes.size() && i < precipitations.size()) {
                    dailyWeatherCodes[i] = codes[i];
                    dailyPrecipitation[i] = precipitations[i]; // NUEVO: Se guarda la probabilidad.
                }
            }
            Serial.println("\n--- Pronostico de 7 dias actualizado ---");
            for (int i = 0; i < 7; i++) {
              Serial.printf("Dia %d: Codigo %d (%s), Prob. Lluvia: %d%%\n", i, dailyWeatherCodes[i], getWeatherDescriptionString(dailyWeatherCodes[i]).c_str(), dailyPrecipitation[i]);
            }
            Serial.println("----------------------------------------");
            
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Pronostico OK!");
            lcd.setCursor(0, 1);
            lcd.print("Usa el control");
          } else {
              Serial.println("Error: Faltan datos en la respuesta JSON de la API.");
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
  } else {
      Serial.println("WiFi no conectado. No se pueden obtener datos.");
  }
}
