# AeroLinux

This software package enables Linux devices to transfer data from Cimel sun photometers via Ethernet, Wi-Fi or a Hologram Nova cellular modem.
#### Version 1.0.2 ####
 * Integrated to one data communication and transfer program
 * Reduced watch dog programs and introduced method to monitor data loss

## Appendix

1) About
2) Local
3) Remote
4) Deployment
5) Technical Details
6) Part List


## About

The software provides fully autonomous data transfer, data uploading and data storage of the Cimel sun photometer Model 5 and TS data, and their accompanying versions (SeaPrism, Polar, etc).


The recommended set up is a Raspberry Pi Zero W 1.7 running Rasbian OS. H
However, this software is lightweight and adaptable to other versions of Linux. A Debian version is strongly recommended.


Simply run the installation script and your system will be configured to run.

The data will be stored on the eMMC flash storage device as a fail safe and will be uploaded or retrieved at a later time.
## Local
This configuration is when the Linux device is connected to LAN or Wi-Fi. The Raspberry Pi can be connected to both LAN and Wi-Fi. We recommend being connected to AC power when operating with this version, as power consumption data has not been analyzed.
## Remote

The remote configuration requires a Hologram Nova modem to transfer data. These configurations should be used when the site is remote and running on solar power.
## Deployment
Your linux device must be connected to internet to clone the directory and install the required packages.
When setting up your Linux system please run the following commands.

Update the apt tool,
```bash
  sudo apt --fix-missing update -y
```
Then install git,
```bash
  sudo apt install git -y
```

To get your project up and running please clone the latest version of the software,
```bash
  sudo git clone https://github.com/anthony-larosa/aerolinux.git
```


Run the installation script:
```bash
  cd aerolinux
```
```bash
  sudo ./installer.sh && chmod 755 installer.sh
```
Thereafter, you will be prompted to enter
a new user which will become the default user
of the system.

Please reboot the system to hard reload daemons and kernel changes.

```bash
  sudo reboot
```

Your system is now ready to go!

## Technical Details

The program will run fully autonomously, with integrated error checking and monitoring. Therefore, the system is designed as a plug and go with no configuartion required after the installation and reboot.

The system can handle unplugging and reconnecting cables, however, it is wise to un-plug and reboot the Pi when everything has been plugged in.

Any updates to this GitHub respository will be propagted to your device automatically. 

## Tech Stack

**Raspberry Pi Devices:** Zero W (1.7), Raspberry Pi 3(A, A+,B+) and Raspberry Pi 4(B).

**OS Image:** 2020-05-27-raspios-buster-lite-armhf.img

## Part List

* Rasberry Pi
* Ethernet Hub and USB Hub w/ Micro USB OTG Connector
* 5V Power Supply
