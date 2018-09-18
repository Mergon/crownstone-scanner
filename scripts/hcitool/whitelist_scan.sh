#!/bin/sh

address=${1:? "Usage: $0 filter"}

sudo hcitool lewlclr
sudo hcitool lewladd --random $address
sudo hcitool lescan --duplicates --whitelist
