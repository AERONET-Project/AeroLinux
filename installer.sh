#!/bin/bash

#  -----------------------------------------------------------------
# |This script installs the necessary packages on a Linux system    |
# | It includes a Python installation to operate a modem	    |
# | The script is meant to run once, running again will add new user|
# | Author: anthony.d.larosa@nasa.gov				    |
#  -----------------------------------------------------------------

if [ `id -u` -ne 0 ]; then
	echo "Please execute this installer script with sudo, exiting.."
	exit 1
fi

echo "Setting system's timezone to UTC."
timedatectl set-timezone Etc/UTC
sleep 1


apt-get install -y libcurl4-openssl-dev
if [[ $> 0 ]]
then
	echo "libcurl failed to install, exiting."
else
	echo "libcurl is installed, continuing with script."
fi

echo "Downloading Hologram SDK..."
curl -L hologram.io/python-install | bash

if [[ $> 0 ]]
then
	echo "SDK failed installation, re-running"
	curl -L hologram.io/python-install | bash
else
	echo "Sucessfully installed Hologram SDK"
fi
curl -L hologram.io/python-install | bash
#Create new user via I/O
read -p "Enter a new username:" user_var # Save username as var user_var
echo "The current username is:" "$user_var"

adduser --force-badname "$user_var" --gecos "" --disabled-password
passwd "$user_var"
while [ $? -ne 0 ]; do
	sleep 3 #give time to ctrl-c
	passwd $user_var
done

for GROUP in $(groups pi | sed 's/.*:\spi//'); do adduser "$user_var" $GROUP; done

sleep 2
getent group sudo | grep -q "$user_var"
if [ $? -eq 0 ]; then
	echo "$user_var has root privileges, continuing..."
else
	echo "Adding using to root failed...Try a new username?" 1>&2
	userdel -r "$user_var"
	exit 1
fi

autologinf=/etc/systemd/system/getty@tty1.service.d/autologin.conf

if [ -e "$autologinf" ]; then
	echo "An auto-login config file already exists"
else
	echo "Building an auto-login config file for "$user_var""
	echo "[Service]"\ >> $autologinf
	echo "ExecStart="\ >> $autologinf
	echo "ExecStart=-/sbin/agetty --autologin "$user_var" --noclear %I $TERM" >> $autologinf
fi

echo "$user_var	ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers
echo "Adding cronjobs to user's crontab"
cronjob1="@reboot sleep 180 && /home/$user_var/aerolinux/controls/startup.sh >> $HOME/logs/connection.log"
cronjob2="@reboot sleep 300 && /home/$user_var/aerolinux/controls/watch.sh"
cronjob3="0 0 */2 * * /home/$user_var/aerolinux/controls/updater.sh"
cronjob4="0 0 */2 * * /home/$user_var/aerolinux/controls/k7_k8_check.sh"

{ crontab -l -u $user_var 2>/dev/null; echo "$cronjob1"; } | crontab -u $user_var -
{ crontab -l -u $user_var; echo "$cronjob2"; } | crontab -u $user_var -
{ crontab -l -u $user_var; echo "$cronjob3"; } | crontab -u $user_var -
{ crontab -l -u $user_var; echo "$cronjob4"; } | crontab -u $user_var -

sleep 2
echo "Building new directories..."
sleep 2

mkdir /home/$user_var/logs #Make a log file directory
mkdir /home/$user_var/backup #For data files saved to disk
touch /home/$user_var/logs/connection.log
touch /home/$user_var/logs/modem_diagnostics.log
cp -r $PWD /home/$user_var #Copy the programs from current user to new user
chown -R ${user_var}: /home/$user_var/
chmod -R 777 /home/$user_var/


sleep 2
echo "Compiling Cimel software package..."
sleep 2

cd /home/$user_var/aerolinux/cimel_connect/
cc -o models_connect_and_reset models_connect_and_reset.c models_port.c -lm -lcurl
chmod -R 777 /home/$user_var/aerolinux/cimel_connect/
chown ${user_var}: /home/$user_var/aerolinux//cimel_connect/models_connect_and_reset



sleep 2
echo "==========================="
sleep 2
echo "==========================="
echo "Build complete"
echo "Please execute a reboot to hard reload daemons and kernel changes"
