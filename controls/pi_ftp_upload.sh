#!/bin/bash

modem_check=$(lsusb | grep Qualcomm) #returns string if modem was identified
user_var=$(logname)

if [ -z "$modem_check" ]; then #Checks if modem_check is 0/empty - i.e., no modem 
	echo "A USB modem is not connected"	
	/home/$user_var/aerolinux/controls/pi_ftp_upload
	
else #Sees that modem_check has value and proceeds as modem is connected
	echo "A USB modem is connected"
	sudo hologram network connect
	/home/$user_var/aerolinux/controls/pi_ftp_upload
	sudo hologram network disconnect
fi
