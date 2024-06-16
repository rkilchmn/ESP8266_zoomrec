Send config for first time based on default in firmware:
curl -v  -X POST  -H "Content-Type: application/json" -u admin:myadminpw --data @ESP8266_zoomrec_config_sample.json http://<IP of ESP8266>:8080/config

curl -v -F "firmware=@build/ESP8266_zoomrec.ino.bin" -u "admin:myadminpw" http://192.168.0.27:8081/update

custom build properties in arduino.json:
   "buildPreferences": [
        [
            "build.extra_flags",
            "-DDEBUG_ESP_HTTP_UPDATE -DDEBUG_ESP_PORT=Serial"
        ]
    ],


Note: if debug information is showing, despite having removed all debug directives, delete the cached core compilates:

Arduino IDE
When you compile a sketch for ESP8266 using the Arduino IDE:

Temporary Build Directory:

The compiled .elf, .bin, .map, and other files are stored in a temporary directory specific to your operating system. This directory is typically cleaned up after the compilation process completes.

Windows: The temporary build directory is usually under C:\Users\<username>\AppData\Local\Temp\arduino_build_xxxxxx.

macOS: The temporary build directory is located under /var/folders/xx/xxxxxxxxxxxxxxxxxxxxxxxxxxxxx/arduino_build_xxxxxx.

Linux: The temporary build directory can be found under /tmp/arduino_build_xxxxxx.

The xxxxxx in the directory name represents a unique identifier generated by Arduino IDE during the build process.


Arduino OTA not working:
https://forum.arduino.cc/t/ota_mode-no-network-port-is-shown/552931/32

python espota.py -i <ESP_IP_Address> -p 8266 -f /path/to/your-sketch.ino.generic.bin

python espota.py -i ESP8266_zoomrec.local -p 8266 -a your_password_here -f /path/to/your-sketch.ino.generic.bin

python3 /home/roger/.arduino15/packages/esp8266/hardware/esp8266/3.1.2/tools/espota.py -i 192.168.0.27 -p 8081 -a myadminpw -f build/ESP8266_zoomrec.ino.bin

linux tool to list mDNS: avahi-browse -a
sudo apt-get install avahi-utils

