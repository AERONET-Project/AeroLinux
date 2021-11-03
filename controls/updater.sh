#!/bin/bash
#testtt
modem_check=$(dmesg | grep modem)
if [ -z "$modem_check" ]; then
  echo "non-modem device"
else
  sudo hologram network disconnect
  sudo hologram network connect
fi

cd /home/$USER/aerolinux
if git fetch origin master --progress 2>&1 | grep -q "Enumerating"; then
  echo "Update found from GitHub, recompiling and then rebooting..."
  git fetch origin master
  git reset --hard origin/master
  sleep 2
  echo "======================="
  echo "======================="
  sleep 3

  cd /home/$USER/aerolinux/remote
  cc -o model5_connect model5_connect.c model5_port.c -lm -lcurl
  sleep 4
  #Need T compilation
  echo "recompiled and now setting permissions"
  chmod -R 777 /home/$USER/aerolinux/remote

  cd /home/$USER/aerolinux/local
  cc -o model5_connect model5_connect_silent.c model5_port.c -lm -lcurl
  sleep 4
  cc -o modelT_connect modelT_connect_silent.c modelT_port.c -lm -lcurl
  #Need T compilation
  echo "recompiled and now setting permissions"
  chmod -R 777 /home/$USER/aerolinux/local

  if [ -z "$modem_check" ]; then
    echo "non-modem device"
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
