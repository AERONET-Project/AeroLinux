#!/bin/bash


#  =============================================================
# | Method for resetting USB hub and devices with 3x redundancy |
# | Script is used for hardware/firmware bugs of Hologram Nova  |
# | 1)Unbind and rebind the drivers to the hub controller       |
# | 2)Reset usb using usb_modeswitch			        |
# | 3)Reset usb using usbreset				        |
# | Author: anthony.d.larosa@nasa.gov					|
#  =============================================================

modem_check=$(dmesg | grep modem)
if [ -z "$modem_check" ]; then
	exit 0 #this code is only needed for modem configurations
else
	true
fi


prodid=$(lsusb | awk '$7=="Terminus"{print $6}' | cut -b 1-4) #get product id string
vendid=$(lsusb | awk '$7=="Terminus"{print $6}' | cut -b 6-9) #get vendor id string

#1
sudo bash -c "echo '1-1' | sudo tee /sys/bus/usb/drivers/usb/unbind"
sleep 15
sudo bash -c "echo '1-1' | sudo tee /sys/bus/usb/drivers/usb/bind"
#bus should always be 1 since the pi has one root hub (host controller)
#the branched devices are given by which this controller controls, first,1, should be hub.
sleep 20

#2
sudo usb_modeswitch -v 0x$prodid -p 0x$vendid --reset-usb
#to use bus instead of vendor id: -b 1 -g 2, for 001 and 002
sleep 20

#3
lsusb | sudo awk '/Terminus.*Hub$/{ system("/usr/bin/usbreset " $6) }'
#Uses built in debian usbresest command, a custom version is installed in tools
#User will have to update exeucatable path to this tool
sleep 30
sudo reboot
