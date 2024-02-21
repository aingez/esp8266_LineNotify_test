#include "secret_variable.h"
#include <TridentTD_LineNotify.h>
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>

#define SSID        secretSSID
#define PASSWORD    secretPass
#define LINE_TOKEN  secretTokenLine
#define AIR_TOKEN  secretTokenAir

int sleepInterval;

String cities[] = {"bangkok", "nonthaburi"};
int aqi[2];
const long utcOffsetInSeconds = 7 * (3600); // GMT +7
String chatReport;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, utcOffsetInSeconds);

void setup() {
  Serial.begin(115200);
  
  WiFi.begin(SSID, PASSWORD);  // Connect to Wi-Fi
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  Serial.println(LINE.getVersion()); // Line setup
  LINE.setToken(LINE_TOKEN); 
  timeClient.begin();
}

void loop() {
  Serial.println("Active");

  timeClient.update();
  int currentHour = timeClient.getHours(); // Get the current hour
  int currentMin = timeClient.getMinutes(); // Get the current min
  
  if ((currentHour == 7) || (currentHour == 10) || (currentHour == 12) || (currentHour == 15) || (currentHour == 18)) {
    updateDataSequence();
    chatReport = "AQIindex.   BKK: " + String(aqi[0]) + " NONT: " + String(aqi[1]);
    Serial.println(chatReport);
    LINE.notify(chatReport);
  }

  sleepInterval = (60 - currentMin) * 60 * 1e6; //next sleep interval
  ESP.deepSleep(sleepInterval);
}

void updateDataSequence(){
  for (int i = 0; i < 2; i++) {
    String responseData = makeHTTPRequest(cities[i]);
    parseAirQualityInfo(responseData, i);
  }
}

String makeHTTPRequest(String cityName) {
  WiFiClient client;
  String httpRequest = "GET /feed/" + cityName + "/?token=" + AIR_TOKEN + " HTTP/1.1\r\n";

  if (!client.connect("api.waqi.info", 80)) {
    Serial.println("Connection failed");
    return "";
  }

  // Make a HTTP GET request
  client.print(httpRequest);
  client.print("Host: api.waqi.info\r\n");
  client.print("Connection: close\r\n\r\n");

  Serial.println("Request sent");

  // Wait for the server's response
  while (!client.available()) {
    delay(100);
  }

  // Read and parse the server's response
  String response = "";
  while (client.available()) {
    response += client.readStringUntil('\r');
  }
  Serial.println(response);

  // Find the start and end index of "data"
  int startIndex = response.indexOf("\"data\":") + 7; // Length of "\"data\":"
  int endIndex = response.indexOf("},", startIndex) + 1;

  // Extract the "data" substring
  String dataString = response.substring(startIndex, endIndex);

  Serial.println("Closing connection");
  client.stop();

  return dataString;
}


void parseAirQualityInfo(String jsonString, int index) {
  DynamicJsonDocument doc(1024); // Assuming a maximum size, adjust if necessary

  deserializeJson(doc, jsonString);

  // Extracting data
   
  String dominantPollutant = doc["dominentpol"];
  // String location = doc["attributions"]["city"]["name"];
  // float iaqiPM10 = doc["attributions"]["iaqi"]["pm10"]["v"];
  // float iaqiCO = doc["attributions"]["iaqi"]["co"]["v"];
  // float iaqiPM25 = doc["attributions"]["iaqi"]["pm25"]["v"];
  Serial.println(doc["attributions"]);

}
