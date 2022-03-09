#!/usr/bin/python3
import board
import busio
import time
import adafruit_ads1x15.ads1115 as ADS
from adafruit_ads1x15.analog_in import AnalogIn
from datetime import datetime
import datetime
import os

i2c = busio.I2C(board.SCL, board.SDA)
ads = ADS.ADS1115(i2c)
path = os.path.expandvars('/home/$USER/aerolinux/tools/battery.txt')
while True:
	f=open(path,'a')
	now = datetime.datetime.now().isoformat()
	chan = AnalogIn(ads, ADS.P0)
	bat_volts = chan.voltage * 5.532
	data = now + ',' + "{0:.3f}".format(bat_volts) + '\n'
	f.write(data)
	time.sleep(600)
	f.close()
