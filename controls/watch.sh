#!/bin/bash

now=$(date)
USER=$(logname)

counter=0
getscript() {
  pgrep -lf ".[ /]$1( |\$)"
}
while [ $counter -lt 4 ]; do
	if getscript "models_connect_and_reset" >/dev/null; then
		echo "Cimel connect is running at ${now}"

		sleep 13h
	else
		echo "Cimel connect was not running at ${now}." >> $HOME/logs/connection.log
		counter=$((counter+1))
		/home/$USER/AeroLinux/controls/startup.sh
		
		sleep 13h
		now=$(date)
	fi
done
echo "Reached counter at ${now} and performed reboot" >> $HOME/logs/connection.log
sudo /sbin/shutdown -r now
