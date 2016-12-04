import urllib2
import json

class Freegeoip(object):

    #example response
    #{"ip":"58.96.34.7",
    # "country_code":"AU",
    # "country_name":"Australia",
    # "region_code":"QLD",
    # "region_name":"Queensland",
    # "city":"Brisbane",
    # "zip_code":"4101",
    # "time_zone":"Australia/Brisbane",
    # "latitude":-27.4891,
    # "longitude":153.0188,
    # "metro_code":0}

    @staticmethod
    def getCityAndCountrycode(ip):
        json_object = Freegeoip.__get_details(ip)
        city = json_object["city"]
        country_code = json_object["country_code"]
        return str(city), str(country_code)
    
    @staticmethod
    def getTimezone(ip):
        json_object = Freegeoip.__get_details(ip)
        time_zone = json_object["time_zone"]
        return str(time_zone)

    @staticmethod
    def getDetails(ip):
        details = Freegeoip.__get_details(ip)
        return details

    @staticmethod
    def __get_details(ip):
        url = "http://freegeoip.net/json/%s" % (str(ip))
        return Freegeoip.__get_json(url)
    
    @staticmethod
    def __get_json(url):
        response = urllib2.urlopen(url)
        json_string = response.read()
        return json.loads(json_string)