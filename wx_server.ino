#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <SparkFunBME280.h>
#include <SparkFunCCS811.h>

#define CCS811_ADDR 0x5B   // Default I2C Address
//#define CCS811_ADDR 0x5A // Alternate I2C Address

#define PIN_NOT_WAKE 5

// Global sensor objects
CCS811 myCCS811(CCS811_ADDR);
BME280 myBME280;

ESP8266WebServer server(80);

const char* ssid = "SSID";             // Enter network SSID (name)
const char* password = "PASSWORD";     // Enter network key
String webString = "";                 // String to display

// Generally, you should use "unsigned long" for variables that hold time
unsigned long previousMillis = 0;      // will store last temp was read
const long interval = 2000;            // interval at which to read sensor
 
void setup() {
  Serial.begin(115200); // Begin Serial at 115200 Baud

  // Connect to WiFi network
  Serial.print("\n\r \n\rConnecting Wifi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.println("Weather Server");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handle_index); // Handle index page

  server.on("/temp", [](){
    getWxData();
    webString=String(myBME280.readTempF(), 0)+" F";
    server.send(200, "text/plain", webString);
  });

  server.on("/humid", [](){
    getWxData();
    webString=String(myBME280.readFloatHumidity())+" %";
    server.send(200, "text/plain", webString);
  });

  server.on("/pres", [](){
    getWxData();
    webString=String(myBME280.readFloatPressure() * 0.0002953)+" inHg";
    server.send(200, "text/plain", webString);
  });

  server.on("/alt", [](){
    getWxData();
    webString=String(myBME280.readFloatAltitudeFeet(), 0)+" ft";
    server.send(200, "text/plain", webString);
  });

  server.on("/co2", [](){
    getWxData();
    webString=String(myCCS811.getCO2())+" ppm";
    server.send(200, "text/plain", webString);
  });

  server.on("/tvoc", [](){
    getWxData();
    webString=String(myCCS811.getTVOC())+" ppb";
    server.send(200, "text/plain", webString);
  });

  server.begin();
  Serial.println("HTTP server started");

  Wire.begin();

  //This begins the CCS811 sensor and prints error status of .beginWithStatus()
  CCS811Core::CCS811_Status_e returnCode = myCCS811.beginWithStatus();
  Serial.print("CCS811 begin exited with: ");
  Serial.println(myCCS811.statusString(returnCode));

  // For I2C, enable the following and disable the SPI section
  myBME280.settings.commInterface = I2C_MODE;
  myBME280.settings.I2CAddress = 0x77;

  // Initialize BME280
  // For I2C, enable the following and disable the SPI section
  myBME280.settings.commInterface = I2C_MODE;
  myBME280.settings.I2CAddress = 0x77;
  myBME280.settings.runMode = 3; // Normal mode
  myBME280.settings.tStandby = 0;
  myBME280.settings.filter = 4;
  myBME280.settings.tempOverSample = 5;
  myBME280.settings.pressOverSample = 5;
  myBME280.settings.humidOverSample = 5;

  // Calling .begin() causes the settings to be loaded
  delay(10); // Make sure sensor had enough time to turn on. BME280 requires 2ms to start up.
  myBME280.begin();
}

void loop() {
  server.handleClient(); // Handling of incoming client requests
}

void handle_index() {
  server.send(200, "text/plain", "Wx Server \n\r /temp \r /humid \r /pres \r /alt \r /co2 \r /tvoc"); // Print title on index page
  delay(100);
}

void getWxData()
{
  // Wait at least 2 seconds seconds between measurements.
  // if the difference between the current time and last time you read
  // the sensor is bigger than the interval you set, read the sensor
  // Works better than delay for things happening elsewhere also
  unsigned long currentMillis = millis();

  if(currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis; // Save the last time you read the sensor

    // Check to see if data is available
    if (myCCS811.dataAvailable())
    {
      // Calling this function updates the global tVOC and eCO2 variables
      myCCS811.readAlgorithmResults();
/*
      // printInfoSerial fetches the values of tVOC and eCO2
      printInfoSerial();
*/
      float BMEtempC = myBME280.readTempC();
      float BMEhumid = myBME280.readFloatHumidity();
/*
      Serial.print("Applying new values (deg C, %): ");
      Serial.print(BMEtempC);
      Serial.print(",");
      Serial.println(BMEhumid);
      Serial.println();
*/
      // This sends the temperature data to the CCS811
      myCCS811.setEnvironmentalData(BMEhumid, BMEtempC);
    }
    else if (myCCS811.checkForStatusError())
    {
      // If the CCS811 found an internal error, print it.
      printSensorError();
    }
  }
}

//---------------------------------------------------------------
void printInfoSerial()
{
  Serial.println("BME280 data:");
  Serial.print(" Temperature: ");
  Serial.print(myBME280.readTempC(), 2);
  Serial.println(" degC");

  Serial.print(" Temperature: ");
  Serial.print(myBME280.readTempF(), 2);
  Serial.println(" degF");

  Serial.print(" %RH: ");
  Serial.print(myBME280.readFloatHumidity(), 2);
  Serial.println(" %");

  Serial.print(" Pressure: ");
  Serial.print(myBME280.readFloatPressure(), 2);
  Serial.println(" Pa");

  Serial.print(" Pressure: ");
  Serial.print((myBME280.readFloatPressure() * 0.0002953), 2);
  Serial.println(" inHg");

  Serial.print(" Altitude: ");
  Serial.print(myBME280.readFloatAltitudeMeters(), 0);
  Serial.println(" m");

  Serial.print(" Altitude: ");
  Serial.print(myBME280.readFloatAltitudeFeet(), 0);
  Serial.println(" ft");

  // getCO2() gets the previously read data from the library
  Serial.println("CCS811 data:");
  Serial.print(" CO2 concentration : ");
  Serial.print(myCCS811.getCO2());
  Serial.println(" ppm");

  // getTVOC() gets the previously read data from the library
  Serial.print(" TVOC concentration : ");
  Serial.print(myCCS811.getTVOC());
  Serial.println(" ppb");

  Serial.println();
}

// PrintSensorError gets, clears, then prints the errors
// saved within the error register.
void printSensorError()
{
  uint8_t error = myCCS811.getErrorRegister();

  if ( error == 0xFF ) // Comm error
  {
    Serial.println("Failed to get ERROR_ID register.");
  }
  else
  {
    Serial.print("Error: ");
    if (error & 1 << 5) Serial.print("HeaterSupply");
    if (error & 1 << 4) Serial.print("HeaterFault");
    if (error & 1 << 3) Serial.print("MaxResistance");
    if (error & 1 << 2) Serial.print("MeasModeInvalid");
    if (error & 1 << 1) Serial.print("ReadRegInvalid");
    if (error & 1 << 0) Serial.print("MsgInvalid");
    Serial.println();
  }
}
