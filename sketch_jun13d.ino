#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <time.h>
#include <Adafruit_NeoPixel.h>

// --- Configuración WiFi ---
const char* ssid = "caliope";
const char* password = "sinlugar";

// --- Configuración de APIs ---
const char* geminiApiKey = "REEMPLAZA_CON_TU_API_KEY_DE_GEMINI";
String openMeteoUrl = "http://api.open-meteo.com/v1/forecast?latitude=-34.898&longitude=-54.952&daily=weather_code,precipitation_probability_mean&timezone=America/Montevideo";

// --- Arrays para 7 días de pronóstico ---
int dailyWeatherCodes[7];
int dailyPrecipitation[7];
const char* dayNames[7] = {"Domingo", "Lunes", "Martes", "Miercoles", "Jueves", "Viernes", "Sabado"};

// --- Variables de Estado para LCD ---
int currentDayOffset = 0;
bool showProbability = false;

// --- Componentes Hardware ---
ESP8266WebServer server(80);
LiquidCrystal_I2C lcd(0x27, 16, 2);
IRrecv irrecv(D7);
decode_results results;

// --- Configuración NTP ---
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -3 * 3600;
const int   daylightOffset_sec = 0;

// --- Definiciones de Pines para LEDs ---
const int ledPinSoleado = D0;
const int ledPinNublado = D3; // Pin original para Nublado
const int ledPinLluvia = D5;
const int ledPinTormenta = D6;
// El pin D8 ya no es un LED individual, ahora es para la matriz
const int ledPins[] = {ledPinSoleado, ledPinNublado, ledPinLluvia, ledPinTormenta};
const int numLeds = 4; // Ahora son 4 LEDs individuales

// --- Configuración de la Matriz y Tira NeoPixel ---
#define PIXEL_PIN D8
#define NUMPIXELS 72    // MODIFICADO: 64 (matriz 8x8) + 8 (tira 1x8) = 72
Adafruit_NeoPixel pixels(NUMPIXELS, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

// --- CÓDIGOS IR ---
#define IR_BUTTON_1 0xFF6897
#define IR_BUTTON_2 0xFF9867
#define IR_BUTTON_3 0xFFB04F
#define IR_BUTTON_4 0xFF30CF
#define IR_BUTTON_5 0xFF18E7
#define IR_BUTTON_6 0xFF7A85
#define IR_BUTTON_ARRIBA 0xFF629D
#define IR_BUTTON_ABAJO  0xFFA857

// --- Intervalo de actualización ---
unsigned long previousMillis = 0;
const long interval = 21600000;

// =================================================================
// --- FUNCIONES PARA LA MATRIZ NEOPIXEL ---
// =================================================================

void clearMatrix() {
  pixels.clear();
  pixels.show();
}

// MODIFICADO: Función para encender la matriz y la tira
void lightUpMatrixPartlyCloudy() {
  uint32_t white = pixels.Color(150, 150, 150);
  uint32_t yellow = pixels.Color(255, 255, 0);

  // Encender la matriz 8x8 (primeros 64 píxeles) en blanco
  for(int i=0; i < 64; i++) {
    pixels.setPixelColor(i, white);
  }

  // Encender la tira 1x8 (los siguientes 8 píxeles) en amarillo
  for(int i=64; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, yellow);
  }
  
  pixels.show();
}


// =================================================================
// --- FUNCIONES COMUNES Y DE OBTENCIÓN DE DATOS ---
// =================================================================

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
  if (WiFi.status() != WL_CONNECTED) { Serial.println("WiFi no conectado."); return; }
  WiFiClient client;
  HTTPClient http;
  if (http.begin(client, openMeteoUrl)) {
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      DynamicJsonDocument doc(4096);
      deserializeJson(doc, http.getString());
      JsonArray codes = doc["daily"]["weather_code"];
      JsonArray precipitations = doc["daily"]["precipitation_probability_mean"];
      for (int i = 0; i < 7; i++) {
        if (i < codes.size() && i < precipitations.size()) {
          dailyWeatherCodes[i] = codes[i];
          dailyPrecipitation[i] = precipitations[i];
        }
      }
      Serial.println("--- Pronostico de 7 dias actualizado ---");
    } else { Serial.printf("[HTTP] GET fallo, error: %s\n", http.errorToString(httpCode).c_str()); }
    http.end();
  }
}

String getGeminiExplanation(String prompt) {
  std::unique_ptr<WiFiClientSecure> client(new WiFiClientSecure);
  client->setInsecure();
  HTTPClient https;
  String url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash-latest:generateContent?key=" + String(geminiApiKey);
  if (https.begin(*client, url)) {
    https.addHeader("Content-Type", "application/json");
    DynamicJsonDocument requestBody(1024);
    JsonArray contentsArray = requestBody.createNestedArray("contents");
    JsonObject userContent = contentsArray.createNestedObject();
    JsonArray partsArray = userContent.createNestedArray("parts");
    JsonObject partObject = partsArray.createNestedObject();
    partObject["text"] = prompt;
    String requestBodyString;
    serializeJson(requestBody, requestBodyString);
    int httpCode = https.POST(requestBodyString);
    if (httpCode == HTTP_CODE_OK) {
      DynamicJsonDocument doc(2048);
      deserializeJson(doc, https.getString());
      const char* explanation = doc["candidates"][0]["content"]["parts"][0]["text"];
      https.end();
      return String(explanation);
    } else { Serial.printf("[HTTPS] POST de Gemini falló, código de error: %d\n", httpCode); https.end(); return "Error al contactar a Gemini."; }
  } else { return "Error de conexión con Gemini."; }
}

// =================================================================
// --- FUNCIONES PARA LCD / IR / LEDS Y MATRIZ ---
// =================================================================

void updateDisplaysForDay(int dayOffset) {
  currentDayOffset = dayOffset;
  int code = dailyWeatherCodes[dayOffset];
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  int forecastDayIndex = (timeinfo.tm_wday + dayOffset) % 7;

  // Actualizar Pantalla LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Dia: ");
  lcd.print(dayNames[forecastDayIndex]);
  lcd.setCursor(0, 1);
  if (showProbability) {
    lcd.print("Lluvia: " + String(dailyPrecipitation[dayOffset]) + "%");
  } else {
    lcd.print(getWeatherDescriptionString(code));
  }

  // Apagar todos los LEDs individuales y la matriz antes de encender lo correcto
  for(int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], LOW);
  }
  clearMatrix();

  // Actualizar LEDs y Matriz NeoPixel según el clima
  switch (code) {
    case 0:
      digitalWrite(ledPinSoleado, HIGH);
      break;
    case 1: case 2: // Clima parcialmente nublado
      lightUpMatrixPartlyCloudy(); // Enciende la matriz y la tira en D8
      break;
    case 3:
      digitalWrite(ledPinNublado, HIGH); // Enciende el LED en D3
      break;
    case 51 ... 82:
      digitalWrite(ledPinLluvia, HIGH);
      break;
    case 95 ... 99:
      digitalWrite(ledPinTormenta, HIGH);
      break;
    default:
      // Para cualquier otro clima, todo permanece apagado.
      break;
  }
}

// =================================================================
// --- FUNCIONES PARA SERVIDOR WEB ---
// =================================================================

void handleRoot() {
  Serial.println("Petición web recibida.");
  String weatherDesc = getWeatherDescriptionString(dailyWeatherCodes[0]);
  int precipProb = dailyPrecipitation[0];
  String geminiExplanation = "No se pudo obtener la explicación.";
  String prompt = "El pronostico del tiempo para hoy es " + weatherDesc + " con una probabilidad de lluvia del " + String(precipProb) + "%. ";
  prompt += "Explica un concepto simple de ciencia de datos relacionado con este pronostico, como si fueras un profesor muy amigable hablandole a un nino de 10 anos. Se breve, y no uses mas de 40 palabras.";
  geminiExplanation = getGeminiExplanation(prompt);
  String htmlPage = "<!DOCTYPE html><html lang='es'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  htmlPage += "<title>Clima con Gemini</title><style>";
  htmlPage += "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; background-color: #f0f8ff; color: #333; display: flex; justify-content: center; align-items: center; min-height: 100vh; margin: 20px; }";
  htmlPage += ".card { background-color: white; border-radius: 15px; box-shadow: 0 4px 12px rgba(0,0,0,0.1); padding: 25px; max-width: 400px; text-align: center; }";
  htmlPage += "h1 { color: #005a9c; } .weather { font-size: 1.5em; margin: 15px 0; } .gemini { background-color: #e6f7ff; border-left: 5px solid #1890ff; padding: 15px; margin-top: 20px; text-align: left; border-radius: 8px;}";
  htmlPage += "</style></head><body><div class='card'>";
  htmlPage += "<h1>Panel del Clima Inteligente</h1>";
  htmlPage += "<div class='weather'>Hoy: <b>" + weatherDesc + "</b></div>";
  htmlPage += "<div class='weather'>Prob. de Lluvia: <b>" + String(precipProb) + "%</b></div>";
  htmlPage += "<div class='gemini'><b>Dato curioso de Gemini:</b><p>" + geminiExplanation + "</p></div>";
  htmlPage += "</div></body></html>";
  server.send(200, "text/html", htmlPage);
}


// =================================================================
// --- SETUP Y LOOP PRINCIPALES ---
// =================================================================

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  irrecv.enableIRIn();
  
  // Inicialización de los LEDs individuales
  for(int i = 0; i < numLeds; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
  }
  
  // Inicialización de la matriz y tira NeoPixel
  pixels.begin();
  pixels.setBrightness(40);
  pixels.clear();
  pixels.show();

  lcd.print("Conectando WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); lcd.print("."); }
  
  Serial.println("\nWiFi conectado!");
  Serial.print("Direccion IP para la web: ");
  Serial.println(WiFi.localIP());

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  getWeatherData();
  updateDisplaysForDay(0);
  
  server.on("/", HTTP_GET, handleRoot);
  server.begin();
  Serial.println("Servidor HTTP iniciado.");
}

void loop() {
  server.handleClient();

  if (irrecv.decode(&results)) {
    if (results.value != 0xFFFFFFFFFFFFFFFF) {
      switch (results.value) {
        case IR_BUTTON_1: showProbability = false; updateDisplaysForDay(0); break;
        case IR_BUTTON_2: showProbability = false; updateDisplaysForDay(1); break;
        case IR_BUTTON_3: showProbability = false; updateDisplaysForDay(2); break;
        case IR_BUTTON_4: showProbability = false; updateDisplaysForDay(3); break;
        case IR_BUTTON_5: showProbability = false; updateDisplaysForDay(4); break;
        case IR_BUTTON_6: showProbability = false; updateDisplaysForDay(5); break;
        case IR_BUTTON_ABAJO: showProbability = true; updateDisplaysForDay(currentDayOffset); break;
        case IR_BUTTON_ARRIBA: showProbability = false; updateDisplaysForDay(currentDayOffset); break;
      }
    }
    irrecv.resume();
  }

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    getWeatherData();
  }
}
