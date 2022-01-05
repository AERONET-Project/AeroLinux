#!/bin/bash

modem_check=$(lsusb | grep Qualcomm)
if [ -z "$modem_check" ]; then
	$HOME/aerolinux/cimel_connect/models_connect_and_reset </dev/null
else
	lsusb | $HOME/aerolinux/cimel_connect/models_connect_and_reset
fi
