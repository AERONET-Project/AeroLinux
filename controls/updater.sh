#!/bin/bash

USER=$(whoami)
modem_check=$(lsusb | grep Qualcomm)
if [ -z "$modem_check" ]; then
  echo "non-modem device"
else
  sudo hologram network disconnect
  sudo hologram network connect
fi

cd /home/$USER/aerolinux
if git fetch origin main --progress 2>&1 | grep -q "Enumerating"; then
  echo "Update found from GitHub, recompiling and then rebooting..."
  git fetch origin main
  git reset --hard origin/main
  sleep 2
  echo "======================="
  echo "======================="
  sleep 3

  cd /home/$USER/aerolinux/cimel_connect
  
  #cc -o models_connect_and_reset models_connect_and_reset.c models_port.c -lm -lcurl
  sleep 4
 
  echo "Recompiled and now setting permissions"
  chmod -R 777 /home/$USER/aerolinux/controls
  chmod -R 777 /home/$USER/aerolinux/tools
  chmod -R 777 /home/$USER/aerolinux/cimel_connect
  chown -R ${USER}:${USER} /home/$USER/aerolinux/

  if [ -z "$modem_check" ]; then
    echo "non-modem device"
    sudo reboot
  else
    sudo hologram network disconnect
    sudo reboot
  fi

 fi

if [ -z "$modem_check" ]; then
  echo "non-modem device"
else
  sudo hologram network disconnect
fi
