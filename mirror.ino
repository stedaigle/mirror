#include <Wire.h>
#include <SFE_MicroOLED.h>
#include <SparkFunBME280.h>

// Global sensor objects
BME280 myBME280;

// Variables to hold the parsed data
boolean newData = false;
const byte numChars = 128;
char receivedChars[numChars];
char tempChars[numChars]; // Temporary array for use when parsing
char temp[16] = {0};
char humid[16] = {0};
char pres[16] = {0};
char alt[16] = {0};
char co2[16] = {0};
char tvoc[16] = {0};
char dest1[16] = {0};
char time1[16] = {0};
char dist1[16] = {0};
char dest2[16] = {0};
char time2[16] = {0};
char dist2[16] = {0};

unsigned long seconds = 1000L;
unsigned long minutes = seconds * 60;
String tempString = "";

//////////////////////////
// MicroOLED Definition //
//////////////////////////
// The library assumes a reset pin is necessary. The Qwiic OLED has RST hard-wired, so pick an arbitrarty IO pin that is not being used
#define PIN_RESET 9
// The DC_JUMPER is the I2C Address Select jumper. Set to 1 if the jumper is open (default), or set to 0 if it's closed.
#define DC_JUMPER_0 0
#define DC_JUMPER_1 1

//////////////////////////////////
// MicroOLED Object Declaration //
//////////////////////////////////
MicroOLED oled_0(PIN_RESET, DC_JUMPER_0);    // I2C declaration for OLED 0
MicroOLED oled_1(PIN_RESET, DC_JUMPER_1);    // I2C declaration for OLED 1

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);
  Serial.println("\n\rIntelligent Mirror Powered by Artemis ATP");
  Serial.println("Waiting for data stream");
  delay(100);
  Wire.begin();
  oled_0.begin();     // Initialize the OLED
  oled_1.begin();
  oled_0.clear(ALL);  // Clear the display's internal memory
  oled_1.clear(ALL);
  oled_0.display();   // Display what's in the buffer (splashscreen)
  oled_1.display();
  delay(1000);
  oled_0.clear(PAGE); // Clear the buffer
  oled_1.clear(PAGE);

  if (myBME280.beginI2C() == false) // Begin communication over I2C
  {
    ////Serial.println("The sensor did not respond. Please check wiring.");
    while(1); // Freeze
  }
}

void loop() {
  recvWithStartEndMarkers();
  if (newData == true) {
    // This temporary copy is necessary to protect the original data
    // because strtok() used in parseData() replaces the commas with \0
    strcpy(tempChars, receivedChars);
    parseData();
    // Print data to serial monitor
    Serial.println(temp);
    Serial.println(humid);
    Serial.println(pres);
    Serial.println(alt);
    Serial.println(co2);
    Serial.println(tvoc);
    Serial.println(dest1);
    Serial.println(time1);
    Serial.println(dist1);
    Serial.println(dest2);
    Serial.println(time2);
    Serial.println(dist2);
    newData = false;
  }

  printTitle_0(" INSIDE","  TEMP");
  printTitle_1(" WORK","TRAFFIC");
  delay(2000);
  oled_0.clear(PAGE);
  oled_1.clear(PAGE);
  oled_0.setFontType(3);
  oled_0.setCursor(0, 0);   // Set cursor to top-middle-left
  tempString = String(myBME280.readTempF(), 0);
  oled_0.print(tempString);
  oled_0.setCursor(32, 32); // Set cursor to bottom-middle
  oled_0.setFontType(1);
  oled_0.print("F");
  oled_0.display();         // Draw on the screen
  oled_1.setFontType(3);
  oled_1.setCursor(0, 0);   // Set cursor to top-middle-left
  oled_1.print(time1);
  oled_1.setCursor(32, 32); // Set cursor to bottom-middle
  oled_1.setFontType(1);
  oled_1.print("MIN");
  oled_1.display();         // Draw on the screen
  delay(10000);

  printTitle_0("OUTSIDE"," TEMP");
  printTitle_1(" SCHOOL","TRAFFIC");
  delay(2000);
  oled_0.clear(PAGE);
  oled_1.clear(PAGE);
  oled_0.setFontType(3);
  oled_0.setCursor(0, 0);   // Set cursor to top-middle-left
  oled_0.print(temp);
  oled_0.setCursor(32, 32); // Set cursor to bottom-middle
  oled_0.setFontType(1);
  oled_0.print("F");
  oled_0.display();         // Draw on the screen
  oled_1.setFontType(3);
  oled_1.setCursor(0, 0);   // Set cursor to top-middle-left
  oled_1.print(time2);
  oled_1.setCursor(32, 32); // Set cursor to bottom-middle
  oled_1.setFontType(1);
  oled_1.print("MIN");
  oled_1.display();         // Draw on the screen
  delay(10000);
}

// Print a small title
void printTitle_0(String title1, String title2) {
  oled_0.clear(PAGE);
  oled_0.setFontType(1);
  // Try to set the cursor in the middle of the screen
  oled_0.setCursor(0,8);
  // Print the title:
  oled_0.print(title1);
  oled_0.setCursor(0,24);
  oled_0.print(title2);
  oled_0.display();
}

void printTitle_1(String title1, String title2) {
  oled_1.clear(PAGE);
  oled_1.setFontType(1);
  oled_1.setCursor(0,8);
  oled_1.print(title1);
  oled_1.setCursor(0,24);
  oled_1.print(title2);
  oled_1.display();
}

void recvWithStartEndMarkers() {
  static boolean recvInProgress = false;
  static byte ndx = 0;
  char startMarker = '<';
  char endMarker = '>';
  char rc;

  while (Serial1.available() > 0 && newData == false) {
    rc = Serial1.read();
    if (recvInProgress == true) {
      if (rc != endMarker) {
        receivedChars[ndx] = rc;
        ndx++;
        if (ndx >= numChars) {
          ndx = numChars - 1;
        }
      } else {
        receivedChars[ndx] = '\0'; // terminate the string
        recvInProgress = false;
        ndx = 0;
        newData = true;
      }
    } else if (rc == startMarker) {
      recvInProgress = true;
    }
  }
}

void parseData() {   // Split the data into its parts
  char * strtokIndx; // This is used by strtok() as an index

  strtokIndx = strtok(tempChars,","); // Get the first part - the string
  strcpy(temp, strtokIndx);           // Copy it to string
 
  strtokIndx = strtok(NULL, ",");     // This continues where the previous call left off
  strcpy(humid, strtokIndx);          // Copy this part to a string

  strtokIndx = strtok(NULL, ",");
  strcpy(pres, strtokIndx);

  strtokIndx = strtok(NULL, ",");
  strcpy(alt, strtokIndx);

  strtokIndx = strtok(NULL, ",");
  strcpy(co2, strtokIndx);

  strtokIndx = strtok(NULL, ",");
  strcpy(tvoc, strtokIndx);

  strtokIndx = strtok(NULL, ",");
  strcpy(dest1, strtokIndx);

  strtokIndx = strtok(NULL, ",");
  strcpy(time1, strtokIndx);

  strtokIndx = strtok(NULL, ",");
  strcpy(dist1, strtokIndx);

  strtokIndx = strtok(NULL, ",");
  strcpy(dest2, strtokIndx);

  strtokIndx = strtok(NULL, ",");
  strcpy(time2, strtokIndx);

  strtokIndx = strtok(NULL, ",");
  strcpy(dist2, strtokIndx);
}
