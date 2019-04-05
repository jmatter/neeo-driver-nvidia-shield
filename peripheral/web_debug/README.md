# Debugger
This page shows the log of the websocket webserver, running on the HID peripheral. This page has an auto-reconnecting websocket implementation and 2 possiblities to send data to the peripheral.

## Option 1 - send a modifier and keycode.
In case you already know the modifier and keycode, you can send them using the box on the left. The format is "<modifier>,<keycode>", where modifier is one of these codes:
  - 0: no modifier
  - 1: Left Ctrl
  - 2: Left Shift
  - 4: Left Alt
  - 8: Left GUI key (Windows key / Command key)
  - 16: Right Ctrl
  - 32: Right Shift
  - 64: Right Alt
  - 128: Right GUI key (Windows key / Command key)

The keycode is an numeric representation of a keycode between 1 and 255; for inspiration, take a look at this online page [USB HID Keyboard Scan Codes](https://serverhelfer.de/usb-hid-keyboard-scan-codes/) (remember to convert the HEX value to the numerical representation).

As an alternative, keycodes can be found in the sourcecode of the peripheral, e.g. an 'a' is "0,4" (no modifier, key is 4) and an 'A' is "2,4" (shift modifier, key is 4).

Specific commands for the Android Shield TV are (with modifier "0" unless otherwise mentioned):
  - Left          (80) / (92)
  - Right         (79) / (94)
  - Up            (82) / (96)
  - Down          (81) / (90)
  - Select        (40)
  - Back          (41)
  - Home          (1,41)
  - Volume up     (237)
  - Volume down   (238)
  - Mute          (239)
  - Menu          (101)
  - Power on/off  (102)
  - Play/pause    (232)
  - Stop          (243)
  - Fast forward  (242)
  - Fast reverse  (241)
  - Next song     (235)
  - Previous song (234)
  - Recent apps (modifier: 4 (alt) + key: 43 (tab))*

## Option 2 - send a text string
With the box on the right of the page you can send a string. This is not implemented in the Neeo driver itself but it is supported by the BLE HID peripheral. Each character will be sent 'as-is'; the peripheral finds out which modifier and keycode are needed to be put in the buffer before it is sent to the paired BLE device.

> Note: This website uses an external jQuery library, served from a CDN. If you want to run this sample on a device which doesn't have access to the internet, you need to modify the index.html yourself.