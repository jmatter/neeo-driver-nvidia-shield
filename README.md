# Neeo-driver-nvidia-shield
This is driver for the Neeo remote to support the NVidia Shield via BlueTooth. It consists of a [peripheral](./peripheral) which runs on, for example, a Raspberry Pi and a [driver](./driver) which can also run on a Raspberry Pi. The peripheral uses a plugged in bluetooth USB adapter to act as a 'keyboard' for the NVidia Shield.

The driver receives commands from the Neeo remote, and passes them on via websockets to the 'keyboard' which in turn passes the commands on to the NVidia Shield over BLE.

> Inspiration taken from:
http://www.freebsddiary.org/APC/usb_hid_usages.php
https://gist.github.com/MightyPork/6da26e382a7ad91b5496ee55fdc73db2
