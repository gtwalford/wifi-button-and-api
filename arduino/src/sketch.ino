/*
 * Wifi IoT Button
 *
 * Using the CC3000 Wifi shield connect to sever and initiate an action
 * Once request is initiated on the server make a second request for confirmation
 * Then notify the user of the status
 *
 * Uses Adafruit CC3000 Library for wireless access - https://github.com/adafruit/Adafruit_CC3000_Library
 *
 */

#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <string.h>
#include "utility/debug.h"

// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   2  // MUST be an interrupt pin!
#define ADAFRUIT_CC3000_VBAT  7
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIVIDER);

#define WLAN_SSID        "WIFI_NETWORK"           // cannot be longer than 32 characters!
#define WLAN_PASS        "WIFI_PASSWORD"
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY    WLAN_SEC_WPA2
// Amount of time to wait with no data received before closing the connection
#define IDLE_TIMEOUT_MS  3000      
// Define the HOST and PAGE location
#define HOST             "HOST_WEBSITE"
#define WEBPAGE          "PAGE_LOCATION"
// Store HOST IP Address
uint32_t ip;

// Set up properities for the button
int buttonPin = A0;
int buttonState; // Used to store the button state in the loop
boolean buttonEngaged = true; // To prevent click events from occurring continually if button held down

// Lights
const int orderInitLight = A1; // Yellow light for when order is initiated
const int confLight = A2; // Blue light to signal order placed
const int errorLight = A3; // Red to notify Error

// Red lights for Order Status
const int state1 = 3;
const int state2 = 4; 
const int state3 = 5;
const int state4 = 6;
const int state5 = 9;

// Setup for flashing lights
int ledState = LOW;
const int alertInterval = 500;
long previousMillis = 0;



/*
 * setup()
 *
 */
void setup()
{
  Serial.begin(115200);
  Serial.println("--------------------------");
  Serial.println("Initialize Device");
  Serial.println("--------------------------");

  pinMode(buttonPin, INPUT);
  pinMode(orderInitLight, OUTPUT);
  pinMode(confLight, OUTPUT);
  pinMode(errorLight, OUTPUT);
  pinMode(state1, OUTPUT);
  pinMode(state2, OUTPUT);
  pinMode(state3, OUTPUT);
  pinMode(state4, OUTPUT);
  pinMode(state5, OUTPUT);

  digitalWrite(orderInitLight, LOW);
  digitalWrite(confLight, LOW);
  digitalWrite(errorLight, LOW);
  digitalWrite(state1, LOW);
  digitalWrite(state2, LOW);
  digitalWrite(state3, LOW);
  digitalWrite(state4, LOW);
  digitalWrite(state5, LOW);

  wifiConnect();

  // Verify wifi connected
  if( cc3000.checkConnected() ){
    Serial.print("Connected to ");
    Serial.println(WLAN_SSID);
  }
  else{
    Serial.println("Error! Connection Failed");
    // Call errorHanding to notify user and reinitialize the program
    errorHandling();
  }

  Serial.println("Setup Complete");

}


/*
 * loop()
 *
 */
void loop(){

  // Cache button state per loop
  buttonState = digitalRead(buttonPin);

  // If button is pushed and is not already engaged
  if( buttonState == LOW && !buttonEngaged ){
    // Engage button to prevent multiple clicks
    buttonEngaged = true;
    // Check Wifi and establish new connection if not connected
    if( cc3000.checkConnected() == 0 ){
      wifiConnect();
      httpRequest();
    }
    else {
      httpRequest();
    }
  }

  // Reset buttonEngaged if it is no longer pushed
  if( buttonState == HIGH && buttonEngaged ){
    buttonEngaged = false;
  }
}


/*
 * wifiConnect()
 *
 * Initialize CC3000 and connect to the network
 */
void wifiConnect(){
  // Turn on the Yellow/Blue/Red lights to signal attempting to connect
  digitalWrite(orderInitLight, HIGH);
  digitalWrite(confLight, HIGH);
  digitalWrite(errorLight, HIGH);

  // Initialize the CC3000 module
  Serial.println(F("\nInitializing..."));
  if (!cc3000.begin())
  {
    Serial.println(F("Couldn't begin()! Check your wiring?"));
    errorHandling();
    while(1);
  }
  
  // Connect to wireless network
  Serial.print(F("\nAttempting to connect to "));
  Serial.println(WLAN_SSID);
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F("Failed!"));
    errorHandling();
    while(1);
  }
  else{
    Serial.println(F("Connected!"));
  }
   
  
  // Wait for DHCP to complete
  Serial.println(F("Request DHCP"));
  while (!cc3000.checkDHCP())
  {
    delay(100); // ToDo: Insert a DHCP timeout!
  }  

  // Turn off lights when connection complete
  digitalWrite(orderInitLight, LOW);
  digitalWrite(confLight, LOW);
  digitalWrite(errorLight, LOW);

}

/*
 * httpRequest()
 *
 * Make Http request and print returned page
 */
void httpRequest(){

  // Signal that HTTP Request is being made
  digitalWrite(orderInitLight, HIGH);
  // Clear anything stored in ip
  ip = 0;

  // Try looking up the website's IP address
  Serial.print(HOST); 
  Serial.print(F(" = "));
  while (ip == 0) {
    if (! cc3000.getHostByName(HOST, &ip)) {
      Serial.println(F("Couldn't resolve!"));
      errorHandling();
    }
    delay(500);
  }

  // Print HOST IP Address
  cc3000.printIPdotsRev(ip);

  // Perform HTTP Request
  Adafruit_CC3000_Client www = cc3000.connectTCP(ip, 80);
  if (www.connected()) {
    www.fastrprint(F("GET "));
    www.fastrprint(WEBPAGE);
    www.fastrprint(F(" HTTP/1.1\r\n"));
    www.fastrprint(F("Host: ")); www.fastrprint(HOST); www.fastrprint(F("\r\n"));
    www.fastrprint(F("User-Agent: ArduinoWiFi/1.1\r\n"));
    // Authorization Required in the request if behind firewall
    // Username and password passed through with base64 encrytion for HTTP
    // www.fastrprint(F("Authorization: Basic dXNlcm5hbWU6cGFzc3dvcmQ=\r\n"));
    www.fastrprint(F("\r\n"));
    www.println();
  } else {
    Serial.println(F("Connection failed"));    
    errorHandling();
    return;
  }

  // Read data until either the connection is closed or the idle timeout is reached
  unsigned long lastRead = millis();
  while (www.connected() && (millis() - lastRead < IDLE_TIMEOUT_MS)) {
    while (www.available()) {
      char c = www.read();
      Serial.print(c);
      lastRead = millis();
    }
  }
  www.close();
  Serial.println();
  Serial.println(F("-------------------------------------"));
  
  // You need to make sure to clean up after yourself or the CC3000 can freak out
  // the next time your try to connect ...
  Serial.println(F("\n\nDisconnecting"));
  cc3000.disconnect();

  // Once complete request confirmation from the user
  requestConfirmation();

}


/*
 * requestConfirmation()
 * 
 * After initial Request is made request confirmation from the user
 * TODO: Perform second HTTP request to signal confirmation was made
 */
void requestConfirmation(){
  boolean alertUser = true;
  buttonEngaged = false;

  // While waiting for the user to click the button to confirm flash Yellow light to notify
  while( alertUser ){
    unsigned long currentMillis = millis();

    if( currentMillis - previousMillis > alertInterval ){
      previousMillis = currentMillis;

      if( ledState == LOW)
        ledState = HIGH;
      else
        ledState = LOW;

      digitalWrite( orderInitLight, ledState );
    }

    if( digitalRead( buttonPin ) == LOW && !buttonEngaged ){
      buttonEngaged = true;
      alertUser = false;
      digitalWrite( orderInitLight, LOW );
      digitalWrite( confLight, HIGH );
      Serial.println("Confirmation Clicked");

      trackOrder();

    }
  }
}


void trackOrder(){
  boolean tracking = true;
  long interval = 20;
  long statusMillis = 0;
  int currentState = 0;
  int currentLight = state1;
  int clickCount = 0;
  buttonEngaged = false;

  Serial.println("Begin Tracking");

  while( tracking ){
    unsigned long currentMillis = millis();

    if( statusMillis >= interval ) {
      Serial.println("Increase State - ");
      currentState++;
      Serial.println(currentState);

      switch( currentState ){
        case 0:
          Serial.println("Case 0");
          break;
        case 1:
          Serial.println("Case 1");
          digitalWrite(state1, HIGH);
          currentLight = state2;
          break;
        case 2:
          Serial.println("Case 2");
          digitalWrite(state2, HIGH);
          currentLight = state3;
          break;
        case 3:
          Serial.println("Case 3");
          digitalWrite(state3, HIGH);
          currentLight = state4;
          break;
        case 4:
          Serial.println("Case 4");
          digitalWrite(state4, HIGH);
          currentLight = state5;
          break;
        case 5:
          Serial.println("Case 5");
          digitalWrite(state5, HIGH);
          currentLight = 0;
          break;
        case 6:
          Serial.println("Case 6");
          tracking = false;
          break;
        default:
          Serial.println("DEFAULT RESPONSE??");
          tracking = false;
          break;
      }

      Serial.println(currentLight);
      statusMillis = 0;
    }

    if( currentMillis - previousMillis > alertInterval ){
      previousMillis = currentMillis;
      statusMillis++;

      if( ledState == LOW)
        ledState = HIGH;
      else
        ledState = LOW;

      digitalWrite( currentLight, ledState );
    }

    if( digitalRead( buttonPin ) == LOW && !buttonEngaged ){
      Serial.print("Button Clicked = ");
      buttonEngaged = true;
      clickCount++;
      Serial.println(clickCount);
    }

    if( digitalRead( buttonPin ) == HIGH && !buttonEngaged ){
      Serial.println("Button Reset");
      buttonEngaged = false;
    }

    if( clickCount == 4 ){
      tracking = false;
      Serial.println("Break Tracking Clicked");
      
      digitalWrite( state1, LOW );
      digitalWrite( state2, LOW );
      digitalWrite( state3, LOW );
      digitalWrite( state4, LOW );
      digitalWrite( state5, LOW );
      digitalWrite( confLight, LOW );
    }
  }
  resetParams();
  Serial.println("END OF TRACKING");
}


void resetParams(){
  Serial.println("RESET BUTTON");
  
  digitalWrite( state1, LOW );
  digitalWrite( state2, LOW );
  digitalWrite( state3, LOW );
  digitalWrite( state4, LOW );
  digitalWrite( state5, LOW );
  digitalWrite( orderInitLight, LOW );
  digitalWrite( confLight, LOW );
  digitalWrite( errorLight, LOW );

}


void errorHandling(){
  boolean errorAlert = true;
  buttonEngaged = false;
  resetParams();

  while( errorAlert ){
    unsigned long currentMillis = millis();

    if(currentMillis - previousMillis > alertInterval) {
      // save the last time you blinked the LED 
      previousMillis = currentMillis;   

      // if the LED is off turn it on and vice-versa:
      if (ledState == LOW)
        ledState = HIGH;
      else
        ledState = LOW;

      // set the LED with the ledState of the variable:
      digitalWrite(errorLight, ledState);
    }

    if( digitalRead(buttonPin) == LOW && !buttonEngaged ){
      buttonEngaged = true;
      errorAlert = false;
      setup();
    }
  }

}


/*
 * From Adafruit example incase needed later
 */
void listSSIDResults(void)
{
  uint8_t valid, rssi, sec, index;
  char ssidname[33]; 

  index = cc3000.startSSIDscan();

  Serial.print(F("Networks found: "));
  Serial.println(index);
  Serial.println(F("================================================"));

  while (index) {
    index--;

    valid = cc3000.getNextSSID(&rssi, &sec, ssidname);
    
    Serial.print(F("SSID Name    : ")); 
    Serial.print(ssidname);
    Serial.println();
    Serial.print(F("RSSI         : "));
    Serial.println(rssi);
    Serial.print(F("Security Mode: "));
    Serial.println(sec);
    Serial.println();
  }
  Serial.println(F("================================================"));

  cc3000.stopSSIDscan();
}


/*
 * From Adafruit example incase needed later
 */
bool displayConnectionDetails(void)
{
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;
  
  if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
    Serial.println(F("Unable to retrieve the IP Address!\r\n"));
    return false;
  }
  else
  {
    Serial.print(F("\nIP Addr: ")); 
    cc3000.printIPdotsRev(ipAddress);
    Serial.println();
    Serial.print(F("\nNetmask: ")); cc3000.printIPdotsRev(netmask);
    Serial.println();
    Serial.print(F("\nGateway: ")); cc3000.printIPdotsRev(gateway);
    Serial.println();
    Serial.print(F("\nDHCPsrv: ")); cc3000.printIPdotsRev(dhcpserv);
    Serial.println();
    Serial.print(F("\nDNSserv: ")); cc3000.printIPdotsRev(dnsserv);
    Serial.println();
    return true;
  }
}
