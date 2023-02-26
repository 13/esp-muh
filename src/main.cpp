#include <Arduino.h>
#include <WiFi.h>

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include <ArduinoOTA.h>

#include <Wire.h>

#define SENSOR_TYPE_ds18b20

#ifdef SENSOR_TYPE_ds18b20
#include <OneWire.h>
#include <DallasTemperature.h>

OneWire oneWire(SENSOR_PIN_OW);
DallasTemperature ds18b20(&oneWire);
#endif

#define SDA_PIN 1
#define SCL_PIN 3
#define SDA_PIN2 0
#define SCL_PIN2 19

// TwoWire I2Cone = TwoWire(0);

AsyncWebServer server(80);

const char *wifi_ssid = "";
const char *wifi_pass = "";
const char *hostname = "shellyplus1";

// Static IP
IPAddress ip(192, 168, 22, 96);
IPAddress sn(255, 255, 255, 0);
IPAddress gw(192, 168, 22, 6);
IPAddress dns1(192, 168, 22, 6);
IPAddress dns2(8, 8, 8, 8);

const char *PARAM_MESSAGE = "message";

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

String scanI2C()
{
  String scanOutput = "";

  // Scan I2C bus for connected devices and append the results to the scanOutput string
  byte error, address;
  int devices = 0;
  for (address = 1; address < 127; address++)
  {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0)
    {
      scanOutput += "I2C device found at address 0x";
      if (address < 16)
      {
        scanOutput += "0";
      }
      scanOutput += String(address, HEX);
      scanOutput += "\n";
      devices++;
    }
  }

  // If no devices were found, add a message indicating that to the scanOutput string
  if (devices == 0)
  {
    scanOutput = "No I2C devices found";
  }

  // Return the scan output as a String
  return scanOutput;
}

void setup()
{

  Serial.begin(115200);

  // Wire.begin(0x40, SDA_PIN, SCL_PIN, 100000);
  //Wire.begin(SDA_PIN, SCL_PIN);
  // I2Cone.begin(SDA_PIN, SCL_PIN, 100000);

  // Wire.begin(SDA_PIN2, SCL_PIN2);

  WiFi.disconnect();
  WiFi.mode(WIFI_STA); // switch off AP
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  WiFi.begin(wifi_ssid, wifi_pass);
  WiFi.config(ip, gw, sn, dns1);

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              Serial.println("Hello");
              request->send(200, "text/plain", "Hello, world 1"); });

  server.on("/bla", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    String response = "I2C device 0x40 ";
    Wire.beginTransmission(0x40);
    byte error = Wire.endTransmission();
    if (error == 0) {
      response += "found";
    } else {
      response += "not found";
    }
    request->send(200, "text/plain", response); });

  // Send a GET request to <IP>/get?message=<message>
  server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request)
            {
        String message;
        if (request->hasParam(PARAM_MESSAGE)) {
            message = request->getParam(PARAM_MESSAGE)->value();
        } else {
            message = "No message sent";
        }
        request->send(200, "text/plain", "Hello, GET: " + message); });

  // Send a POST request to <IP>/post with a form field message set to <message>
  server.on("/post", HTTP_POST, [](AsyncWebServerRequest *request)
            {
        String message;
        if (request->hasParam(PARAM_MESSAGE, true)) {
            message = request->getParam(PARAM_MESSAGE, true)->value();
        } else {
            message = "No message sent";
        }
        request->send(200, "text/plain", "Hello, POST: " + message); });

  // Route to handle incoming HTTP GET requests to /i2cscan
  server.on("/i2cscan", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    // Scan I2C bus and send the results to the client
    String scanOutput = scanI2C();
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", scanOutput);
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "GET");
    request->send(response); });

  server.onNotFound(notFound);

  server.begin();

  ArduinoOTA
      .onStart([]()
               {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("[OTA]: Start updating " + type); })
      .onEnd([]()
             { Serial.println("\n[OTA]: End"); })
      .onProgress([](unsigned int progress, unsigned int total)
                  { Serial.printf("[OTA]: Progress: %u%%\r", (progress / (total / 100))); })
      .onError([](ota_error_t error)
               {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("[OTA]: Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("[OTA]: Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("[OTA]: Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("[OTA]: Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("[OTA]: End Failed"); });

  ArduinoOTA.begin();
}

void loop()
{
  ArduinoOTA.handle();
}