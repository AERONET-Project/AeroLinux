#!/bin/bash

echo -e "Last reboot was: $(uptime -s)\n" >> $HOME/logs/uptime.log
echo -e "\nThe system has been up since: $(uptime)" >> $HOME/logs/uptime.log
