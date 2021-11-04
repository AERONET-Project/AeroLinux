#!/bin/bash

modem_check=$(dmesg | grep modem)
if [ -z "$modem_check" ]; then
	site='local'
else
	site='remote'
fi

devp=$(
for sysdevpath in $(find /sys/bus/usb/devices/usb*/ -name dev); do
    (
        syspath="${sysdevpath%/dev}"
        devname="$(udevadm info -q name -p $syspath)"
        [[ "$devname" == "bus/"* ]] && exit
        eval "$(udevadm info -q property --export -p $syspath)"
        [[ -z "$ID_SERIAL" ]] && exit
        echo "/dev/$devname - $ID_SERIAL" | grep FTDI | head -n1 | cut -d " " -f1 | tail -c 5
    )
done)

if [ -z "$devp" ]
then
	devp=USB0
else
	true
fi
USER=$(whoami)
/home/$USER/aerolinux/$site/modelT_connect $devp
