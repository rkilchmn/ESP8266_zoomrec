Send config for first time based on default in firmware:
curl -v  -X POST  -H "Content-Type: application/json" -u admin:myadminpw --data @ESP8266_zoomrec_config_sample.json http://<IP of ESP8266>:8080/config