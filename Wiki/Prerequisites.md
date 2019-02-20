Please make sure you have the following prerequisites in order before attempting to install/run this driver.
  * Bluetooth hardware dongle
  * For the Noble Bluetooth package, [some additional prerequisites](https://github.com/noble/bleno#prerequisites) are required.



## Notes
  * macOS / Mac OS X, Linux, FreeBSD and Windows are currently the **only** supported OSes. _This limitation is emposed by the Bluetooth library which is in use._
  * If you run into problems running as non-root on Linux, follow [the steps outlined here](https://github.com/noble/bleno#running-without-rootsudo)
  * If you have multiple bluetooth adapters, check the documentation of the [Bleno package](https://github.com/noble/bleno#multiple-adapters) which is used in this driver.
  * If you are running node 10 or higher and run into compilation errors, [check this issue](https://github.com/noble/node-bluetooth-hci-socket/issues/84) for a fix.
