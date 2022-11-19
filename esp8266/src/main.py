from utime import sleep
from machine import UART, Pin
import os

from mfrc522 import MFRC522 # https://github.com/danjperron/micropython-mfrc522

WAIT_TIME = 0.25

uart = UART(0, baudrate=9600)

CARDS = {
    # Drinks
    "16E834C3": "Rhum blanc",
    "174E7BB3": "Rhum ambre",
    "172799D3": "Citron vert",
    "1594FF03": "Cachaca",
    "15AC9D53": "Coca",
    # Cocktails
    "152FB0D3": "Caipirinha",
    "1594D803": "Cuba libre",
    "159B5FA3": "Ti Punch"
}

DRINKS = (
    "Rhum blanc",
    "Rhum ambre",
    "Citron vert",
    "Cachaca",
    "Coca"
)

COCKTAILS = {
    "Caipirinha": {"Cachaça": 50, "Citron vert": 25, "Sucre": 10},
    "Cuba libre": {"Rhum ambré": 40, "Citron vert": 10, "Coca": 150},
    "Ti Punch": {"Rhum blanc": 50, "Citron vert": 10, "Sucre": 5},
    "Mojito": {"Rhum ambré": 40, "Citron vert": 5, "Sucre": 10, "Eau gazeuse": 60}
}

BOTTLES = {
    1: "Citron vert",
    2: None,
    3: None,
    4: None,
    5: None,
    6: None,
    7: None,
    8: "Rhum ambre"
}

# Cuillère à café : 5mL / 5g
# Cuillère à soupe : 15mL / 15g
# Un citron vert entier équivalent à 2 càs de jus soit 30mL / 30g# Cuillère à café : 5mL / 5g
# Cuillère à soupe : 15mL / 15g
# Un citron vert entier équivalent à 2 càs de jus soit 30mL / 30g

def screen_print(line, msg):
    uart.write("MSG,3,{:^20}".format("Appuyer sur #"))

def uidToString(uid):
    mystring = ""
    for i in uid:
        mystring = "%02X" % i + mystring
    return mystring

rc522 = MFRC522(spi_id=0,sck=0,miso=4,mosi=2,cs=14,rst=5)

while True:
    #incoming_msg = uart.readline()
    #print(incoming_msg)
    #uart.write("MSG,1,%s;" % incoming_msg)

    (stat, tag_type) = rc522.request(rc522.REQIDL)
    if stat == rc522.OK:
        (stat, uid) = rc522.SelectTagSN()
    if stat == rc522.OK:
        suid = uidToString(uid)
        print("UID card = %s" % suid)
        if suid in CARDS:
            cardname = CARDS[suid]
            if cardname in DRINKS:
                print("Boisson détectée: %s" % cardname)
                uart.write("CLEAN_SCREEN;")
                uart.write("MSG,0,{:^20};".format(cardname))
                sleep(WAIT_TIME)
                uart.write("MSG,1,{:^20};".format("Choisir la quantite"))
                sleep(WAIT_TIME)
                uart.write("MSG,2,{:^20};".format("* Annuler # Valider"))
                sleep(WAIT_TIME)
                uart.write("MSG,3,{};".format("    mL             "))

                # Attente du retour d'information de la quantité:
                content = uart.read()
                while content == '':
                    content = uart.read()
                    sleep(2*WAIT_TIME)

                # Distribution de la bouteille et quantité voulu
                target_mass = int(content[:-1].replace('KEYPAD,'))
                if cardname in BOTTLES.values():
                    for bottleid, name in BOTTLES.items():
                        if name == cardname:
                            uart.write("DISPENSE,{},{};".format(bottleid, target_mass))
                else:
                    uart.write("MSG,1,{:^20};".format("Non disponible"))
                    uart.write("MSG,2,{:^20};".format(" "))
                    uart.write("MSG,3,{:^20};".format(" "))
            elif cardname in COCKTAILS:
                print("Cocktail détecté: %s" % cardname)
                ingredients = COCKTAILS[cardname]
                uart.write("CLEAN_SCREEN;")
                for bottleid, name in BOTTLES.items():
                    pass
        sleep(0.5) # delai pour éviter les lectures multiples

# Commands:
# ---------
# MASS,2;           --> renvoi la masse de la cellule 2 sous le format : MASS,2:563; pour 563gr
# TARE,3;           --> Fais une tare de la cellule 3
# MSG,0;bonjour     --> Affiche sur la ligne 0 le message bonjour sur l'écran LCD
# DISPENSE,1,50     --> Distribue 50mL depuis la bouteille 1
# CONFIG_SCALE,3    --> Calibration de la cellule 3
