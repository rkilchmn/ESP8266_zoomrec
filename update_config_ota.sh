#!/bin/sh

# Check if both parameters are passed
if [ -z "$1" ] || [ -z "$2" ]; then
    echo "Usage: $0 <config_file> <ESP8266_IP>"
    exit 1
fi

# Assigning parameters to variables for clarity
CONFIG_FILE=$1
ESP8266_IP=$2

# Execute the curl command
curl -v -X POST -H "Content-Type: application/json" -u user:myuserpw --data @$CONFIG_FILE http://$ESP8266_IP:8080/config

