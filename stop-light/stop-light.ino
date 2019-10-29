#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <PubSubClient.h>
#include "./config.h"

ESP8266WiFiMulti wifiMulti;
WiFiClient wifiClient;
PubSubClient client("172.20.10.3", 1883, wifiClient);
String device_name = "esp8266-stop-light";
const char* ssid1 = HOME_WIFI_SSID;
const char* passwd1 = HOME_WIFI_PASSWORD;
const char* ssid2 = MATT_HOTSPOT_SSID;
const char* passwd2 = MATT_HOTSPOT_PASSWORD;

int RED_LIGHT = D5;
int YEL_LIGHT = D6;
int GRN_LIGHT = D7;

bool doorOpen = false;
bool flash = false;
unsigned long timer_pointer = 0;

void connectToWifi(){
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(ssid1, passwd1);
  wifiMulti.addAP(ssid2, passwd2);
  
  Serial.print("\nConnecting");
  
  while (wifiMulti.run() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Connected to ");
  Serial.print(WiFi.SSID());
  Serial.print(", IP Address: ");
  Serial.println(WiFi.localIP());

  if (wifiMulti.run() == WL_CONNECTED){
    digitalWrite(BUILTIN_LED, LOW);
  }
}

void callback(char* _topic, byte* payload, unsigned int length) {
  int i;
  char message_buff[100];
  for(i = 0; i < length; i++) {
    message_buff[i] = payload[i];
  }
  message_buff[i] = '\0';
  String message = String(message_buff);
  String topic = String(_topic);

  if (topic == "/door" || topic == "/door/p"){
    doorOpen = (message == "1");
  }
  if (topic == "/distance" || topic == "/distance/p"){
    handleDistance(message);
  }
}

void handleDistance(String message){
  if (!doorOpen){
    return;
  }
  int distance = message.toInt();
  if (distance >= 25){
    setLED("green");
    flash = false;
  } else if (25 > distance && distance >= 15){
    setLED("yellow");
    flash = false;
  } else if (15 > distance && distance >= 5){
    setLED("red");
    flash = false;
  } else if (distance < 5) {
    setLED("red");
    flash = true;
  }
}

void reconnectToHub() {
  while (!client.connected()){
    if (client.connect((char*) device_name.c_str())){
      Serial.println("Connected to MQTT Hub");
      String message = "Stoplight (" + device_name + ") connected @ " + WiFi.localIP().toString();
      client.publish("/connections/stop-light", message.c_str());
    } else {
      Serial.println("Connection to MQTT failed, trying again...");
      delay(3000);
    }  
  }  
}

void setLED(String color) {
  digitalWrite(RED_LIGHT, (color == "red"));
  digitalWrite(YEL_LIGHT, (color == "yellow"));
  digitalWrite(GRN_LIGHT, (color == "green"));
}

String getLED() {
  String res = "";

  if (digitalRead(RED_LIGHT)) return "red";
  if (digitalRead(YEL_LIGHT)) return "yellow";
  if (digitalRead(GRN_LIGHT)) return "green";

  return "off";
}

void setup(){
  Serial.begin(115200);
  connectToWifi();

  pinMode(RED_LIGHT, OUTPUT);
  pinMode(YEL_LIGHT, OUTPUT);
  pinMode(GRN_LIGHT, OUTPUT);
}

void loop(){
  if (!client.connected()){
    reconnectToHub();
  }

  client.loop();
  if (flash && (millis() > timer_pointer + 1000)){
    timer_pointer = millis();
    if (getLED() == "red"){
      setLED("off");
    } else if (getLED() == "off"){
      setLED("red");
    }    
  }
}
