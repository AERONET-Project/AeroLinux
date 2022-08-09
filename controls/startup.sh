#!/bin/bash

modem_check=$(lsusb | grep Qualcomm)
if [ -z "$modem_check" ]; then
	$HOME/AeroLinux/cimel_connect/models_connect_and_reset </dev/null
else
	lsusb | $HOME/AeroLinux/cimel_connect/models_connect_and_reset
fi
