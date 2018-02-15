# retro-wifi

This is an attempt to create a WiFi modem that will work with any home computer equipped with an RS-232C serial port or adapter.

Example target platforms:

* Commodore 64 with VIC-1011A terminal adapter

* Tandy Color Computer with Deluxe RS-232 Program Pak cartridge

* TRS-80 Model 100 laptop with built-in serial port and Telecom software

* Vintage PC running MS-DOS and Windows 3.1

Requirements:

* ESP8266 / NodeMCU development board

* SD card breakout board

* MAX3232 or similar RS-232 to TTL breakout board with DE-9 or DB-25 connector

* Null modem cable (or normal serial cable and null modem adapter)

* Arduino IDE with ESP8266 add-on and core libraries (see https://arduino-esp8266.readthedocs.io/en/latest/index.html for setup)

Depending on your exact setup, you may also need a DB-25 to DE-9 adapter.

Commodore 64 users will need a terminal program such as CCGS or Novaterm. The other example systems already include terminal software.

Proposed capabilities:

* Telnet and IRC connectivity over WiFi

* HTTP and FTP file download to SD card over WiFi

* File transfer from SD card to host computer using XMODEM protocol

