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
const char* geminiApiKey = "AIzaSyCryFNg3fM7CFzQ6dKsAU-qbZ8DkNPA1-E";

// --- Estructura para Ciudades ---
struct City {
  const char* name;
  const char* latitude;
  const char* longitude;
  const char* timezone;
  const long gmtOffset_sec;
};

// --- Array de Ciudades ---
City cities[] = {
  {"Maldonado", "-34.898", "-54.952", "America/Montevideo", -3 * 3600},
  {"Paris", "48.8566", "2.3522", "Europe/Paris", 2 * 3600},
  {"London", "51.5072", "-0.1276", "Europe/London", 1 * 3600},
  {"Tokyo", "35.6895", "139.6917", "Asia/Tokyo", 9 * 3600}
};
const int numCities = sizeof(cities) / sizeof(cities[0]);
int currentCityIndex = 0;

// --- Arrays para 7 días de pronóstico ---
int dailyWeatherCodes[7];
int dailyPrecipitation[7];
const char* dayNames[7] = {"Domingo", "Lunes", "Martes", "Miercoles", "Jueves", "Viernes", "Sabado"};
String geminiExplanations[7]; // Caché para las respuestas de Gemini

// --- Variables de Estado Globales ---
int currentDayOffset = 0;
bool showProbability = false;
bool isShowingDashboard = false;
unsigned long lastIrInteractionTime = 0;
unsigned long cityChangeTimestamp = 0; // Para el display temporal de la ciudad

// --- Componentes Hardware ---
ESP8266WebServer server(80);
LiquidCrystal_I2C lcd(0x27, 16, 2);
IRrecv irrecv(D7);
decode_results results;

// --- OPTIMIZACIÓN DE MEMORIA ---
WiFiClientSecure client;
HTTPClient https;

// --- Configuración NTP ---
const char* ntpServer = "pool.ntp.org";
const long daylightOffset_sec = 0;

// --- Configuración de las 5 Matrices NeoPixel ---
#define NUM_PIXELS_STD_MATRIX 64
#define NUM_PIXELS_PARTLY_CLOUDY 72
#define PIN_SUN D0
#define PIN_PARTLY_CLOUDY D8
#define PIN_CLOUDY D3
#define PIN_RAIN D5
#define PIN_STORM D6

Adafruit_NeoPixel pixelsSun(NUM_PIXELS_STD_MATRIX, PIN_SUN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixelsPartlyCloudy(NUM_PIXELS_PARTLY_CLOUDY, PIN_PARTLY_CLOUDY, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixelsCloudy(NUM_PIXELS_STD_MATRIX, PIN_CLOUDY, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixelsRain(NUM_PIXELS_STD_MATRIX, PIN_RAIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixelsStorm(NUM_PIXELS_STD_MATRIX, PIN_STORM, NEO_GRB + NEO_KHZ800);

Adafruit_NeoPixel* allPixels[] = {&pixelsSun, &pixelsPartlyCloudy, &pixelsCloudy, &pixelsRain, &pixelsStorm};
const int numMatrices = 5;

// --- CÓDIGOS IR ---
#define IR_BUTTON_1 0xFF6897
#define IR_BUTTON_2 0xFF9867
#define IR_BUTTON_3 0xFFB04F
#define IR_BUTTON_4 0xFF30CF
#define IR_BUTTON_5 0xFF18E7
#define IR_BUTTON_6 0xFF7A85
#define IR_BUTTON_ARRIBA 0xFF629D
#define IR_BUTTON_ABAJO  0xFFA857
#define IR_BUTTON_LEFT 0xFF22DD
#define IR_BUTTON_RIGHT 0xFFC23D


// --- Intervalo de actualización ---
unsigned long previousMillis = 0;
const long interval = 21600000;

void changeCity(); // Declaración adelantada de la función

// =================================================================
// --- FUNCIONES PARA ILUMINAR LAS MATRICES Y OBTENCIÓN DE DATOS ---
// =================================================================
void clearAllMatrices() { for (int i=0; i < numMatrices; i++) { allPixels[i]->clear(); allPixels[i]->show(); } }
void lightUpMatrix(Adafruit_NeoPixel &pixels, uint32_t color) { for (int i = 0; i < pixels.numPixels(); i++) { pixels.setPixelColor(i, color); } pixels.show(); }
void lightUpPartlyCloudy() {
  uint32_t white = pixelsPartlyCloudy.Color(150, 150, 150);
  uint32_t yellow = pixelsPartlyCloudy.Color(255, 255, 0);
  for (int i = 0; i < 64; i++) { pixelsPartlyCloudy.setPixelColor(i, white); }
  for (int i = 64; i < 72; i++) { pixelsPartlyCloudy.setPixelColor(i, yellow); }
  pixelsPartlyCloudy.show();
}
String getWeatherDescriptionString(int code) { switch (code) { case 0: return "Soleado"; case 1: return "Prin. Nublado"; case 2: return "Parc. Nublado"; case 3: return "Nublado"; case 45: case 48: return "Niebla"; case 51: case 53: case 55: return "Llovizna"; case 61: case 63: case 65: return "Lluvia"; case 80: case 81: case 82: return "Chubascos"; case 95: case 96: case 99: return "Tormenta"; default: return "No disponible"; } }
String getFullWeatherDescriptionString(int code) { switch (code) { case 0: return "Soleado"; case 1: return "Principalmente Nublado"; case 2: return "Parcialmente Nublado"; case 3: return "Nublado"; case 45: case 48: return "con Niebla"; case 51: case 53: case 55: return "con Llovizna"; case 61: case 63: case 65: return "con Lluvia"; case 80: case 81: case 82: return "con Chubascos"; case 95: case 96: case 99: return "con Tormenta"; default: return "indefinido"; } }

void getWeatherData() {
  if (WiFi.status() != WL_CONNECTED) { Serial.println("WiFi no conectado."); return; }
  
  String url = "http://api.open-meteo.com/v1/forecast?latitude=" + String(cities[currentCityIndex].latitude);
  url += "&longitude=" + String(cities[currentCityIndex].longitude);
  url += "&daily=weather_code,precipitation_probability_mean&timezone=" + String(cities[currentCityIndex].timezone);
  
  WiFiClient localClient;
  HTTPClient http;
  if (http.begin(localClient, url)) {
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
          geminiExplanations[i] = ""; // Limpiar la caché al obtener nuevos datos
        }
      }
      Serial.println("--- Pronostico actualizado para " + String(cities[currentCityIndex].name) + " ---");
    } else { Serial.printf("[HTTP] GET fallo, error: %s\n", http.errorToString(httpCode).c_str()); }
    http.end();
  }
}

String getGeminiExplanation(String prompt) {
  client.setInsecure(); 
  String url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash-latest:generateContent?key=" + String(geminiApiKey);
  if (https.connected()) https.end();
  if (https.begin(client, url)) {
    https.addHeader("Content-Type", "application/json");
    DynamicJsonDocument requestBody(512); 
    requestBody.createNestedArray("contents").createNestedObject().createNestedArray("parts").createNestedObject()["text"] = prompt;
    String requestBodyString;
    serializeJson(requestBody, requestBodyString);
    int httpCode = https.POST(requestBodyString);
    if (httpCode == HTTP_CODE_OK) {
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, https.getString());
      const char* explanation = doc["candidates"][0]["content"]["parts"][0]["text"];
      https.end();
      return String(explanation);
    } else { 
      Serial.printf("[HTTPS] POST de Gemini falló, código de error: %d (%s)\n", httpCode, https.errorToString(httpCode).c_str()); 
      https.end(); 
      return "Error al contactar a Gemini."; 
    }
  } else { return "Error de conexión con Gemini."; }
}

// =================================================================
// --- FUNCIONES PARA LCD / IR Y MATRICES ---
// =================================================================
void showWelcomeDisplay() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Pulse un boton");
  lcd.setCursor(0, 1);
  lcd.print("del 1 al 6");
  clearAllMatrices(); // Apaga todas las matrices
}

void updateDisplaysForDay(int dayOffset) {
  currentDayOffset = dayOffset;
  int code = dailyWeatherCodes[dayOffset];
  
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  
  int forecastDayIndex = (timeinfo.tm_wday + dayOffset) % 7;
  
  // Lógica para mostrar "Hoy" y "Mañana"
  String displayName;
  if (dayOffset == 0) {
    displayName = "Hoy";
  } else if (dayOffset == 1) {
    displayName = "Manana";
  } else {
    displayName = dayNames[forecastDayIndex];
  }
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(displayName);
  
  lcd.setCursor(0, 1);
  if (showProbability) {
    lcd.print("Lluvia: " + String(dailyPrecipitation[dayOffset]) + "%");
  } else {
    lcd.print(getWeatherDescriptionString(code));
  }
  
  clearAllMatrices();
  switch (code) {
    case 0: lightUpMatrix(pixelsSun, pixelsSun.Color(255, 255, 0)); break;
    case 1: case 2: lightUpPartlyCloudy(); break;
    case 3: lightUpMatrix(pixelsCloudy, pixelsCloudy.Color(150, 150, 150)); break;
    case 51 ... 82: lightUpMatrix(pixelsRain, pixelsRain.Color(0, 0, 255)); break;
    case 95 ... 99: lightUpMatrix(pixelsStorm, pixelsStorm.Color(255, 0, 255)); break;
    default: break;
  }
}

// =================================================================
// --- FUNCIONES PARA SERVIDOR WEB (NUEVA ARQUITECTURA) ---
// =================================================================

void handleRoot() {
  String html = "<!DOCTYPE html><html lang='es'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>Panel del Clima</title>";
  html += "<style>body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,Helvetica,Arial,sans-serif;margin:0;transition:background-color .5s;}";
  html += ".screen{display:flex;justify-content:center;align-items:center;min-height:100vh;text-align:center;width:100%;}";
  html += "#welcome-screen{background-color:#1a1a1a;color:white;}";
  html += "#dashboard-screen{background-color:#f0f8ff;color:#333;flex-direction:column;}";
  html += ".hidden{display:none;}";
  html += ".welcome-container{max-width:600px;}.title{font-size:3em;margin-bottom:20px;}.subtitle{font-size:1.2em;color:#aaa;margin-bottom:40px;}";
  html += ".card{background-color:white;border-radius:15px;box-shadow:0 4px 12px rgba(0,0,0,0.1);padding:25px;max-width:400px;width:90%;}";
  html += "h1{color:#005a9c;font-size:2.2em;margin-bottom:10px;}.day-title{color:#555;font-size:1.8em;margin-bottom:20px;}.weather{font-size:1.5em;margin:15px 0;}";
  html += ".gemini{background-color:#e6f7ff;border-left:5px solid #1890ff;padding:15px;margin-top:20px;text-align:left;border-radius:8px;min-height:80px;}</style>";
  html += "</head><body>";
  
  // Capa de Bienvenida
  html += "<div id='welcome-screen' class='screen'><div class='welcome-container'><h1 class='title'>Panel del Clima Inteligente</h1><p class='subtitle'>Pulse un botón del 1 al 6 para comenzar...</p></div></div>";
  
  // Capa del Panel
  html += "<div id='dashboard-screen' class='screen hidden'><div class='card'><h1 id='city-name'></h1><div id='day-title'>Cargando...</div>";
  html += "<div class='weather' id='weather-desc'></div><div class='weather' id='precip-prob'></div>";
  html += "<div class='gemini'><b>Dato curioso de Gemini:</b><p id='gemini-text'></p></div></div></div>";
  
  // Script de JavaScript
  html += "<script>const welcome=document.getElementById('welcome-screen');const dashboard=document.getElementById('dashboard-screen');";
  html += "function fetchData(){fetch('/data').then(r=>r.json()).then(d=>{if(d.showDashboard){welcome.classList.add('hidden');dashboard.classList.remove('hidden');";
  html += "document.getElementById('city-name').innerText=d.cityName;document.getElementById('day-title').innerText=d.dayName;document.getElementById('weather-desc').innerHTML='Clima: <b>'+d.weatherDesc+'</b>';";
  html += "document.getElementById('precip-prob').innerHTML='Prob. Lluvia: <b>'+d.precipProb+'%</b>';document.getElementById('gemini-text').innerText=d.geminiExplanation;";
  html += "}else{welcome.classList.remove('hidden');dashboard.classList.add('hidden');}}).catch(e=>console.error('Error:',e));}";
  html += "setInterval(fetchData,2000);window.onload=fetchData;</script>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleDataRequest() {
  DynamicJsonDocument doc(1536);
  doc["showDashboard"] = isShowingDashboard;

  if (isShowingDashboard) {
    doc["cityName"] = cities[currentCityIndex].name;
    
    struct tm timeinfo;
    getLocalTime(&timeinfo);
    int forecastDayIndex = (timeinfo.tm_wday + currentDayOffset) % 7;

    String displayName;
    if (currentDayOffset == 0) {
      displayName = "Hoy";
    } else if (currentDayOffset == 1) {
      displayName = "Mañana";
    } else {
      displayName = dayNames[forecastDayIndex];
    }
    
    doc["dayName"] = displayName;
    doc["weatherDesc"] = getWeatherDescriptionString(dailyWeatherCodes[currentDayOffset]);
    doc["precipProb"] = dailyPrecipitation[currentDayOffset];

    if (geminiExplanations[currentDayOffset] == "") {
      Serial.println("Cache de Gemini vacia para el dia " + String(currentDayOffset) + ". Llamando a la API...");
      String fullWeatherDesc = getFullWeatherDescriptionString(dailyWeatherCodes[currentDayOffset]);
      String prompt = "El pronóstico para " + String(cities[currentCityIndex].name) + " para " + displayName + " es " + fullWeatherDesc + " con una probabilidad de lluvia del " + String(dailyPrecipitation[currentDayOffset]) + "%. ";
      prompt += "Explica un concepto simple de ciencia de datos relacionado con este pronostico, como si fueras un profesor muy amigable hablandole a un nino de 10 anos. Se breve, y no uses mas de 40 palabras.";
      geminiExplanations[currentDayOffset] = getGeminiExplanation(prompt);
    } else {
      Serial.println("Usando respuesta de Gemini desde la cache para el dia " + String(currentDayOffset));
    }
    doc["geminiExplanation"] = geminiExplanations[currentDayOffset];
  }
  
  String jsonString;
  serializeJson(doc, jsonString);
  server.send(200, "application/json", jsonString);
}

// =================================================================
// --- SETUP Y LOOP PRINCIPALES ---
// =================================================================
void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  irrecv.enableIRIn();
  for(int i=0; i < numMatrices; i++) { allPixels[i]->begin(); allPixels[i]->setBrightness(40); allPixels[i]->clear(); allPixels[i]->show(); }
  
  // Limpiar caché de Gemini al iniciar
  for(int i=0; i<7; i++) { geminiExplanations[i] = ""; }

  lcd.print("Conectando WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); lcd.print("."); }
  Serial.println("\nWiFi conectado!");
  Serial.print("Direccion IP para la web: ");
  Serial.println(WiFi.localIP());
  
  // Configurar hora para la ciudad inicial
  configTime(cities[currentCityIndex].gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  getWeatherData();
  showWelcomeDisplay();
  
  // Rutas del servidor
  server.on("/", HTTP_GET, handleRoot);
  server.on("/data", HTTP_GET, handleDataRequest);

  server.begin();
  Serial.println("Servidor HTTP iniciado.");
  lastIrInteractionTime = millis();
}

void changeCity() {
  Serial.printf("Cambiando a la ciudad: %s\n", cities[currentCityIndex].name);
  configTime(cities[currentCityIndex].gmtOffset_sec, daylightOffset_sec, ntpServer);
  getWeatherData();
  currentDayOffset = 0; // Resetear al día 0 de la nueva ciudad
  
  // Mostrar ciudad temporalmente
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Ciudad:");
  lcd.setCursor(0,1);
  lcd.print(cities[currentCityIndex].name);
  clearAllMatrices();

  cityChangeTimestamp = millis(); // Iniciar timer de 5 segundos
  lastIrInteractionTime = millis();
  isShowingDashboard = true;
}

void loop() {
  server.handleClient();
  
  // MODIFICADO: Lógica para volver a la bienvenida o al display del día
  if (isShowingDashboard && (millis() - lastIrInteractionTime > 30000)) {
    Serial.println("Inactividad detectada, volviendo a la pantalla de bienvenida.");
    isShowingDashboard = false;
    cityChangeTimestamp = 0; // Cancelar el timer de ciudad si lo hubiera
    showWelcomeDisplay();
  }

  // NUEVO: Revertir de la pantalla de ciudad a la del día después de 5 seg
  if (cityChangeTimestamp != 0 && (millis() - cityChangeTimestamp > 5000)) {
    Serial.println("5 segundos pasaron, mostrando pronostico del dia.");
    cityChangeTimestamp = 0; // Resetear timer
    updateDisplaysForDay(currentDayOffset); // Mostrar el día actual
  }


  if (irrecv.decode(&results)) {
    if (results.value != 0xFFFFFFFFFFFFFFFF) {
      lastIrInteractionTime = millis(); 
      isShowingDashboard = true; 
      cityChangeTimestamp = 0; // Cualquier botón cancela el display temporal de ciudad
      
      switch (results.value) {
        case IR_BUTTON_1: showProbability = false; updateDisplaysForDay(0); break;
        case IR_BUTTON_2: showProbability = false; updateDisplaysForDay(1); break;
        case IR_BUTTON_3: showProbability = false; updateDisplaysForDay(2); break;
        case IR_BUTTON_4: showProbability = false; updateDisplaysForDay(3); break;
        case IR_BUTTON_5: showProbability = false; updateDisplaysForDay(4); break;
        case IR_BUTTON_6: showProbability = false; updateDisplaysForDay(5); break;
        case IR_BUTTON_ABAJO: showProbability = true; updateDisplaysForDay(currentDayOffset); break;
        case IR_BUTTON_ARRIBA: showProbability = false; updateDisplaysForDay(currentDayOffset); break;
        
        case IR_BUTTON_LEFT:
          currentCityIndex--;
          if (currentCityIndex < 0) { currentCityIndex = numCities - 1; }
          changeCity();
          break;
          
        case IR_BUTTON_RIGHT:
          currentCityIndex++;
          if (currentCityIndex >= numCities) { currentCityIndex = 0; }
          changeCity();
          break;
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
