# ------------------------------------------------------------------------------
# 1.                    Mise en place d'un firmware:
# ------------------------------------------------------------------------------
>>> pip install esptool setuptools

# 1.1 - Nettoyage du firmware (en root):
>>> python -m esptool --port /dev/ttyUSB0 erase_flash


# 1.2 - Téléchargement d'un firmware:
https://micropython.org/download/esp8266/


# 1.3 - Upload d'un nouveau firmware (en root):
>>> python -m esptool --port /dev/ttyUSB0 --baud 460800 write_flash --flash_size=detect 0 esp8266-ota-20220618-v1.19.1.bin

# ------------------------------------------------------------------------------
# 2.              Mise en place des communication avec l'ESP8266:
# ------------------------------------------------------------------------------
# Lancement d'un terminal (REPL) depuis la connexion USB:
# -------------------------------------------------------
>>> picocom /dev/ttyUSB0 -b115200

# Activation du web terminal (par wifi):
# --------------------------------------
>>> import webrepl_setup

# Activation de la connexion wifi entre l'ESP et le routeur:
# ----------------------------------------------------------
import network
sta_if = network.WLAN(network.STA_IF)
ap_if = network.WLAN(network.AP_IF)
sta_if.active(True)
sta_if.connect('jorouter2_2.4G', 'Terafrageur60ajorouter2')

if sta_if.isconnected():
    print("Connexion wifi active avec l'ip : %s" % sta_if.ifconfig()[0])
    print("On desactive la connexion wifi local")
    ap_if.active(False)
else:
    print("La connexion wifi n'est pas active !")

# ------------------------------------------------------------------------------
# 3.                        Echanges de fichiers:
# ------------------------------------------------------------------------------
# https://github.com/wendlers/mpfshell
>>> pip install mpfshell

# Récupération des librairies:
# -----------------------------
RC522 RFID : https://github.com/danjperron/micropython-mfrc522

# Envoi de fichiers:
# ------------------
mpfshell -c "open ws:192.168.0.127,1106; put ../src/boot.py boot.py; put ../src/main.py main.py; ls" -n


# ------------------------------------------------------------------------------
# 3.                           SOURCES:
# ------------------------------------------------------------------------------
# Désactivation du serial REPL qui pose problème:
#------------------------------------------------
import os
os.dupterm(None, 1)

# Réactivation
import os, machine
uart = machine.UART(0, 115200)
os.dupterm(uart, 1)



# Envoi d'un message à l'arduino
# -------------------------------
from machine import UART
uart = UART(0, baudrate=9600)
uart.write('GET,MASS,3;')


# Lecture d'une carte RFID:
# -------------------------
from mfrc522 import MFRC522 # https://github.com/danjperron/micropython-mfrc522
from utime import sleep

def uidToString(uid):
    mystring = ""
    for i in uid:
        mystring = "%02X" % i + mystring
    return mystring

rc522 = MFRC522(spi_id=0,sck=6,miso=4,mosi=7,cs=5,rst=3)

print("")
print("Placez une carte RFID pres du lecteur.")
print("")

while True:

    (stat, tag_type) = rc522.request(rc522.REQIDL)

    if stat == rc522.OK:
        (stat, uid) = rc522.SelectTagSN()

    if stat == rc522.OK:
        print("Carte detectee %s" % uidToString(uid))
        sleep(1) # delai pour éviter les lectures multiples
