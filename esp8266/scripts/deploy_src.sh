#!/usr/bin/env bash

mpfshell -c "open ws:192.168.0.127,1106; put ../src/boot.py boot.py; put ../src/main.py main.py; ls" -n
