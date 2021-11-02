#!/bin/bash

now=$(date)

is_ppp=0
text=`sudo ifconfig | grep ppp0`
if [ -n "$text"]; then
	text=`sudo ifconfig ppp0 | grep "inet addr"`
	if [ -n "text" ]; then
		is_ppp=1
	fi
fi

echo "Connection is ${is_ppp} at ${now}" 
echo "0 status = connected and 1 status = disconnected to modem"
