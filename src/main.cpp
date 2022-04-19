#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <FastLED.h>
#include <ArduinoOTA.h>
#include <FS.h>
#include <Ticker.h>

#define PIN_LED 13
#define PIN_BOOT 0

#define TOWER_HEIGHT 1
#define LED_PER_TOWER 8
#define NUM_LEDS (LED_PER_TOWER * TOWER_HEIGHT)

const char *settings_path = "/wifi_settings.txt";
String settings_pass = "STACKLAMP!!";

ESP8266WebServer WebSV(80);
Ticker ledTicker;

CRGB leds[NUM_LEDS];

enum LED_MODE
{
  LED_MODE_NONE,
  LED_MODE_ALWAYS,
  LED_MODE_BLINK,
  LED_MODE_TURN,
  LED_MODE_RAINBOW,
  LED_MODE_RAINBOW_SPIN,
  LED_MODE_MAX,
};

typedef struct
{
  enum LED_MODE mode;
  float hz;
  CRGB color;
} TOWER_MODE;

TOWER_MODE tower_setting[TOWER_HEIGHT];

void setled()
{
  int i, j;
  for (i = 0; i < TOWER_HEIGHT; i++)
  {
    switch (tower_setting[i].mode)
    {
    case LED_MODE_NONE:
      for (j = 0; j < LED_PER_TOWER; j++)
      {
        leds[i * LED_PER_TOWER + j] = CRGB::Black;
      }
      break;
    case LED_MODE_ALWAYS:
      for (j = 0; j < LED_PER_TOWER; j++)
      {
        leds[i * LED_PER_TOWER + j] = tower_setting[i].color;
      }
      break;
    case LED_MODE_BLINK:
      for (j = 0; j < LED_PER_TOWER; j++)
      {
        leds[i * LED_PER_TOWER + j] = ((int)(1.0 * millis() / 1000.0 * tower_setting[i].hz) % 2) ? (tower_setting[i].color) : (CRGB::Black);
      }
      break;
    case LED_MODE_TURN:
      for (j = 0; j < LED_PER_TOWER; j++)
      {
        leds[i * LED_PER_TOWER + j] = (((int)(1.0 * millis() / 1000.0 * tower_setting[i].hz * LED_PER_TOWER) % LED_PER_TOWER) == j) ? (tower_setting[i].color) : (CRGB::Black);
      }
      break;
    case LED_MODE_RAINBOW:
      for (j = 0; j < LED_PER_TOWER; j++)
      {
        leds[i * LED_PER_TOWER + j].setHue((int)(millis() / 1000.0 * tower_setting[i].hz * 255) % 255);
      }
      break;
    case LED_MODE_RAINBOW_SPIN:
      for (j = 0; j < LED_PER_TOWER; j++)
      {
        leds[i * LED_PER_TOWER + j].setHue((int)(millis() / 1000.0 * tower_setting[i].hz * 255 + 1.0 * (LED_PER_TOWER - j - 1) * 255.0 / LED_PER_TOWER) % 255);
      }
      break;
    }
  }
  FastLED.show();
}

//  MIMEタイプを推定
String getContentType(String filename)
{
  if (filename.endsWith(".html") || filename.endsWith(".htm"))
    return "text/html";
  else if (filename.endsWith(".css"))
    return "text/css";
  else if (filename.endsWith(".js"))
    return "application/javascript";
  else if (filename.endsWith(".png"))
    return "image/png";
  else if (filename.endsWith(".gif"))
    return "image/gif";
  else if (filename.endsWith(".jpg"))
    return "image/jpeg";
  else
    return "text/plain";
}

bool handleFileRead(String path)
{
  Serial.println("handleFileRead: trying to read " + path);
  if (path.endsWith("/"))
    path += "index.html";
  String contentType = getContentType(path);
  if (SPIFFS.exists(path))
  {
    Serial.println("handleFileRead: sending " + path);
    File file = SPIFFS.open(path, "r");
    WebSV.streamFile(file, contentType);
    file.close();
    Serial.println("handleFileRead: sent " + path);
    return true;
  }
  else
  {
    Serial.println("handleFileRead: 404 not found");
    return false;
  }
}

void handleNotFound()
{
  if (WebSV.uri().indexOf(settings_path) >= 0 || !handleFileRead(WebSV.uri()))
  {
    Serial.println("404 not found");
    WebSV.send(404, "text/plain", "File not found");
  }
}

void handleSettingGet()
{
  String html = "";
  html += "<h1>WiFi Settings</h1>";
  html += "<form method='post'>";
  html += "  <input type='text' name='ssid' placeholder='ssid'><br>";
  html += "  <input type='text' name='pass' placeholder='pass'><br>";
  html += "  <input type='submit'><br>";
  html += "</form>";
  WebSV.send(200, "text/html", html);
}

void handleSettingPost()
{
  String ssid = WebSV.arg("ssid");
  String pass = WebSV.arg("pass");

  File f = SPIFFS.open(settings_path, "w");
  f.println(ssid);
  f.println(pass);
  f.close();

  String html = "";
  html += "<h1>WiFi Settings</h1>";
  html += ssid + "<br>";
  html += pass + "<br>";
  WebSV.send(200, "text/html", html);
}

void setup_server()
{
  byte mac[6];
  WiFi.macAddress(mac);
  String ssid = "StackLamp";
  for (int i = 0; i < 6; i++)
  {
    ssid += String(mac[i], HEX);
  }
  Serial.println("SSID: " + ssid);
  Serial.println("PASS: " + settings_pass);

  WiFi.softAP(ssid.c_str(), settings_pass.c_str());

  WebSV.on("/", HTTP_GET, handleSettingGet);
  WebSV.on("/", HTTP_POST, handleSettingPost);
  WebSV.begin();
  Serial.println("HTTP server started.");
}

void setup()
{
  int i;
  unsigned long time;
  Serial.begin(115200);
  SPIFFS.begin();
  FastLED.addLeds<NEOPIXEL, PIN_LED>(leds, NUM_LEDS);
  FastLED.setBrightness(64);
  pinMode(PIN_BOOT, INPUT);

  for (i = 0; i < TOWER_HEIGHT; i++)
  {
    tower_setting[i].hz = 10;
    tower_setting[i].mode = LED_MODE_BLINK;
    tower_setting[i].color = CRGB::Red;
  }
  ledTicker.attach_ms(5, setled);

  time = millis();
  while (millis() - time < 3000)
  {
    if (digitalRead(PIN_BOOT) == 0)
    {
      setup_server();
      for (i = 0; i < TOWER_HEIGHT; i++)
      {
        tower_setting[i].hz = 5;
        tower_setting[i].mode = LED_MODE_RAINBOW_SPIN;
      }

      while (1)
      {
        WebSV.handleClient();
        delay(10);
      }
      break;
    }
    delay(10);
  }

  for (i = 0; i < TOWER_HEIGHT; i++)
  {
    tower_setting[i].hz = 5;
    tower_setting[i].mode = LED_MODE_TURN;
    tower_setting[i].color = CRGB::Blue;
  }

  File f = SPIFFS.open(settings_path, "r");
  String ssid = f.readStringUntil('\n');
  String pass = f.readStringUntil('\n');
  f.close();

  ssid.trim();
  pass.trim();

  Serial.println("SSID: " + ssid);
  Serial.println("PASS: xxx");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  ArduinoOTA.onStart([]()
                     {
                       String type;
                       if (ArduinoOTA.getCommand() == U_FLASH)
                       {
                         type = "sketch";
                       }
                       else
                       { 
                         // U_SPIFFS
                         type = "filesystem";
                       }
                       // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
                       Serial.println("Start updating " + type); });
  ArduinoOTA.onEnd([]()
                   { Serial.println("\nEnd"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });
  ArduinoOTA.onError([](ota_error_t error)
                     {
                       Serial.printf("Error[%u]: ", error);
                       if (error == OTA_AUTH_ERROR)
                       {
                         Serial.println("Auth Failed");
                       }
                       else if (error == OTA_BEGIN_ERROR)
                       {
                         Serial.println("Begin Failed");
                       }
                       else if (error == OTA_CONNECT_ERROR)
                       {
                         Serial.println("Connect Failed");
                       }
                       else if (error == OTA_RECEIVE_ERROR)
                       {
                         Serial.println("Receive Failed");
                       }
                       else if (error == OTA_END_ERROR)
                       {
                         Serial.println("End Failed");
                       } });
  ArduinoOTA.begin();

  WebSV.on("/tower", []()
           {
             int layer = WebSV.arg("layer").toInt();
             int mode = WebSV.arg("mode").toInt();
             float hz = WebSV.arg("hz").toFloat();
             int color = strtol(WebSV.arg("color").c_str(), NULL, 16);
             CRGB c;
             if (0 <= layer && layer < TOWER_HEIGHT && 0 <= mode && mode < LED_MODE_MAX)
             {
               tower_setting[layer].mode = (LED_MODE)mode;
               if (0 < hz && hz < 100)
               {
                 tower_setting[layer].hz = hz;
               }
               if (0 <= color && color <= 0xffffff)
               {
                 c.setColorCode(color);
                 tower_setting[layer].color = c;
               }
               WebSV.send(200, "text/html", "ok");
             }
             else
             {
               WebSV.send(500, "text/html", "error");
             } });

  WebSV.on("/tower_height", []()
           { WebSV.send(200, "text/html", String(TOWER_HEIGHT).c_str()); });
  WebSV.on("/tower_info", []()
           {
             int i;
             char tmp[256];
             String buf = "";
             sprintf(tmp, "{\"height\":%d,\"tower\":[", TOWER_HEIGHT);
             buf += tmp;
             for (i = 0; i < TOWER_HEIGHT; i++)
             {
               sprintf(tmp, "{\"mode\":%d,\"hz\":%f,\"color\":\"#%02x%02x%02x\"}%s",
                       tower_setting[i].mode, tower_setting[i].hz,
                       tower_setting[i].color.red, tower_setting[i].color.green, tower_setting[i].color.blue, ((i < TOWER_HEIGHT - 1) ? "," : ""));
               buf += tmp;
             }
             buf += "]}";

             WebSV.send(200, "text/html", buf); });

  WebSV.onNotFound(handleNotFound);
  WebSV.begin();

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Reset
  for (i = 0; i < TOWER_HEIGHT; i++)
  {
    tower_setting[i].mode = LED_MODE_NONE;
    tower_setting[i].color = CRGB::Black;
  }
}

void loop()
{
  ArduinoOTA.handle();
  WebSV.handleClient();
  delay(10);
}