# Scan for Crownstones

This utility can parse iBeacon messages and Crownstone specific scan responses.

## Dependencies

Very few dependencies are required. The Bluez library is used directly (not communicating over dbus).

* Bluez
* libblepp 

You can install bluez via your package manager, apt for example.
Meri says: Don't forget, you need libbluetooth-dev!

To install libblepp:

	sudo apt install libboost-dev bluez
	git clone https://github.com/edrosten/libblepp
	cd libblepp
	mkdir build
	cd build
	cmake ..
	make
	sudo make install

If you can't install, it's just a `libble++.so` symlink and a `libble++.so.5` file you will need. You might just be able to copy it to your system and be fine.

## Installation

The installation is straightforward:

	git clone https://github.com/mrquincle/crownstone-scanner.git
	cd crownstone-scanner
	mkdir build
	cd build
	cmake ..
	make

## Usage

There is one obligated option (assuming you are in the build directory):

    ./crownstone -k 5f3540aa869f57efbdc12cb5ad409a19

Here the argument `k` requires the guest key. The guest key of a sphere can be obtained through 
<cloud.crownstone.rocks>. 

## Errors

It might be that you will get an error like:

	error 1531465825.596375: Error obtaining HCI device ID
	terminate called after throwing an instance of 'BLEPP::HCIScanner::HCIError'
	  what():  Error obtaining HCI device ID

This means that there is no BLE device available. There are some instructions after the error message about how to reinsert a kernel module, unblock bluetooth via rfkill, and bring up the interface.

    sudo rmmod btusb
    sudo modprobe btusb
    sudo rfkill unblock bluetooth
    sudo hciconfig hci0 up

Another error might be the following:

	error 1531466395.775174: Setting scan parameters: Operation not permitted
	terminate called after throwing an instance of 'BLEPP::HCIScanner::IOError'
	  what():  Setting scan parameters: Operation not permitted

This means that the capabilities of the binary are not such that it is allowed to scan for BLE devices. It will give some instructions again, namely to either use sudo or manually add the capabilities (you will have to do this each time you compile it).

    sudo setcap cap_net_raw+ep crownstone

## Copyright

* Author: Anne van Rossum
* Copyright: Crownstone (https://crownstone.rocks)
* Date: September 2018, 2018
* License: LGPLv3, MIT, Apache

