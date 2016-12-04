#include <SPI.h>
#include <Ethernet.h>
#include "RGBdriver.h"
#include "Colors.h"
#define CLK 7//pins definitions for the driver        
#define DIO 8

extern const uint8_t gamma[];

// Initialize the Ethernet client library
byte mac[] = { 0x90, 0xA2, 0xDA, 0x00, 0x6D, 0x7E }; // Enter a MAC address for your controller
char weatherlightApiService[] = "weather-light.appspot.com";    // name address for API server (using DNS)
char apiKey[] = "xxx"
IPAddress ip(10, 26, 12, 90); // Set the static IP address to use if the DHCP fails to assign
EthernetClient client;
unsigned long lastConnectionTime = 0;      // last time you connected to the server, in milliseconds
unsigned long pollWaitTime = 300L * 1000L; //for storing the current value of the poll wait time interval, default is 5 minutes

// Initialize the RGB driver library
RGBdriver Driver(CLK,DIO);
int fadeResolution = 50;
RGB currentColor = { 0 , 0 , 0 };

//global vars to hold the weather and the time (hours only)
int ledEnabled = 0;     //global var to hold global disabled/enabled flag of led bar
int weatherCode = 0;    //default to 0, in case when there is e.g. no data available
int tempMax = 0;        //default to 0, in case when there is e.g. no data available

void setup()  
{ 
  refresh();
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip);
    client.setTimeout(500);
  }
  delay(1000); // give the Ethernet shield a second to initialize
  Serial.print("* Weatherlight IP address: ");Serial.println(Ethernet.localIP()); 
  requestWeatherData(); //perform this once at startup to have immediate weather and timing data
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
  //r = pgm_read_byte(&gamma[r]);
  //g = pgm_read_byte(&gamma[g]);
  //b = pgm_read_byte(&gamma[b]);  
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
                            (byte) currentColor.b+b_step,};
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

void loop()  
{ 
  // if <requestInterval> seconds have passed since your last connection, then connect again and send data:
  if (millis() - lastConnectionTime > pollWaitTime) {
    requestWeatherData();
  }
  retrieveWeatherData();
  if(ledEnabled){
    RGB targetColor = getLedColorForWeatherCode(weatherCode, tempMax);
    fadeToNewColor(targetColor,1000);
  }else{
    fadeToNewColor(OFF,1000);
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

void retrieveWeatherData() {
  boolean received = false;
  // if there's incoming data from the net connection, store it
  while(client.connected()) {
    while (client.available()) {
      received = true;
      char c = client.read();
      if (c=='\r'){ // \r is received
        c = client.read();
        if (c=='\n'){ // \r\n is received
          c = client.read();
          if (c=='\r'){ // \r\n\r is received
            c = client.read();
            if (c=='\n'){ // \r\n\r\n is received, payload is coming
              //payload format: "status;code;temp;wait_poll"
              //example: 0;802;299;32823
              char enabled[10];
              char weather[10];
              char temp[10];
              char wait_poll[10];
              char* dataSequence[] = {enabled, weather, temp, wait_poll};
              int numberOfSemicolonsfound = 0;
              char* p = dataSequence[numberOfSemicolonsfound];
              Serial.write("<-- ");
              while (client.available()) { //read whole payload
                char c = client.read();
                Serial.write(c);
                if(c==';'){
                  numberOfSemicolonsfound++;
                  if(numberOfSemicolonsfound<sizeof(dataSequence)){
                    p=dataSequence[numberOfSemicolonsfound]; //move to next array (enabled->weather->temp->wait_poll)
                  }
                } else{
                  *p=c;
                  p++;
                }
              }
              Serial.println();Serial.println();
              ledEnabled = atoi(enabled);
              weatherCode = atoi(weather);
              tempMax = atoi(temp);
              pollWaitTime = atoi(wait_poll) * 1000L;
            }
          }
        }
      }
    }
  } 
  if(received){
    client.stop();
    Serial.println("Received data with the following values:");
    Serial.print("-ledEnabled: ");
    Serial.println(ledEnabled);
    Serial.print("-weather code: ");
    Serial.println(weatherCode);
    Serial.print("-temp max: ");
    Serial.println(tempMax);
    Serial.print("-pollWaitTime: ");
    Serial.println(pollWaitTime);
    Serial.println();
  }
}

// this method makes a HTTP connection to the server:
void requestWeatherData() {
  // close any connection before sending a new request.
  // This will free the socket on the ethernet shield
  client.stop(); 
  // if there's a successful connection:
  int response = client.connect(weatherlightApiService, 80);
  if (response>0) {
    //Serial.println("connecting to weather api server...");
    // send the HTTP GET request:
    Serial.println("--> GET /v1/statusandweather HTTP/1.1");
    client.println("GET /v1/statusandweather HTTP/1.1");
    client.print("Host: "); client.println(weatherlightApiService);
    client.println("User-Agent: arduino-ethernet");
    client.print("X-Device-ID: "); client.println(getDeviceId());
    client.println("Connection: close");
    client.println();

    // note the time that the connection was made:
    lastConnectionTime = millis();
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
    Serial.print("[response=");Serial.print(response);Serial.println("]");
  }
}


char* getDeviceId(){
    return (char*)mac;
}

const uint8_t PROGMEM gamma[] = {
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



