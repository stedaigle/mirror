#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <GoogleMapsApi.h>
#include <WiFiClientSecure.h>
#include <SoftwareSerial.h>

//------- Replace the following! ------
const char* ssid = "SSID";          // Network SSID (name)
const char* password = "PASSWORD";  // Network key
#define API_KEY "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"  // Google apps API Token

SoftwareSerial huzzah(4, 5);  // Adafruit Huzzah #4, #5 = RX, TX
WiFiClientSecure client;
GoogleMapsApi api(API_KEY, client);

// Free Google Maps Api only allows for 2500 "elements" a day, so be careful that you don't go over
// Adjust the amount of delay between requests to limit one element request per minute
unsigned long seconds = 1000L;
unsigned long minutes = seconds * 60;

// Inputs
// For both origin and destination you can pass multiple locations seperated by the | symbol
// e.g destination1|destination2 etc.
String origin = "LATITUDE,LONGITUDE";
String destination = "LATITUDE,LONGITUDE|LATITUDE,LONGITUDE";

// Optional
String departureTime = "now";       // This can also be a timestamp, needs to be in the future for traffic info
String trafficModel = "best_guess"; // Defaults to this anyway - see https://developers.google.com/maps/documentation/distance-matrix/intro#DistanceMatrixRequests for more info

// Declare strings that will store the Google Maps results
String time1 = "";
String dist1 = "";
String time2 = "";
String dist2 = "";

// Weather server web addresses
const char* host = "http://XXX.XXX.X.X"; // Use local IP address of weather server - e.g 192.168.1.5
const char* http_temp = "http://XXX.XXX.X.X/temp";
const char* http_humid = "http://XXX.XXX.X.X/humid";
const char* http_pres = "http://XXX.XXX.X.X/pres";
const char* http_alt = "http://XXX.XXX.X.X/alt";
const char* http_co2 = "http://XXX.XXX.X.X/co2";
const char* http_tvoc = "http://XXX.XXX.X.X/tvoc";

// Declare variables that will store data retrieved from the weather server
int httpCode = 0;
String temp = "";
String humid = "";
String pres = "";
String alt = "";
String co2 = "";
String tvoc = "";

void setup() {
  Serial.begin(115200);
  huzzah.begin(115200);

  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // Connect to WiFi network
  Serial.print("\n\r \n\rConnecting Wifi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    client.setInsecure();
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

bool checkGoogleMaps() {
  Serial.println("Google Maps API");
  Serial.println("Getting traffic for " + origin + " to " + destination);
    String responseString = api.distanceMatrix(origin, destination, departureTime, trafficModel);
    DynamicJsonDocument jsonBuffer(1024);
    DeserializationError response = deserializeJson(jsonBuffer, responseString);
    JsonObject root = jsonBuffer.as<JsonObject>();

    if (!response) {
      String status = root["status"];
      if(status =="OK") {
        Serial.println("Status : " + status);
        String distance1 = root["rows"][0]["elements"][0]["distance"]["text"];
        String traffic1 = root["rows"][0]["elements"][0]["duration_in_traffic"]["text"];
        String distance2 = root["rows"][0]["elements"][1]["distance"]["text"];
        String traffic2 = root["rows"][0]["elements"][1]["duration_in_traffic"]["text"];
        time1 = traffic1;
        dist1 = distance1;
        time2 = traffic2;
        dist2 = distance2;
        return true;
      }
      else {
        Serial.println("Got an error status");
        return false;
      }
    }
    else {
      if(responseString == "") {
        Serial.println("No response, probably timed out");
      } 
      else {
        Serial.println("Failed to parse Json. Response:");
        Serial.println(responseString);
      }
      return false;
    }
  return false;
}

void loop() {
  WiFiClient client;
  HTTPClient http;

  Serial.print("Connecting to ");
  Serial.println(host);
  if (http.begin(client, host)) { // Start connection and get HTTP header
    httpCode = http.GET();        // httpCode will be negative on error
    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String host = http.getString();
        Serial.println(host);
      }
    } else {
      Serial.printf("HTTP getString() failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  } else {
    Serial.println("Unable to connect");
  }

  if (http.begin(client, http_temp)) {
    httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
      temp = http.getString();
    }
    http.end();
  }

  if (http.begin(client, http_humid)) {
    httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
      humid = http.getString();
    }
    http.end();
  }

  if (http.begin(client, http_pres)) {
    httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
      pres = http.getString();
    }
    http.end();
  }

  if (http.begin(client, http_alt)) {
    httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
      alt = http.getString();
    }
    http.end();
  }

  if (http.begin(client, http_co2)) {
    httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
      co2 = http.getString();
    }
    http.end();
  }

  if (http.begin(client, http_tvoc)) {
    httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
      tvoc = http.getString();
    }
    http.end();
  }

  checkGoogleMaps();

  huzzah.print("<"+temp+","+humid+","+pres+","+alt+","+co2+","+tvoc+",WORK,"+time1+","+dist1+",SCHOOL,"+time2+","+dist2+">");   // Send data from Huzzah to Artemis ATP
  Serial.println("<"+temp+","+humid+","+pres+","+alt+","+co2+","+tvoc+",WORK,"+time1+","+dist1+",SCHOOL,"+time2+","+dist2+">"); // Print data to serial monitor
  delay(2 * minutes);
}
