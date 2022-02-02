<img align="right" width="400" height="200" src="https://aeronet.gsfc.nasa.gov/images/aeronet_banner2.gif"><br/>

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
  cd aerolinux && chmod 755 installer.sh
```
```bash
  sudo ./installer.sh
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

**Raspberry Pi Devices:** Zero W (1.7 and 2), Raspberry Pi 3(A, A+,B+) and Raspberry Pi 4(B).

**OS Image:** 2020-05-27-raspios-buster-lite-armhf.img - Can be downloaded [here](http://ftp.jaist.ac.jp/pub/raspberrypi/raspios_lite_armhf/images/raspios_lite_armhf-2020-05-28/)
There is no need to upgrade OS image to current as the updates are GUI related. 

## Part List

| Part  | Purchase URL |
| ------------- | ------------- |
| Raspberry Pi Zero W (1.7)  | [Link](https://www.raspberrypi.com/products/raspberry-pi-zero-w/)  |
| USB + Ethernet Hat & Case  | [Link](https://www.newegg.com/p/0XM-04WN-00006?Item=9SIB6FWGR62475)  |
| AC Power Supply  | [Link](https://www.adafruit.com/product/1995?gclid=Cj0KCQiA0eOPBhCGARIsAFIwTs7Y9D-CGvd56o7pYmg8GlOTs6Ii-GeUW5u5k6WKNL8dwm3qR1UXf3MaAkWaEALw_wcB)  |
| Micro SD Card  | [Link](https://www.newegg.com/sandisk-16gb-microsdhc/p/N82E16820173358)  |

***Using a cellular modem*** 
This an option for an internet connection to transfer data, the software supports the Hologram Nova [2G/3G](https://www.digikey.com/en/products/detail/hologram-inc./HOL-NOVA-U201/7915568) or [/4G/LTE](https://www.digikey.com/en/products/detail/hologram-inc/HOL-NOVA-R410/9489897) (USA)

***Using solar power***
Sometimes your Cimel may be located in a remote region powered by battery. Most times the battery will provide 12 volts. The Pi can only take 5 volts in put, a DC/DC converter is required and we recommend the following due to its ease of use, [12-5V DC](https://www.newegg.com/p/2S7-01JK-0JVH6?Description=12v%20to%205v%20micro%20usb%20converter&cm_re=12v_to%205v%20micro%20usb%20converter-_-2S7-01JK-0JVH6-_-Product&quicklink=true)
