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

// Wireless Network
/*#define WLAN_SSID        "CPB-BDR-Guest"           // cannot be longer than 32 characters!*/
/*#define WLAN_PASS        "BDRCO6450"*/
#define WLAN_SSID        "arduinoLine"           // cannot be longer than 32 characters!
#define WLAN_PASS        "testtest"
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY    WLAN_SEC_WPA2
// Amount of time to wait with no data received before closing the connection
#define IDLE_TIMEOUT_MS  1000
// Define the HOST and PAGE location
#define HOST             "starsdontcare.com"
// Device Information
#define deviceName       "Pizza%20Button"
// Chars for storing returned values
char confNum[32];
char response[32];

// Store HOST IP Address
uint32_t ip;

// Set up properities for the button
int buttonPin = A0;
int buttonState; // Used to store the button state in the loop
boolean buttonEngaged = true; // To prevent click events from occurring continually if button held down

// Lights
const int orderInitLight = A1; // Yellow light for when order is initiated
const int confLight = A3; // Blue light to signal order placed
const int errorLight = A4; // Red to notify Error

// Red lights for Order Status
const int state1 = 3;
const int state2 = 4; 
const int state3 = 5;
const int state4 = 6;
const int state5 = 8;

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
  if( !cc3000.checkConnected() ){
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
      newOrder();
    }
    else {
      newOrder();
    }
  }

  // Reset buttonEngaged if it is no longer pushed
  if( buttonState == HIGH && buttonEngaged ){
    buttonEngaged = false;
  }
}



/*
 * newOrder()
 *
 * Make Http request and print returned page
 */
void newOrder(){

  Serial.println("Make new order");
  // Signal that HTTP Request is being made
  digitalWrite(orderInitLight, HIGH);

  // Print HOST IP Address
  cc3000.printIPdotsRev(ip);
  Serial.println(ip);
  // Perform HTTP Request
  memset( &confNum, 0, 32 );
  httpRequest( true, strcat("POST /v1/orders/new/", deviceName) );

  Serial.println();
  Serial.print("Confirmation Number = ");
  Serial.println(confNum);
  Serial.println(F("-------------------------------------"));

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
  bool alertUser = true;
  buttonEngaged = false;
  char confAddr[] = "GET /v1/orders/confirm/";

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
      httpRequest( false, strcat(confAddr, confNum) );

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
  bool tracking = true;
  long interval = 4000;
  long startMillis = millis();
  int currentState = 1;
  int currentLight = state1;
  int clickCount = 0;
  buttonEngaged = false;

  Serial.println("Begin Tracking");

  while( tracking ){
    unsigned long currentMillis = millis();
    char statusAddr[] = "GET /v1/orders/status/";

    if( currentMillis - startMillis >= interval ) {
      httpRequest( false, strcat( statusAddr, confNum ) );

      int intResponse = atoi(response);
      Serial.print("New response ========== ");
      Serial.println(intResponse);

      switch( intResponse ){
        case 0:
          break;
        case 1:
          digitalWrite(state1, HIGH);
          currentLight = state2;
          break;
        case 2:
          digitalWrite(state2, HIGH);
          currentLight = state3;
          break;
        case 3:
          digitalWrite(state3, HIGH);
          currentLight = state4;
          break;
        case 4:
          digitalWrite(state4, HIGH);
          currentLight = state5;
          break;
        case 5:
          digitalWrite(state5, HIGH);
          currentLight = 0;
          break;
        case 6:
          tracking = false;
          break;
        default:
          Serial.println("DEFAULT RESPONSE??");
          tracking = false;
          break;
      }

      startMillis = millis();
    }

    if( currentMillis - previousMillis > alertInterval ){
      previousMillis = currentMillis;

      if( ledState == LOW)
        ledState = HIGH;
      else
        ledState = LOW;

      digitalWrite( currentLight, ledState );
    }
  }
  resetParams();
  Serial.println("END OF TRACKING");
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
}

/*
 * httpRequest( bool newOrder, String request )
 *
 */
void httpRequest( bool newOrder, char request[64] )
{
  bool startRead = false;
  int stringPos = 0;
  char parsedReturn[32];
  // Clear anything stored in response
  Serial.println(request);
  memset( &response, 0, 32 );

  Adafruit_CC3000_Client www = cc3000.connectTCP(ip, 3000);
  if (www.connected()) {
    www.fastrprint(request);
    www.fastrprint(F(" HTTP/1.1\r\n"));
    www.fastrprint(F("Host: 104.131.136.115:3000\r\n"));
    www.fastrprint(F("User-Agent: ArduinoWiFi/1.1\r\n"));
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
      if ( c == '{' ){
        startRead = true;
      }
      else if ( startRead ){
        if ( c != '}' && newOrder ){
          confNum[stringPos] = c;
          stringPos++;
        }
        else if ( c != '}' && !newOrder ){
          response[stringPos] = c;
          stringPos++;
        }
        else {
          startRead = false;
        }
      }

      lastRead = millis();
    }
  }
  www.close();
  Serial.println();
  Serial.println(F("-------------------------------------"));
}


/*
 * errorHandling()
 *
 */
void errorHandling()
{
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
 * resetParams()
 *
 */
void resetParams()
{
  digitalWrite( state1, LOW );
  digitalWrite( state2, LOW );
  digitalWrite( state3, LOW );
  digitalWrite( state4, LOW );
  digitalWrite( state5, LOW );
  digitalWrite( orderInitLight, LOW );
  digitalWrite( confLight, LOW );
  digitalWrite( errorLight, LOW );

  // You need to make sure to clean up after yourself or the CC3000 can freak out
  // the next time your try to connect ...
  Serial.println(F("\n\nDisconnecting"));
  cc3000.disconnect();

}
