#!/bin/bash

now=$(date)
counter=0
getscript() {
  pgrep -lf ".[ /]$1( |\$)"
}
while [ $counter -lt 4 ]; do
	if getscript "modelT_connect" >/dev/null || getscript "model5_connect" >/dev/null; then
		echo "Cimel connect is running at ${now}"
		sleep 30m
	else
		echo "Cimel connect was not running at ${now}." >> $HOME/logs/connection.log
		counter=$((counter+1))
		/home/$USER/aerolinux/controls/startup.sh
		sleep 30m
	fi
done
echo "Reached counter at ${now} and performed reboot" >> $HOME/logs/connection.log
sudo /sbin/shutdown -r now
