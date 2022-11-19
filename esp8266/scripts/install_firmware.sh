#!/usr/bin/env bash

pip install esptool setuptools

# 1.1 - Nettoyage du firmware (en root):
python -m esptool --port /dev/ttyUSB0 erase_flash

# 1.3 - Upload d'un nouveau firmware (en root):
python -m esptool --port /dev/ttyUSB0 --baud 460800 write_flash --flash_size=detect 0 ../firmwares/esp8266-ota-20220618-v1.19.1.bin
