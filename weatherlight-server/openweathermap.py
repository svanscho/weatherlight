import urllib2
import json
from datetime import datetime
import pytz
import logging

class Openweathermap(object):

    def __init__(self, appid):
        self.appid = appid #the openweathermaps application id you got when signing up for the API

    #return codes according to this definition http://openweathermap.org/weather-conditions
    def getForecastForDay(self, latitude, longitude, day):
        json_object = self.__forecast(latitude, longitude)
        days = json_object["list"]
        days = sorted(days, key=lambda days: days["dt"])        #sort by date
        return days[day]["weather"][0]["id"], int(round(days[day]["temp"]["max"]))

    def getForecastForDayByCityAndCountry(self, city, country_code, day):
        json_object = self.__forecastByCityAndCountry(city, country_code)
        days = json_object["list"]
        days = sorted(days, key=lambda days: days["dt"])        #sort by date
        return days[day]["weather"][0]["id"], int(round(days[day]["temp"]["max"]))

    def getSunset(self, latitude, longitude):
        json_object = self.__weather(latitude, longitude)
        sys_object = json_object["sys"]
        unix_time = int(sys_object["sunset"])
        return datetime.fromtimestamp(unix_time, pytz.utc)
    
    def getSunsetByCityAndCountry(self, city, country_code):
        json_object = self.__weatherByCityAndCountry(city, country_code)
        sys_object = json_object["sys"]
        return datetime.fromtimestamp(int(sys_object["sunset"]), pytz.utc)

    #helper methods
    def __get_json(self, url):
        response = urllib2.urlopen(url)
        json_string = response.read()
        return json.loads(json_string)
    # {
    #   "city": {
    #     "id": 2174003,
    #     "name": "Brisbane",
    #     "coord": {
    #       "lon": 153.028091,
    #       "lat": -27.467939
    #     },
    #     "country": "AU",
    #     "population": 0
    #   },
    #   "cod": "200",
    #   "message": 0.0115,
    #   "cnt": 2,
    #   "list": [
    #     {
    #       "dt": 1475802000,
    #       "temp": {
    #         "day": 302.05,
    #         "min": 291.11,
    #         "max": 303.66,
    #         "night": 291.11,
    #         "eve": 301.04,
    #         "morn": 296.67
    #       },
    #       "pressure": 1022.44,
    #       "humidity": 63,
    #       "weather": [
    #         {
    #           "id": 800,
    #           "main": "Clear",
    #           "description": "clear sky",
    #           "icon": "01d"
    #         }
    #       ],
    #       "speed": 1.61,
    #       "deg": 0,
    #       "clouds": 0
    #     },
    #     {
    #       "dt": 1475888400,
    #       "temp": {
    #         "day": 298.05,
    #         "min": 288.61,
    #         "max": 302.57,
    #         "night": 294.27,
    #         "eve": 301.71,
    #         "morn": 288.61
    #       },
    #       "pressure": 1019.83,
    #       "humidity": 59,
    #       "weather": [
    #         {
    #           "id": 801,
    #           "main": "Clouds",
    #           "description": "few clouds",
    #           "icon": "02d"
    #         }
    #       ],
    #       "speed": 2.46,
    #       "deg": 297,
    #       "clouds": 12
    #     }
    #   ]
    # }


    def __forecastByCityAndCountry(self, city, country_code):
        url = "http://api.openweathermap.org/data/2.5/forecast/daily?q=%s,%s&APPID=%s&cnt=16" % (city, country_code, self.appid)
        return self.__get_json(url)

    def __forecast(self, latitude, longitude):
        url = "http://api.openweathermap.org/data/2.5/forecast/daily?lat=%s&lon=%s&APPID=%s&cnt=16" % (latitude,longitude, self.appid)
        return self.__get_json(url)    

    #e.g. weather response
    # {
    #   "coord": {
    #     "lon": 153.03,
    #     "lat": -27.47
    #   },
    #   "weather": [
    #     {
    #       "id": 800,
    #       "main": "Clear",
    #       "description": "clear sky",
    #       "icon": "01n"
    #     }
    #   ],
    #   "base": "stations",
    #   "main": {
    #     "temp": 296.694,
    #     "pressure": 1025.52,
    #     "humidity": 60,
    #     "temp_min": 296.694,
    #     "temp_max": 296.694,
    #     "sea_level": 1037.97,
    #     "grnd_level": 1025.52
    #   },
    #   "wind": {
    #     "speed": 1.86,
    #     "deg": 352.501
    #   },
    #   "clouds": {
    #     "all": 0
    #   },
    #   "dt": 1475796100,
    #   "sys": {
    #     "message": 0.0083,
    #     "country": "AU",
    #     "sunrise": 1475695231,
    #     "sunset": 1475740278
    #   },
    #   "id": 2174003,
    #   "name": "Brisbane",
    #   "cod": 200
    # }
    def __weatherByCityAndCountry(self, city, country_code):
        url = "http://api.openweathermap.org/data/2.5/weather?q=%s,%s&APPID=%s" % (city, country_code, self.appid)
        return self.__get_json(url)

    def __weather(self, latitude, longitude):
        url = "http://api.openweathermap.org/data/2.5/weather?lat=%s&lon=%s&APPID=%s" % (latitude,longitude, self.appid)
        return self.__get_json(url)