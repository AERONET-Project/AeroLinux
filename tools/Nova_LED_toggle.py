#!/usr/bin/env python

import serial

port = "dev/ttyUSB1"
ser = serial.Serial(port)
print("Opening serial port to toggle GPIO pins on")
ser.write("AT.UGPIOC=23,10\r\n".encode()) #Blue power LED
ser.write("AT.UGPIOC=16,2\r\n".encode()) #Red modem LED
ser.close()
