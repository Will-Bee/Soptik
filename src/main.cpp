#include <Arduino.h>

// ----------------------------------------------
// Display includes and settings
// ----------------------------------------------

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_I2CDevice.h>

#define OLED_RESET 0  // GPIO0
Adafruit_SSD1306 display(OLED_RESET);



// ----------------------------------------------
// WiFi includes and settings
// ----------------------------------------------

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>

#include <ArduinoJson.h>

WiFiManager wifiManager;

WiFiClientSecure client;
HTTPClient http;
String JSONDetected;
String JSONEmpty;
String JSON;
StaticJsonDocument<220> messageDetected;
StaticJsonDocument<220> messageEmpty;



// ----------------------------------------------
// Function and variable declarations here:
// ----------------------------------------------

void displayIntro();
void displayConnected();
void displayGettingWebhook();
void getWebhookLink();
void messageBuild();
void displayStatus(bool status);
void displayDone();
int notification(bool status);
void displayNetworkError();

int notifState;
int httpCode;
String webhookLink = "";
String payload;
String yourApi = "https://www.bartosek.cz/shared/OFD/api/?secret=heslo";

int sensorPin = 2;

bool state = false;
int oldState = 2;




// ----------------------------------------------
// Setup and loop functions
// ----------------------------------------------

void setup() {
  // put your setup code here, to run once:
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);


  displayIntro();
  wifiManager.autoConnect("AutoConnectAP");
  client.setInsecure();
  displayConnected();

  displayGettingWebhook();
  getWebhookLink();
  displayDone();

  client.connect(webhookLink, 443);
  http.begin(client, webhookLink);

  messageBuild();

  pinMode(sensorPin, INPUT);


}

void loop() {

  state = digitalRead(sensorPin);

  if (state != oldState) {
      notifState = notification(state);
    }

  oldState = state;

  if (notifState == 0) {

    displayNetworkError();
    delay(1000);
    ESP.restart();

  }

}




// ----------------------------------------------
// Display functions
// ----------------------------------------------

void displayIntro() {

  display.clearDisplay();
  display.display();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Github/   Will-Bee                      Connectingto WiFi..");

  display.display();

}


void displayNetworkError() {

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Github/   Will-Bee                      Network error !");

  display.display();

  delay(1000);
}



void displayConnected() {

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Github/   Will-Bee                      Connected to WiFi !");

  display.display();

  delay(1000);
}



void displayGettingWebhook() {

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Github/   Will-Bee            - Getting webhook   link..");

  display.display();

}



void displayDone() {

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Github/   Will-Bee            SETUP DONE");

  display.display();

}



void displayStatus(bool status) {

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);

  if (status) {
    display.println("Github/   Will-Bee                      Status:   DETECTED");
  } else {
    display.println("Github/   Will-Bee                      Status:   EMPTY");
  }

  display.display();

}



// ----------------------------------------------
// Building JSON Object to send
// ----------------------------------------------

void messageBuild(){

  messageDetected["content"] = "@everyone";
  messageDetected["username"] = "OFD-Device";
  messageDetected["embeds"][0]["description"] = "OPTICAL FIBER DETECTOR";

  messageEmpty["content"] = "@everyone";
  messageEmpty["username"] = "OFD-Device";
  messageEmpty["embeds"][0]["description"] = "OPTICAL FIBER DETECTOR";

  messageDetected["embeds"][0]["title"] = "Status: DETECTED";
  messageDetected["embeds"][0]["color"] = "5763719";

  messageEmpty["embeds"][0]["title"] = "Status: EMPTY";
  messageEmpty["embeds"][0]["color"] = "15548997";

  serializeJson(messageDetected, JSONDetected);
  serializeJson(messageEmpty, JSONEmpty);

}



// ----------------------------------------------
// Getting webhook link from private API
// ----------------------------------------------

void getWebhookLink() {

  http.begin(client, yourApi);
  httpCode = http.GET();

  if (httpCode > 0) {
      String payload = http.getString();
      client.stop();

      // All "/" are \/ in response, so we need to replace them with /
      payload.replace("\\/", "/");

      // Example of response:
      // {"URL":"https://discord.com/api/webhooks/123456789/123456789","text":"test"}

      // Parse JSON without using ArduinoJson
      // Get webhook link
      int start = payload.indexOf("https://discord.com/api/webhooks/");
      int end = payload.indexOf("\",\"text\"");
      webhookLink = payload.substring(start, end);

      // Cut this part from webhook: ","text":"...
      webhookLink = webhookLink.substring(0, webhookLink.indexOf("\",\"text\""));



    } else {
      // fail
    }
}



// ----------------------------------------------
// Sending message to discord
// ----------------------------------------------

int notification(bool status){
  displayStatus(status);
  // Send POST request to Discord webhook

  http.addHeader("Content-Type", "application/json");

  if (status) {
      httpCode = http.POST(JSONDetected);
  } else {
      httpCode = http.POST(JSONEmpty);
  }

  // Check if device is connected to internet
  if (WiFi.status() != WL_CONNECTED) {
    // end program if not connected
    return 0;
  }

  return 1;
}