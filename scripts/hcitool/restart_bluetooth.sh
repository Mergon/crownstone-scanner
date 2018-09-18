sudo rmmod btusb
sudo modprobe btusb
sudo rfkill unblock bluetooth
sudo hciconfig hci0 up
