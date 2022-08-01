#include <DHT.h>
#include <Wire.h>
#include <CTBot.h>
#include <RTClib.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

// Personal Lib
#include "MQ2.h" // MQ2 Lib

// Personal Configuration
#include "auth.h"

// API Server
String serverName = "http://skripsi.skripsif.site/fadli/insertData/sensor";

/////// please enter your sensitive data in the Secret tab/auth.h
/////// Wifi Settings ///////
const char* ssid = WIFI_SSID;
const char* pass = WIFI_PASS;

// Variable Declaration
int _second, prevSmoke;
float temp, hum, smoke;
bool tempWarning = false;
bool humWarning = false;
//bool smokeWarning = false;
unsigned long prevMillis = 0;        // will store last time LED was updated
unsigned long currentMillis;

// Telegram BOT Section
CTBot myBot;

// RTC
RTC_DS3231 rtc;

//// MQ2 Init
//MQ2 mq2(mq2Pin);

// Object (Sensor) Init
DHT dht(dhtPin, DHT22);

// NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "0.id.pool.ntp.org");

void setup() {
  Serial.begin(115200);

  // We start by connecting to a WiFi network
  Serial.print(F("\n\nNode "));
  Serial.println(String(node));
  Serial.print(F("Connecting to "));
  Serial.println(ssid);

  myBot.wifiConnect(ssid, pass);   // connect the ESP8266 to the desired access point
  myBot.setTelegramToken(token);   // set the telegram bot token

  initIO();

  // check if all things are ok
  if (myBot.testConnection()) {
    Serial.println(F("\nConnection OK"));
    rtc.adjust(DateTime(2022, 6, timeClient.getDay(), timeClient.getHours(), timeClient.getMinutes(), timeClient.getSeconds()));
    myBot.sendMessage(GroupID, "Node " + String(node) + ": Connected to " + String(ssid) + "\n\nNode " + String(node) + ": Koneksi Berhasil\n");
  } else {
    Serial.println(F("\nConnection Not OK"));
  }

  delay(2000);
  temp = dht.readTemperature(); // Get Temperature Data
  hum = dht.readHumidity(); // Get Humidity Data
//  smoke = mq2.readSmoke(); // Get Smoke Data
}

// OK
void initIO() {
  dht.begin();
//  mq2.begin();
  timeClient.begin();
  timeClient.setTimeOffset(25200); // UTP+7
  timeClient.update();
  Wire.begin(5, 4);  //Setting wire (5/D1 untuk SDA dan 4/D2 untuk SCL)
  rtc.begin();
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

void loop() {
  currentMillis = millis();
  DateTime now = rtc.now();
  _second = now.second();

  if ((_second % 30 == 0) || (currentMillis - prevMillis >= 30000)) {
    // save the last time you blinked the LED
    prevMillis = currentMillis;
    sendToDB();
  }

  if (_second % 2 == 0) {
    temp = dht.readTemperature(); // Get Temperature Data
    hum = dht.readHumidity(); // Get Humidity Data
//    smoke = mq2.readSmoke(); // Get Smoke Data
    Serial.println(F("\n\nRTC \t\t:") + String(_second));
    Serial.println(F("\nSuhu \t\t:") + String(temp));
//    Serial.println(F("Smoke \t\t:") + String(smoke));
    Serial.println(F("Kelembapan \t:") + String(hum));
    Serial.println(F("======================================="));
  }

  checkData();
  getMsg();
  delay(1000); // wait 1 seconds
}

void getMsg() {
  // a variable to store telegram message data
  TBMessage msg;

  // if there is an incoming message...
  if (myBot.getNewMessage(msg))
  {
    // check if the message is a text message
    if (msg.messageType == CTBotMessageText) {
      int id;
      // check if the message comes from a chat group (the group.id is negative)
      if (msg.group.id < 0) {
        id =  msg.group.id;
        sendFeedback(id, msg.text);
      } else {
        id = msg.sender.id;
        sendFeedback(id, msg.text);
      }
      Serial.print(F("Pesan : "));
      Serial.println(msg.text);
    }
  }
}

void sendFeedback(int id, String cmd) {
  if (cmd.indexOf("/status") == 0) {
//    myBot.sendMessage(id, "Node " + String(node) + " Online \n\nSuhu : " + String(temp) + " ºC \nAsap\t\t: " + String(smoke) + " ppm\nKelembapan : " + String(hum) + " %RH\n");
    myBot.sendMessage(id, "Node " + String(node) + " Online \n\nSuhu : " + String(temp) + " ºC \nKelembapan : " + String(hum) + " %RH\n");
  }
}

void checkData() {
  //  Temperature Section
  if ((temp < lowTemp) && !tempWarning)  {
    tempWarning  = true;
    myBot.sendMessage(GroupID, "Suhu Sekarang kurang dari " + String(lowTemp) + "ºC pada Ruang Server Node " + String(node) + "\n\nSuhu: " + String(temp) + "ºC\nMohon Cek Ruangan Server!!!");
  }

  if ((temp > highTemp) && !tempWarning) {
    tempWarning = true;
    myBot.sendMessage(GroupID, "Suhu Sekarang melebihi dari " + String(highTemp) + "ºC pada Ruang Server Node " + String(node) + "\n\nSuhu: " + String(temp) + "ºC\nMohon Cek Ruangan Server!!!");
  }

  if ((temp < highTemp) && (temp > lowTemp) && tempWarning) {
    myBot.sendMessage(GroupID, "Suhu Telah Normal pada Ruang Server Node " + String(node) + "\n\nSuhu: " + String(temp) + "ºC");
    tempWarning = false;
  }

  //  Humidity Section
  if ((hum < lowHum)  && !humWarning)  {
    myBot.sendMessage(GroupID, "Kelembapan Sekarang Kurang dari " + String(lowHum) + " % RH pada Ruang Server Node " + String(node) + "\n\nKelembapan: " + String(hum) + " % RH\nMohon Cek Ruangan Server!!!");
    humWarning = true;
  }

  if ((hum > highHum) && !humWarning)  {
    myBot.sendMessage(GroupID, "Kelembapan Sekarang Lebih dari " + String(highHum) + " % RH pada Ruang Server Node " + String(node) + "\n\nKelembapan: " + String(hum) + " % RH\nMohon Cek Ruangan Server!!!");
    humWarning = true;
  }

  if ((hum < highHum) && (hum > lowHum) && humWarning) {
    myBot.sendMessage(GroupID, "Kelembapan Telah Normal pada Ruang Server Node " + String(node) + "\n\nKelembapan: " + String(hum) + " % RH");
    humWarning = false;
  }

  //  Smoke Section
//  if ((smoke > highSmoke) && !smokeWarning) {
//    prevSmoke = smoke;
//    myBot.sendMessage(GroupID, "Terdeteksi Asap pada Ruang Server Node " + String(node) + "\n\nAsap: " + String(smoke) + "ppm");
//    smokeWarning = true;
//  }
//
//  if ((prevSmoke > highSmoke) && (smoke < highSmoke) && smokeWarning) {
//    myBot.sendMessage(GroupID, "Asap sudah Tidak Terdeteksi pada Ruang Server Node " + String(node) + "\n\nAsap: " + String(smoke) + "ppm");
//    smokeWarning = false;
//  }
}

void sendToDB() {
  // Check if any reads failed and exit early (to try again).
  Serial.println(F("Kirim Data"));
  // Check Connection status
  if (myBot.testConnection()) {
    WiFiClient client;
    HTTPClient http;
//    String serverPath = serverName + node + "/" + String(temp) + "/" + String(hum) + "/" + String(smoke) +  "/0"; 
    String serverPath = serverName + node + "/" + String(temp) + "/" + String(hum) + "/0/0"; // make sure no space in quote on this line

    // Your Domain name with URL path or IP address with path
    http.begin(client, serverPath.c_str());

    // Send HTTP GET request
    int httpResponseCode = http.GET();

    Serial.println(serverPath);
    Serial.print(F("HTTP Response code: "));
    Serial.println(httpResponseCode);

    http.end(); // Free resources
  } else {
    Serial.println(F("Not Connected to Internet"));
  }
}
