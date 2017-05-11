#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>

#define DHTPIN 2        
#define DHTTYPE DHT11     
DHT dht(DHTPIN, DHTTYPE);
ESP8266WebServer server(80);
IPAddress ip(10,0,1,77);
IPAddress gateway(10, 0, 1, 1);
IPAddress subnet(255, 255, 255, 0);

const char* ssid = "AirPort Extreme";
const char* password = "vitaminu123";
int temp;
int humidity;
String WebString;
bool isTempHigh  = false;

void handleRoot() {
  GetTemp();
  server.send(200, "text/plain", WebString);
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
  WebString = "Temperature: " + String((int)temp)+"C \n";
  WebString += "Humidity: " + String((int)humidity)+"% \n";
  Serial.println(WebString);

  if(temp > 20 && !isTempHigh) {
    SendNotificationToSlack();
    isTempHigh = true;
  }else if(isTempHigh && temp <= 20){
    isTempHigh = false;
  } 
}

void SendNotificationToSlack(){
  String notification = "{'text': 'Temperature level is high - " + String((int)temp)+"C \n'}";
  HTTPClient http;
  http.begin("https://hooks.slack.com/services/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");      
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


