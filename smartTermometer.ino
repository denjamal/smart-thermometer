#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

#define DHTPIN 2        
#define DHTTYPE DHT11
#define SLACK_SSL_FINGERPRINT "AC:95:5A:58:B8:4E:0B:CD:B3:97:D2:88:68:F5:CA:C1:0A:81:E3:6E"     
DHT dht(DHTPIN, DHTTYPE);
ESP8266WebServer server(80);
IPAddress ip(172,27,28,149);
IPAddress gateway(172, 27, 29, 254);
IPAddress subnet(255, 255, 255, 0);

const char * indexHTML  = "<!DOCTYPE html>"
"<html lang=\"en\">"

"<head>"
    "<title>ClarityAg</title>"
    "<link href=\"https://fonts.googleapis.com/css?family=Space+Mono:400,700|Teko\" rel=\"stylesheet\">"
    "<script src=\"https://code.jquery.com/jquery-3.2.1.min.js\" integrity=\"sha256-hwg4gsxgFZhOsEEamdOYGBf13FyQuiTwlAQgxVSNgt4=\""
        "crossorigin=\"anonymous\"></script>"
    "<style>"
        "html {"
            "height: 100%;"
        "}"

        "body {"
            "height: 100%;"
            "background: url(http://www.zastavki.com/pictures/1920x1200/2010/Computers_Server_019601_.jpg) center center no-repeat;"
            "background-size: cover;"
            "padding: 0px;"
        "}"

        "h2,"
        "h3 {"
            "font-family: 'Space Mono', monospace;"
            "color: white;"
            "text-transform: uppercase;"
            "text-align: center;"
            "font-size: 70px;"
        "}"

        "h3 {"
            "font-size: 60px;"
        "}"

        ".data {"
            "position: absolute;"
            "top: 50%;"
            "left: 0;"
            "right: 0;"
            "margin-top: -200px;"
        "}"
    "</style>"
"</head>"
  "<body>"
      "<div class=\"data\">"
          "<h2>temperature: <span id=\"temperature\"></span> &#8451;</h2>"
          "<h3>humidity: <span id=\"humidity\"></span> %</h3>"
      "</div>"
  "</body>"
  "<script>"
      "$(document).ready(function(){"
          "$.ajax({"
              "url: \"getTemp\","
              "method: \"Get\","
              "dataType: \"json\""
          "})"
          ".done(function(data){"
              "debugger;"
              "$(\"#temperature\").text(data.temperature);"
              "$(\"#humidity\").text(data.humidity);"
          "})"
          ".fail(function(){"
              "alert(\"Error: Please contact someone\");"
          "});"
      "});"
  "</script>"
"</html>";

const char* ssid = "clarity-ag";
const char* password = "zflvbyrj";
int temp;
int humidity;
String WebString;
bool isTempHigh  = false;

void handleRoot() {
  server.send(200, "text/html", indexHTML);
}

void getTemperature() {
  WebString = "";
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["temperature"] = (int)temp;
  root["humidity"] = (int)humidity;
  root.printTo(WebString);
  Serial.println(WebString);
  
  server.send(200, "application/json", WebString);
}

void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void GetTemp (){
  temp  = dht.readTemperature(); 
  humidity = dht.readHumidity();
  
  if(temp > 25 && !isTempHigh) {
    SendNotificationToSlack();
    isTempHigh = true;
  }else if(isTempHigh && temp <= 25){
    isTempHigh = false;
  } 
}

void SendNotificationToSlack(){
  String notification = "{'text': 'Temperature level is high - " + String((int)temp)+"C \n'}";
  HTTPClient http;
  http.begin("https://hooks.slack.com/services/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", SLACK_SSL_FINGERPRINT);      
  http.addHeader("Content-Type", "application/json");  
  int httpCode = http.POST(notification);
  http.end();
}

void Wifi_Reconect (){
  WiFi.disconnect();
  WiFi.config(ip, gateway, subnet);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Trying to connect");
    Serial.print(".");
  }
}

void setup(void){
  Serial.begin(115200);
  
  dht.begin();
  WiFi.config(ip, gateway, subnet);
  WiFi.begin(ssid, password);
  
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.on("/getTemp", getTemperature);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void){
  if (WiFi.status() != WL_CONNECTED)
    {
      Wifi_Reconect();
    }else {
      server.handleClient();
      delay(2000);
      GetTemp();  
    }
}