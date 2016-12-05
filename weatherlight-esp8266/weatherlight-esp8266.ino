/*
 *  Weatherlight
 *
 *  You need to register and get a username and password at weatherlight.appspot.com.
 *  This code downloads the weather forecast and sets the LED color accordingly. Enjoy!
 *  
 *  Author: Sander Van Schoote <vanschoote.sander@gmail.com>
 *  Created: 4/12/2016
 */

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "Colors.h"
#include "RGBdriver.h"

#define CLK D7//pins definitions for the driver        
#define DIO D8

//initialize RGB driver
static RGBdriver Driver(CLK,DIO);
const int fadeResolution = 50;
static RGB currentColor = OFF;
extern const uint8_t gamma_table[];

const int BAUD_RATE = 115200;   //set serial console baud rate

//set up wifi details
const char* ssid     = "xxx";
const char* password = "yyy";
const char* loginUser = "aaa";
const char* loginPass = "bbb";


//set up weatherlight api details
const char* API_HOST = "weather-light.appspot.com";
const int API_PORT = 443;
const bool API_TOKEN_REQUIRED = true;
const char* FINGERPRINT = "06 34 A7 8F 52 4B 18 E8 72 B6 2F 2C 1C FF E7 9F E9 FF 72 7C"; // SHA1 fingerprint of the certificate of the apiService, use e.g. browser to find out
String privateKey;
bool authorized = false;

//global vars to hold the weatherlight (led) state
static bool ledEnabled = false;     //global var to hold global disabled/enabled flag of led bar
static int weatherCode = 0;    //default to 0, in case when there is e.g. no data available
static int tempMax = 0;        //default to 0, in case when there is e.g. no data available
static unsigned long lastConnectionTime = 0;      // last time you connected to the server, in milliseconds
const unsigned long DEFAULT_POLL_WAIT_TIME = 5 * 60 * 1000L;  //default is 5 minutes
static unsigned long pollWaitTime = DEFAULT_POLL_WAIT_TIME; //for storing the current value of the poll wait time interval
static bool force_update = false;


void setup() {
  Serial.begin(BAUD_RATE);
  delay(10);
  setup_wifi();
  delay(10);
  setup_weatherlight();
  delay(10);
}

void loop()  
{ 
  // if <requestInterval> seconds have passed since your last connection, then connect again and send data:
  if (millis() - lastConnectionTime > pollWaitTime || force_update) {
    force_update = false;
    getWeatherData();
  }
  if(ledEnabled){
    RGB targetColor = getLedColorForWeatherCode(weatherCode, tempMax);
    fadeToNewColor(targetColor,1000);
  }else{    
    fadeToNewColor(OFF,1000);
  }
}

void setup_wifi(){
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup_weatherlight(){
  //1. start with led switched off
  RGB currentColor = OFF;
  refresh();
  //2. initialize weather data for first time
  getWeatherData();
}

void refresh(){
  Driver.begin();
  RGB calibratedColor = calibrate(currentColor);
  //set the color
  Driver.SetColor(calibratedColor.r, calibratedColor.g, calibratedColor.b);
  Driver.end();
}

RGB calibrate(RGB color){
  RGB calibratedColor = color;
  //calibration, for some reason the green and blue are much stronger
  byte r = color.r;
  byte g = color.g/3.0;
  byte b = color.b/3.0;
  //gamma correction
  //r = pgm_read_byte(&gamma_table[r]);
  //g = pgm_read_byte(&gamma_table[g]);
  //b = pgm_read_byte(&gamma_table[b]);  
  calibratedColor.r = r;
  calibratedColor.g = g;
  calibratedColor.b = b;
  return calibratedColor;
}

void printColor(){
  Serial.println("Current LED colors:");
  Serial.print("-red  : ");Serial.println(currentColor.r);
  Serial.print("-green: ");Serial.println(currentColor.g);
  Serial.print("-blue : ");Serial.println(currentColor.b);
  Serial.println();
}

void fadeToNewColor(RGB &color, int fadeTime){ 
  if(color.r != currentColor.r || color.g != currentColor.g || color.b != currentColor.b){
    int timeElapsed = 0;
    //fade for fadeResolution-1 steps
    for(int i=fadeResolution;i>0;i--){     
       int t_step=(fadeTime-timeElapsed)/(float)i;
       byte r_step = (color.r-currentColor.r)/(float)i;
       byte g_step = (color.g-currentColor.g)/(float)i;
       byte b_step = (color.b-currentColor.b)/(float)i;
       currentColor = (RGB){(byte) currentColor.r+r_step,
                            (byte) currentColor.g+g_step,
                            (byte) currentColor.b+b_step};
       delay(t_step);
       refresh();   
       timeElapsed+=t_step;
       //Serial.print("t_step:");Serial.println(t_step);
       //Serial.print("r_step:");Serial.println(r_step);
       //Serial.print("g_step:");Serial.println(g_step);
       //Serial.print("b_step:");Serial.println(b_step);
    }
    //Serial.print("timeElapsed: ");Serial.println(timeElapsed);
    printColor();
  }
}

RGB getLedColorForWeatherCode(int weatherCode, int tempMax){
  if(tempMax>303){ //temp>303K ==> temp>30 degr C, this is a hot day
    return COLOR_RED;
  }
  //return codes according to this definition http://openweathermap.org/weather-conditions 
  RGB targetColor;
  switch(weatherCode){
    case 0 ... 199: //not a valid weather code
      targetColor = currentColor; //don't change color
      break;
    case 200 ... 299: //thunderstorm
      targetColor = COLOR_blueviolet;
      break;
    case 300 ... 399: //drizzle
      targetColor = COLOR_lightsteelblue;
      break;
   case 500 ... 599: //rain
      targetColor = COLOR_dodgerblue2;
      break;
    case 600 ... 699: //snow
      targetColor = COLOR_snow1;
      break;
    case 700 ... 799: //fog, mist, smoke
      targetColor = COLOR_gray31;
      break;
    case 800 ... 801: //clear or few clouds
      targetColor = COLOR_yellow1;
      break;
    case 802: //scattered clouds
      targetColor = COLOR_khaki1;
      break;
    case 803 ... 899: //clouds
      targetColor = COLOR_lightslategray;
      break;
    case 900 ... 999: //extreme and additionals
      targetColor = COLOR_darkslateblue;
      break;
   default:
      targetColor = currentColor; //don't change color
      break;
  }
  return targetColor;
}

String getDeviceId(){
    return String(ESP.getChipId())+"-"+String(ESP.getFlashChipId());
}

const uint8_t gamma_table[] PROGMEM = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };


void getJWToken() {
  boolean received = false;
  String path = "/v1/token";
  StaticJsonBuffer<400> jsonBuffer; //used to store server response

  //build json object to send data
  const int BUFFER_SIZE = JSON_OBJECT_SIZE(2);
  StaticJsonBuffer<BUFFER_SIZE> jsonRequestBuffer;

  JsonObject& loginRoot = jsonRequestBuffer.createObject();
  loginRoot["username"] = loginUser;
  loginRoot["password"] = loginPass;

  Serial.println("----------Retrieving JWT--------");
  Serial.print("Requesting URL: ");Serial.println(path);

  WiFiClientSecure client; // Use WiFiClient class to create TCP connections

  // This will send the request to the server
  postRequest(&client, path, loginRoot, false);

  //block until data available
  while (client.available() == 0)
  {
    if (client.connected() == 0)
    {
      return;
    }
  }
  Serial.print("response length:");
  Serial.println(client.available());

  client.setTimeout(1000); //in case things get stuck

  //read http header lines
  while (client.available()) {
    String line = client.readStringUntil('\n');
    Serial.println(line);
    if (line.length() == 1) { //empty line means end of headers
      break;
    }
  }

  //read first line of body (should be jwt)
  if (client.available()) {
    received = true;
    String line = client.readStringUntil('\n');
    Serial.println(line);
    if (line.startsWith("HTTP/1.1 401 Unauthorized")) {
      authorized = false;
      Serial.println("----------Login Failed----------");
      return;
    }
    else{
      const char* lineChars = line.c_str();
      JsonObject& root = jsonBuffer.parseObject(lineChars);
  
      if (!root.success()) {
        Serial.println("-------Parse Json Failed-------");
        return;
      }
  
      if (root.containsKey("jwt"))
      {
        JsonObject& jwt = root["jwt"];
        const char* access_token = jwt["token"];
        String token = String(access_token); //convert to String
        privateKey = token;
        Serial.print("-JWT token: ");
        Serial.println(token);
        Serial.println("----------JWT Updated----------");
  
      } else {
        Serial.println("----------Login Failed----------");
        return;
      }
      authorized = true; //login process complete
    }
  }
  if(received){
    client.stop();
  }
}

void getWeatherData() {
  boolean received = false;
  pollWaitTime = DEFAULT_POLL_WAIT_TIME; //in case something goes wrong, this will trigger a retry after DEFAULT_POLL_WAIT_TIME
  String path = "/v1/data";
  StaticJsonBuffer<400> jsonBuffer; //used to store server response
  
  Serial.println("----------Retrieving weather data--------");
  Serial.print("Requesting URL: ");Serial.println(path);

  // This will send the request to the server
  WiFiClientSecure client; // Use WiFiClient class to create TCP connections
  if (!authorized && API_TOKEN_REQUIRED) {
    getJWToken();
  }
  getRequest(&client, path, API_TOKEN_REQUIRED);

  //block until data available
  while (client.available() == 0)
  {
    if (client.connected() == 0)
    {
      return;
    }
  }
  Serial.print("response length:");
  Serial.println(client.available());

  //read http header lines
  while (client.available()) {
    String line = client.readStringUntil('\n');
    Serial.println(line);
    if (line.startsWith("HTTP/1.1 401 Unauthorized")) {
      authorized = false;
      force_update = true;
    }
    if (line.length() == 1) { //empty line means end of headers
      break;
    }
  }

  //read first line of body (should be data)
  if (client.available()) {
    lastConnectionTime = millis(); // update the time the last connection was made
    received = true;
    String line = client.readStringUntil('\n');
    const char* lineChars = line.c_str();
    JsonObject& root = jsonBuffer.parseObject(lineChars);

    if (!root.success()) {
      Serial.println("-------Parse Json Failed-------");
      return;
    }

    if (root.containsKey("data"))
    {
      JsonObject& data = root["data"];
      ledEnabled = data["status"].as<bool>();
      weatherCode = data["code"].as<int>();
      tempMax = data["temp"].as<int>();
      pollWaitTime = data["wait_poll"].as<int>() * 1000L;
      Serial.println();
      Serial.print("-ledEnabled: ");
      Serial.println(ledEnabled);
      Serial.print("-weather code: ");
      Serial.println(weatherCode);
      Serial.print("-temp max: ");
      Serial.println(tempMax);
      Serial.print("-pollWaitTime: ");
      Serial.println(pollWaitTime);
      Serial.println("----------Weather Data Updated----------");
    } else {
      Serial.println("----------Weather Data Failed----------");
      return;
    }
    if(received){
      client.stop();
    }
  }
}

void postRequest(WiFiClientSecure* client, String path, JsonObject& jsonRoot, bool needKey) {
  client->stop(); 
  client->setTimeout(2000); //in case things get stuck
  int response = client->connect(API_HOST, API_PORT);
  if (response>0) {
  // This will send the request to the server
    if (!client->verify(FINGERPRINT, API_HOST)) {
      Serial.println("certificate doesn't match");
      client->stop();
      return;
    }
    // This will send the request to the server
    client->print("POST ");client->print(path);client->println(" HTTP/1.1");
    client->print("Host: ");client->println(API_HOST);
    client->println("User-Agent: Esp8266WiFi/0.9");
    client->print("X-Device-ID: "); client->println(getDeviceId());
    if (needKey) {
      client->print("Authorization: JWT ");client->println(privateKey);
    }
    client->println("Accept: application/json");
    client->println("Content-Type: application/json");
    client->print("Content-Length: ");
  
    String dataStr;
    jsonRoot.printTo(dataStr);
  
    client->println(dataStr.length());
    client->println("Connection: close");
    client->println();
    //client->println(dataStr);
    jsonRoot.printTo(*client);
    client->println();
    delay(10);
  } else {
    // if you couldn't make a connection:
    Serial.print("connection to server failed:");
    switch(response){
      case -1:
        Serial.print("TIMED_OUT");
        break;
      case -2:
        Serial.print("INVALID_SERVER");
        break;
      case -3:
        Serial.print("TRUNCATED");
        break;
      case -4:
        Serial.print("INVALID_RESPONSE");
        break;
      default:
        Serial.print("UNKNOWN");
        break;
    }
  }
}

void getRequest(WiFiClientSecure* client, String path, bool needKey) {
  client->stop(); 
  client->setTimeout(2000); //in case things get stuck
  int response = client->connect(API_HOST, API_PORT);
  if (response>0) {
  // This will send the request to the server
    if (!client->verify(FINGERPRINT, API_HOST)) {
      Serial.println("certificate doesn't match");
      client->stop();
      return;
    }
    client->print("GET ");client->print(path);client->println(" HTTP/1.1");
    client->print("Host: ");client->println(API_HOST);
    client->println("User-Agent: Esp8266WiFi/0.9");
    client->print("X-Device-ID: "); client->println(getDeviceId());
    if (needKey) {
      client->print("Authorization: JWT ");client->println(privateKey);
    }
    client->println("Accept: application/json");
    client->println("Connection: close");
    client->println();
    delay(10);
  } else {
    // if you couldn't make a connection:
    Serial.print("connection to server failed:");
    switch(response){
      case -1:
        Serial.print("TIMED_OUT");
        break;
      case -2:
        Serial.print("INVALID_SERVER");
        break;
      case -3:
        Serial.print("TRUNCATED");
        break;
      case -4:
        Serial.print("INVALID_RESPONSE");
        break;
      default:
        Serial.print("UNKNOWN");
        break;
    }
  }
}


