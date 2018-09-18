#!/bin/sh

filter=${1:? "Usage: $0 filter"}

sudo hcitool lescan --duplicates | tee | grep $1
