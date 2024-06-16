Send config for first time based on default in firmware:
curl -v  -X POST  -H "Content-Type: application/json" -u admin:myadminpw --data @ESP8266_zoomrec_config_sample.json http://<IP of ESP8266>:8080/config

 curl -v -F "image=@ESP8266_zoomrec.ino.bin" http://192.168.0.27:8081/update

custom build properties in arduino.json:
   "buildPreferences": [
        [
            "build.extra_flags",
            "-DDEBUG_ESP_HTTP_UPDATE -DDEBUG_ESP_PORT=Serial"
        ]
    ],