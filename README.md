
# AeroLinux

This software package enables Linux devices to transfer data from Cimel sun photometers via Ethernet, Wi-Fi or a Hologram Nova cellular modem.

## Appendix

1) About
2) Installation
3) Demo
4) Part List


## About

The software provides fully autonomous data transfer, data uploading and data storage of the Cimel sun photometer version 5 and T data.


The recommended set up is a Raspberry Pi Zero W 1.7 running Rasbian OS. H
However, this software is lightweight and adaptable to other versions of Linux. A Debian version is strongly recommended.


Two data transfer configurations are possible, **Local** or **Remote**.

Simply run the installation script and your system will be configured to run.

The data will be stored on the eMMC flash storage device as a fail safe and will be uploaded or retrieved at a later time.
## Local
This configuration is when the Linux device is connected to LAN or Wi-Fi. The Raspberry Pi can be connected to both LAN and Wi-Fi.
## Remote

The remote configuration requires a Hologram Nova modem to transfer data. These configurations should be used when the site is remote and running on solar power.
## Deployment
Your linux device must be connected to internet to clone the directory and install the required packages.

To get your project up and running please clone the latest version of the software.

Run the installation script:
```bash
  sudo ./installer.sh
```
Thereafter you will be prompted to enter
a new user which will become the default user
of the system.

Please reboot the system to hard reload daemons and kernel changes.

```bash
  sudo reboot
```

Your system is now ready to go.