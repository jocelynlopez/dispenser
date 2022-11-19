#!/usr/bin/env bash

mpfshell -c "open ws:192.168.0.127,1106; put ../libs/micropython-mfrc522/mfrc522.py mfrc522.py; ls" -n
