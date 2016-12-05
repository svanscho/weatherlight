Weatherlight 
============

*Forecasting the weather in style*

Led strip indicating the weather of the next day with colors.

It has been written with Arduino and ESP8266 in mind, but it should be quite portable so you can use this code in any other C++ project.

<img style="float: right;"  src="https://github.com/svanscho/weatherlight/blob/master/images/2015-11-27%2011.00.51.jpg?raw=true" width="300" />

Works on
--------

* Arduino Uno
* ESP8266

Quick start
-----------

#### Openweathermap

In order to download weather forecasts we need an openweathermap appid (something like an api key). Go and create an account on the [openweathermap website](https://home.openweathermap.org/users/sign_up).

#### Google App Engine

In order to deploy the server component we need to create an GAE project. Go and create an account on the [GAE website](https://appengine.google.com).

#### Configure your service

The GAE service uses a config.yml file which you need to populate with some values for your project. Go ahead and copy the weatherlight-server/config.yml.default to weatherlight-server/config.yml.

Then edit with your favorite editor to set the appid, credentials and jwt secret as a minimum. Feel free to play with the other configuration values as well.

#### Deploy to GAE

Once the service is configured deploy the code to your brand new GAE project.

#### Flash your weatherlight

The firmware for the weatherlight needs some configuration values as well. 

Go ahead and modify the following values:
```c++
const char* API_HOST = "<your GAE project>.appspot.com";
const char* FINGERPRINT = "<sha1 fingerprint of ssl certificate>"; //GAE: "06 34 A7 8F 52 4B 18 E8 72 B6 2F 2C 1C FF E7 9F E9 FF 72 7C"
const char* ssid     = "<your wifi ssid>";
const char* password = "<your wifi password>";
const char* loginUser = "<your username>";
const char* loginPass = "<your password>";
```

That's it. Enjoy!

Found this project useful? Please star this project or [help me back with a donation!](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=P3RDCSF7FN5FW&lc=BE&item_name=Weatherlight&item_number=weatherlight&currency_code=AUD&bn=PP%2dDonationsBF%3abtn_donate_LG%2egif%3aNonHosted) :smile:





