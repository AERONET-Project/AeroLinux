#!/bin/bash

now=$(date)
USER=$(logname)
cronjob="10 */6 * * * /home/$user_var/aerolinux/controls/updater.sh"
cronjob2="5 */10 * * * /home/$user_var/aerolinux/controls/pi_ftp_upload.sh"
cron_check=$(crontab -l | grep "6")

if [ -z "$cron_check" ]; then
	if [ $LOGNAME == "RPiz_19" ]; then
		echo "identified user requiring service"
		{ crontab -l -u $LOGNAME; echo "$cronjob"; } | crontab -u $LOGNAME -
		{ crontab -l -u $LOGNAME; echo "$cronjob2"; } | crontab -u $LOGNAME -
		cc -o /home/$USER/aerolinux/controls/pi_ftp_upload /home/$USER/aerolinux/controls/pi_ftp_upload.c -lm -lcurl
	elif [ $LOGNAME == "RPiz_17" ]; then
		echo "Identified user requiring service"
		{ crontab -l -u $LOGNAME; echo "$cronjob"; } | crontab -u $LOGNAME -
		{ crontab -l -u $LOGNAME; echo "$cronjob2"; } | crontab -u $LOGNAME -
		cc -o /home/$USER/aerolinux/controls/pi_ftp_upload /home/$USER/aerolinux/controls/pi_ftp_upload.c -lm -lcurl
	elif [ $LOGNAME == "RPiz_16" ]; then
		echo "Identified user requiring service"
		{ crontab -l -u $LOGNAME; echo "$cronjob"; } | crontab -u $LOGNAME -
		{ crontab -l -u $LOGNAME; echo "$cronjob2"; } | crontab -u $LOGNAME -
		cc -o /home/$USER/aerolinux/controls/pi_ftp_upload /home/$USER/aerolinux/controls/pi_ftp_upload.c -lm -lcurl
	elif [ $LOGNAME == "RPiz_18" ]; then
		echo "Identified user requiring service"
		{ crontab -l -u $LOGNAME; echo "$cronjob"; } | crontab -u $LOGNAME -
		{ crontab -l -u $LOGNAME; echo "$cronjob2"; } | crontab -u $LOGNAME -
		cc -o /home/$USER/aerolinux/controls/pi_ftp_upload /home/$USER/aerolinux/controls/pi_ftp_upload.c -lm -lcurl
	else
		echo "Current user is not required to have this update"
	fi
else
	echo 'crontab has the job in it, exiting...'

fi

counter=0
getscript() {
  pgrep -lf ".[ /]$1( |\$)"
}
while [ $counter -lt 4 ]; do
	if getscript "models_connect_and_reset" >/dev/null; then
		echo "Cimel connect is running at ${now}"

		sleep 30m
	else
		echo "Cimel connect was not running at ${now}." >> $HOME/logs/connection.log
		counter=$((counter+1))
		/home/$USER/aerolinux/controls/startup.sh
		
		sleep 30m
		now=$(date)
	fi
done
echo "Reached counter at ${now} and performed reboot" >> $HOME/logs/connection.log
sudo /sbin/shutdown -r now
