This project provides a simple serial interface to SPI LoRa modules, using suitable Arduino or compatible boards.  LoRa modules are 3.3V logic so it's best to use an Arduino with 3.3V logic, such as the Arduino Mini Pro.

The Arduino serial can then connect to a PC/Mac/whatever using a suitable TTL serial connection (e.g. Prolific or FTDI USB adapter) or Tablet/Phone/whatever using Bluetooth (e.g. HC-06) Bluetooth serial adapter).

For a Windows PC, this software can be used with my serial gateway for uploading telemetry/SSDV to Habitat.  See:

https://github.com/daveake/LoRaSerialGateway

Hardware
========

Connections are:

Arduino 0 (Tx) - USB/Bluetooth adapter Rx
Arduino 1 (Rx) - USB/Bluetooth adapter Tx

Arduino  8 - LoRa DIO0
Arduino  9 - LoRa DIO5

Arduino 10 - LoRa NSS
Arduino 11 - LoRa MOSI
Arduino 12 - LoRa MISO
Arduino 13 - LoRa CLK

Serial Protocol
===============

The serial connection is 57600 baud (**changed from V1.0**) and will send status information and incoming LoRa packets without being polled.  Each of these is of the form

something=value<CR><LF>

The somethings are, currently:

	- CurrentRSSI=<RSSI>
	- Message=<telemetry>
	- Hex=<hex packet e.g. SSDV>
	- FreqErr=<error in kHz>
	- PacketRSSI=<RSSI>
	- PacketSNR=<SNR>

The serial interface accepts a few commands, each of the form

~<command><value><CR>

(a trailing <LF> can be sent but is ignored).  Accepted commands are responded to with an OK (* <CR> <LF>) and rejected commands (unknown command, or invalid command value) with a WTF (? <CR> <LF>)

The current commands are:

	- F<frequency in MHz>
	- M<mode>
	- B<bandwidth>
	- E<error coding from 5 to 8>
	- S<spreading factor from 6 to 11>
	- I<1=implicit mode, 0=explicit mode>
	- L(low data rate optimisation: 1=enable, 0=disable)
	
The supported modes are:

0 = (normal for telemetry)  Explicit mode, Error coding 4:8, Bandwidth 20.8kHz, SF 11, Low data rate optimize on
1 = (normal for SSDV)       Implicit mode, Error coding 4:5, Bandwidth 20.8kHz,  SF 6, Low data rate optimize off
2 = (normal for repeater)   Explicit mode, Error coding 4:8, Bandwidth 62.5kHz,  SF 8, Low data rate optimize off	

Bandwidth value strings can be 7K8, 10K4, 15K6, 20K8, 31K25, 41K7, 62K5, 125K, 250K, 500K

History
=======

30/09/2022	V2.02	- Bug fix in SNR calculation

08/03/2021	V2.01	- Larger receive buffer to allow for longer transmissions

			V2.0	- Added device, version and transmit commands

23/09/2016	V1.1	- Added Hex=... message for any packet that is not ASCII telemetry
					- Added LoRa modes 3-7K8
					- Increased baud rate to 57,600 (from 9,600) so handle high LoRa data rates esp. with SSDV
					
30/06/2016	V1.0	- First version
