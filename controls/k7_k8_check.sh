#!/bin/bash

file1=$HOME/last_time.k7
file2=$HOME/last_time.k8

if [ -f $file1 ] || [ -f $file2 ]
then
	echo "A file exist"
else
	echo "No last_time file detected at ${date}" >> $HOME/logs/connection.log
	exit 0
fi

if [ -f $file1 ] && [ -f $file2 ]
then
	echo 'Both files exist'
	if [ ${file1} -ot ${file2} ]
	then
		echo 'k8 file is newer using that to compare'
		FILE=$HOME/last_time.k8
		last_time=$(stat -c %Y $HOME/last_time.k8)
		current_time=$(date +%s)
		last_timehuman=$(stat -c '%y' $HOME/last_time.k8)
		if [ "$((current_time-last_time))" -gt 173000 ]; then
		echo "Over two days since last_time has been updated, checked at: ${date}" >> $HOME/logs/connection.log
		echo "The most recent last_time modification was: ${last_timehuman}" >> $HOME/logs/connection.log
		sudo reboot
		fi
	else
		echo 'k7 file is newer using that to compare'
		FILE=$HOME/last_time.k7
		last_time=$(stat -c %Y $HOME/last_time.k7)
		current_time=$(date +%s)
		last_timehuman=$(stat -c '%y' $HOME/last_time.k7)
		if [ "$((current_time-last_time))" -gt 173000 ]; then
		echo "Over two days since last_time has been updated, checked at: ${date}" >> $HOME/logs/connection.log
		echo "The most recent last_time modification was: ${last_timehuman}" >> $HOME/logs/connection.log
		sudo reboot
		fi

	fi
echo $FILE "was used to compare, exiting"
exit 0
fi

if [ -f $file1 ]
then
	echo "Only one file exists, a .k7"
	FILE=$HOME/last_time.k7
	last_time=$(stat -c %Y $HOME/last_time.k7)
	current_time=$(date +%s)
	last_timehuman=$(stat -c '%y' $HOME/last_time.k7)
	if [ "$((current_time-last_time))" -gt 173000 ]; then
	echo "Over two days since last_time has been updated, checked at: ${date}" >> $HOME/logs/connection.log
	echo "The most recent last_time modification was: ${last_timehuman}" >> $HOME/logs/connection.log
	sudo reboot
	fi
else
	echo "Only one file exists, a .k8"
	FILE=$HOME/last_time.k7
	last_time=$(stat -c %Y $HOME/last_time.k8)
	current_time=$(date +%s)
	last_timehuman=$(stat -c '%y' $HOME/last_time.k8)
	if [ "$((current_time-last_time))" -gt 173000 ]; then
	echo "Over two days since last_time has been updated, checked at: ${date}" >> $HOME/logs/connection.log
	echo "The most recent last_time modification was: ${last_timehuman}" >> $HOME/logs/connection.log
	sudo reboot
fi