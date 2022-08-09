cd ~
sudo update-alternatives --install /usr/bin/python python /usr/bin/python3 10
pip3 install Adafruit-Blinka
sudo pip3 install adafruit-circuitpython-ads1x15
sudo pip3 install --upgrade adafruit-python-shell
cronjob="@reboot sleep 120 && /usr/bin/python3 /home/$USER/AeroLinux/tools/adc_battery.py >> ~/cron.log 2>&1"
{ crontab -l -u $USER; echo "$cronjob"; } | crontab -u $USER -
echo "========="
echo "Enable I2C manually now"
echo "yes"
