#!/bin/bash

# Check if base directory parameter is provided
if [ -z "$1" ]; then
    echo "Error: Please provide the base directory where libraries should be managed."
    exit 1
fi

# Assign base directory
base_dir="$1"

# Array of repositories to clone or update
repos=(
    "https://github.com/esp8266/ESPWebServer.git"
    "https://github.com/plageoj/urlencode.git"
    "https://github.com/bblanchon/ArduinoJson.git"
    "https://github.com/tzapu/WiFiManager.git"
    "https://github.com/highno/rtcvars.git"
    "https://github.com/rlogiacco/CircularBuffer.git"
    "https://github.com/JAndrassy/TelnetStream.git"
)

# Loop through each repository
for repo in "${repos[@]}"; do
    # Extract repository name from URL
    repo_name=$(basename "$repo" .git)

    # Construct full path to library directory
    library_dir="$base_dir/$repo_name"

    # Check if the directory exists
    if [ -d "$library_dir" ]; then
        echo "Updating $repo_name"
        # Directory exists, so pull the latest changes
        (cd "$library_dir" && git pull)
    else
        echo "Cloning $repo_name"
        # Directory doesn't exist, so clone the repository
        git clone "$repo" "$library_dir"
    fi
done
