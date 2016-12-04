"""`main` is the top level module for your Flask application."""

import logging

# Import the Flask Framework
from flask import Flask, Response, request, g, jsonify
from functools import wraps
import urllib2
import json
from datetime import datetime, timedelta
from pytz import timezone, utc
from openweathermap import Openweathermap
from freegeoip import Freegeoip
import yaml
from flask_jwt import JWT, jwt_required, current_identity
from werkzeug.security import safe_str_cmp

with open('config.yml', 'r') as f:
    doc = yaml.load(f)
    openweathermap_appid  = doc['openweathermap']['appid']
    weatherlight_duration = doc['weatherlight']['duration']
    weatherlight_max_wait = doc['weatherlight']['max_wait_poll']
    default_username = doc['security']['username']
    default_password = doc['security']['password']

class User(object):
    def __init__(self, id, username, password):
        self.id = id
        self.username = username
        self.password = password

    def __str__(self):
        return "User(id='%s', username='%s')" % (self.id, self.username)

users = [
    User(1, default_username, default_password),
]

username_table = {u.username: u for u in users}
userid_table = {u.id: u for u in users}

def authenticate(username, password):
    user = username_table.get(username, None)
    if user and safe_str_cmp(user.password.encode('utf-8'), password.encode('utf-8')):
        return user

def identity(payload):
    user_id = payload['identity']
    return userid_table.get(user_id, None)

app = Flask(__name__)
# Note: We don't need to call run() since our application is embedded within
# the App Engine WSGI application server.
app.debug = True
app.config['SECRET_KEY'] = 'OVDOS9ZL2ZURYZU580NKN7PIJ29K0FM0BUKIATS9AXD1KWO6CM43AFC5LR5OAS21'
app.config['JWT_AUTH_URL_RULE'] = '/v1/token'
app.config['JWT_EXPIRATION_DELTA'] = timedelta(minutes=30)
app.config['JWT_AUTH_USERNAME_KEY'] = 'username'
app.config['JWT_AUTH_PASSWORD_KEY'] = 'password'

jwt = JWT(app, authenticate, identity)

#because microcontrollers are bad ad json deserializing with pretty print output
@jwt.auth_response_handler
def custom_auth_response_handler(access_token, identity):
    logging.info(identity)
    result = {'jwt': {'token': access_token.decode('utf-8')}}
    logging.info(result)
    return json.dumps(result), 200, {'Content-Type': 'application/json'}

def openweathermap():
    if not hasattr(g, 'openweathermap'):
        g.openweathermap = Openweathermap(openweathermap_appid)
    return g.openweathermap

def calculateLedBarStatus(ref_time, start_time, duration):
    stop_time  = start_time + timedelta(hours=duration)
    enabled    = (start_time < ref_time and ref_time < stop_time)
    next_event = stop_time if enabled else start_time
    wait_poll = (next_event - ref_time).seconds
    if wait_poll > timedelta(hours=weatherlight_max_wait).seconds:
        wait_poll = timedelta(hours=weatherlight_max_wait).seconds
    else:
        # the end device might not have a very accurate clock/timer so we are 
        # making it fetch updated information somewhat before the actual next
        # event, but only if it's still far enough away (~5min)
        wait_poll = wait_poll-300 if wait_poll>300 else wait_poll
    return enabled, wait_poll

@app.before_request
def getGeoDetails():
    ip = request.remote_addr
    details = Freegeoip.getDetails(ip)
    logging.info(details)
    #g.timezone = details["time_zone"]
    #g.city = details["city"]
    #g.country_code = details["country_code"]
    request.latitude = details["latitude"]
    request.longitude = details["longitude"]
    request.now = datetime.now(utc)

#@app.route('/v1/statusandweather', methods=['GET'])
#def statusandweather():
#    #1. parse details from incoming request
#    lat, lon = request.latitude, request.longitude
#    
#    #2. get weather forecast for tomorrow 
#    code, temp = openweathermap().getForecastForDay(lat, lon, 1)
#    
#    #3. set start hour (e.g. based on sunset)
#    start = openweathermap().getSunset(lat, lon)
#
#    #4. calculate current led bar status (0=off, 1=on)
#    status, wait_poll = calculateLedBarStatus(request.now, start, weatherlight_duration)
#
#    response = "%i;%s;%s;%i" % (status, code, temp, wait_poll)
#    return response, 200, {'Content-Type': 'text/plain; charset=ascii'}

@app.route('/v1/data', methods=['GET'])
@jwt_required()
def data():
    logging.info('user: %s' % current_identity)

    #1. parse details from incoming request
    lat, lon = request.latitude, request.longitude
    
    #2. get weather forecast for tomorrow 
    code, temp = openweathermap().getForecastForDay(lat, lon, 1)
    
    #3. set start hour (e.g. based on sunset)
    start = openweathermap().getSunset(lat, lon)

    #4. calculate current led bar status (0=off, 1=on)
    status, wait_poll = calculateLedBarStatus(request.now, start, weatherlight_duration)

    result = {'data': {'status': status, 'code': code, 'temp': temp, 'wait_poll': wait_poll}}
    logging.info(result)

    #return jsonify(**result)
    return json.dumps(result), 200, {'Content-Type': 'application/json'}

@app.errorhandler(404)
def page_not_found(e):
    """Return a custom 404 error."""
    return 'Not found', 404


@app.errorhandler(500)
def application_error(e):
    """Return a custom 500 error."""
    return '{}'.format(e), 500
