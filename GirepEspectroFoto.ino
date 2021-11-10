//Include of Libraries
#include <Arduino.h>
#include <Arduino_JSON.h>       
#include <AsyncTCP.h>             //Lib Asyncronous Server
#include <ESPAsyncWebServer.h>    //Lib Asyncronous Server
#include <ESPmDNS.h>              //Lib DNS
#include <SPIFFS.h>               //Lib File Manager
#include <WiFi.h>                 //Lib WiFi

//Alocacao de Pinos
const int SensorPin = 34;         //Pin for Analog Sensor
const int LedPinRed = 19;         //Pin for Activate LED Red
const int LedPinGreen = 18;       //Pin for Activate LED Green
const int LedPinBlue = 17;        //Pin for Activate LED Blue

//Definicao de Variaveis
String message = "";              //Variable for 
String sliderValue1 = "0";
String sliderValue2 = "0";
String sliderValue3 = "0";

int dutyCycle1;
int dutyCycle2;
int dutyCycle3;

const int freq = 5000;
const int ledChannelRed = 0;
const int ledChannelGreen = 1;
const int ledChannelBlue = 2;

const int resolution = 8 ;

//Definicao de Credenciais de Rede WiFi
const char* ssid = "YOURWIFISSID";
const char* password = "YOURPASSWORD";

//Ajuste de Servidor
AsyncWebServer server(80);

// Criar objeto WebSocket
AsyncWebSocket ws("/ws");

//Variavel JSON para manter os valores dos controles
JSONVar sliderValues;

//Leitura dos Controles da WebPage
String getSliderValues() {
  sliderValues["sliderValue1"] = String(sliderValue1);
  sliderValues["sliderValue2"] = String(sliderValue2);
  sliderValues["sliderValue3"] = String(sliderValue3);

  String jsonString = JSON.stringify(sliderValues);
  return jsonString;
}

//Leitura dos Dados do Sensor
String read_light() {
//  float light = analogRead(SensorPin);
//float volts = analogRead(SensorPin) * 3.3 / 4095.0;
//float amps = volts / 10000.0;
//  float microamps = amps * 1000000;
//  float light = microamps * 2.0;

float light = ((analogRead(SensorPin) * 3.3) / 4095.0)*200;
  
  if (isnan(light)) {
    Serial.println("Falhou ao ler sensor de Luz!");
    return "";
  }
  else {
    Serial.println(light);
    return String(light);
  }
}

// Initialize SPIFFS
void initFS() {
  if (!SPIFFS.begin()) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  else {
    Serial.println("SPIFFS mounted successfully");
  }
}


// Initialize WiFi
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  if (!MDNS.begin("espectro")) {
    Serial.println("Erro no Resolvedor de Nomes");
    return;
  }
  Serial.println("IP Address: ");
  Serial.println(WiFi.localIP());
}


void notifyClients(String sliderValues) {
  ws.textAll(sliderValues);
}


void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    message = (char*)data;
    if (message.indexOf("1s") >= 0) {
      sliderValue1 = message.substring(2);
      dutyCycle1 = map(sliderValue1.toInt(), 0, 100, 0, 255);
      Serial.println(dutyCycle1);
      Serial.print(getSliderValues());
      notifyClients(getSliderValues());
    }
    if (message.indexOf("2s") >= 0) {
      sliderValue2 = message.substring(2);
      dutyCycle2 = map(sliderValue2.toInt(), 0, 100, 0, 255);
      Serial.println(dutyCycle2);
      Serial.print(getSliderValues());
      notifyClients(getSliderValues());
    }
    if (message.indexOf("3s") >= 0) {
      sliderValue3 = message.substring(2);
      dutyCycle3 = map(sliderValue3.toInt(), 0, 100, 0, 255);
      Serial.println(dutyCycle3);
      Serial.print(getSliderValues());
      notifyClients(getSliderValues());
    }
    if (strcmp((char*)data, "getValues") == 0) {
      notifyClients(getSliderValues());
    }
  }
}
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void setup() {
  //Adjust of ADC
  analogSetAttenuation(ADC_6db);
  Serial.begin(115200);
  pinMode(SensorPin, INPUT);
  pinMode(LedPinRed, OUTPUT);
  pinMode(LedPinGreen, OUTPUT);
  pinMode(LedPinBlue, OUTPUT);
  initFS();
  initWiFi();

  // configure LED PWM functionalitites
  ledcSetup(ledChannelRed, freq, resolution);
  ledcSetup(ledChannelGreen, freq, resolution);
  ledcSetup(ledChannelBlue, freq, resolution);

  // attach the channel to the GPIO to be controlled
  ledcAttachPin(LedPinRed, ledChannelRed);
  ledcAttachPin(LedPinGreen, ledChannelGreen);
  ledcAttachPin(LedPinBlue, ledChannelBlue);

  initWebSocket();

  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html", "text/html");
  });

  server.on("/light", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", read_light().c_str());
  });

  server.serveStatic("/", SPIFFS, "/");

  // Start server
  server.begin();

}

void loop() {
  ledcWrite(ledChannelRed, dutyCycle1);
  ledcWrite(ledChannelGreen, dutyCycle2);
  ledcWrite(ledChannelBlue, dutyCycle3);
  delay(100);
  ws.cleanupClients();

}
