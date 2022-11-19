#include <Wire.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>    // LiquidCrystal_I2C v1.1.2
#include <Keypad.h>               // Keypad v3.1.1
#include "HX711.h"                // HX711 Arduino library v0.3.3    https://github.com/RobTillaart/HX711


// Keypad configuration:
// ---------------------
const byte ROWS = 4; // Define the number of rows on the keypad
const byte COLS = 3; // Define the number of columns on the keypad
char keys[ROWS][COLS] = { // Matrix defining character to return for each key
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {26, 27, 29, 30}; //connect to the row pins (R0-R3) of the keypad  : Pins from left to right : P1, P5, P6, P3
byte colPins[COLS] = {28, 24, 25}; //connect to the column pins (C0-C2) of the keypad   : Pins from left to right : P2, P0, P4
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );
String keypad_number = "";


// LCD configuration:
// ------------------
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 20, 4);   // 20, 4 --> LCD2004 (20 columns, 4 rows)


// Weight sensors:
// ---------------
const int NBC = 1+8;  // nombre de bouteilles + 1 pour le verre
HX711* cells = new HX711[NBC];
const int pin_cells[NBC] = {11, 3, 4, 5, 6, 7, 8, 9, 10};      // {3, 4, 5, 6, 7, 8, 9, 10}; --> RELAIS : {13+8;}
const int LC_SCK = 2;


// Valves :
// --------
const int valves[NBC-1] = {39, 40, 41, 42, 43, 44, 45, 46}; // {52, 50, 48, 46, 44, 42, 40, 38};


// Pumps:
// ------
const int pumps[NBC-1] = {31, 32, 33, 34, 35, 36, 37, 38}; // {53, 51, 49, 47, 45, 43, 41, 39};


// Serial configuration:
// ---------------------
const int SERIAL_BAUDRATE=9600;
const int rxPin=13;
const int txPin=12;
const char ENDMSG=';';

SoftwareSerial esp = SoftwareSerial(rxPin, txPin);

String getArgFromMsg(String data, char separator, int index){
    int found = 0;
    int strIndex[] = {0, -1};
    int maxIndex = data.length()-1;
    for(int i=0; i<=maxIndex && found<=index; i++){
        if(data.charAt(i)==separator || i==maxIndex){
            found++;
            strIndex[0] = strIndex[1]+1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}


void cleanLCD(){
    for (int line=0; line<4; line++){
        lcd.setCursor(0, line);
        lcd.print("                    ");
    }
}


void setup() {
    // Activation de la communication Arduino <-> ESP8266 (MicroPython)
    pinMode(rxPin, INPUT);
    pinMode(txPin, OUTPUT);
    esp.begin(SERIAL_BAUDRATE);

    Serial.begin(SERIAL_BAUDRATE);
    Serial.println("Configuration des capteurs de poids.");
    // Weight sensors configuration:
    for (int n=0; n<NBC; n++){
        cells[n].begin(pin_cells[n], LC_SCK);
        switch (n){
            case 0:
                cells[n].set_scale(-838.29);
                cells[n].set_offset(-3742);
            case 1:
                cells[n].set_scale(-862.78);
                cells[n].set_offset(-185281);
            case 2:
                cells[n].set_scale(-934.78);
                cells[n].set_offset(-3089954);
            case 8:
                cells[n].set_scale(-838.29);
                cells[n].set_offset(-3742);
            default:
                break;
        }
    }

    // Configuration des valves:
    for (int n=0; n<NBC-1; n++){
        pinMode(valves[n], OUTPUT);
        digitalWrite(valves[n], HIGH);
    }

    // Configuration des pompes:
    for (int n=0; n<NBC-1; n++){
        pinMode(pumps[n], OUTPUT);
        digitalWrite(pumps[n], HIGH);
    }

    Serial.println("Initialisation de l'écran LCD.");
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("     JoBar v0.1    ");

    Serial.println("Setup end.");
}


void loop() {

    delay(10);

    // Gestion des communications entrantes et actions associées
    // ---------------------------------------------------------
    if (esp.available()) {
        Serial.print("Données UART disponibles...");

        String msg = esp.readStringUntil(ENDMSG);
        Serial.println("msg = " + msg);
        String cmd = getArgFromMsg(msg, ',', 0);

        if (cmd == "GPIO_FIND"){
            
            lcd.setCursor(0, 0);
            for (int i=0; i<8;i++){
              cleanLCD();
              lcd.print("Valve " + String(i+1));
              delay(1000);
              digitalWrite(valves[i], LOW);
              delay(5000);
              digitalWrite(valves[i], HIGH);
              delay(1000);
              cleanLCD();
              lcd.print("Pump " + String(i+1));
              delay(1000);
              digitalWrite(pumps[i], LOW);
              delay(5000);
              digitalWrite(pumps[i], HIGH);
              delay(1000);
            }
        }

        // Distribution d'une boisson:
        // ---------------------------
        if (cmd == "DISPENSE"){
            Serial.println("cmd == 'DISPENSE'");
            int bottle = getArgFromMsg(msg, ',', 1).toInt();
            float target_mass = getArgFromMsg(msg, ',', 2).toFloat();
            float empty_glass = cells[0].get_units(3);
            // Distribution active
            digitalWrite(pumps[bottle], LOW);
            float current_mass = cells[0].get_units(3) - empty_glass;
            while (current_mass <= target_mass){
                current_mass = cells[0].get_units(3) - empty_glass;
            }
            // Arrêt de la distribution active
            digitalWrite(pumps[bottle], HIGH);
            // Ouverture de la valve pour arrêter la distribution passive
            digitalWrite(valves[bottle], LOW);
            delay(2000);
            // Referme la valve lorsqu'on ajoute moins de 1g/2s en distribution passive
            while ((cells[0].get_units(3)-empty_glass) - current_mass > 1){
                delay(2000);
                current_mass = cells[0].get_units(3) - empty_glass;
            }
            digitalWrite(valves[bottle], HIGH);
            Serial.println("DISPENSE end.");
            esp.print(msg + ":END;");
        }

        // Print to screen:
        // ----------------
        if (cmd == "MSG"){
            Serial.println("cmd == 'MSG'");
            int line = getArgFromMsg(msg, ',', 1).toInt();
            String content = getArgFromMsg(msg, ',', 2);
            lcd.setCursor(0, line);
            lcd.print(content);
            Serial.println("MSG end.");
        }
        
        if (cmd == "CLEAN_SCREEN"){
            Serial.println("cmd == 'CLEAN_SCREEN'");
            cleanLCD();
            esp.print(msg + ":END;");
            Serial.println("CLEAN_SCREEN end.");
        }
        
        // Get mass from sensors:
        // ----------------------
        if (cmd == "MASS"){
            Serial.println("cmd == 'MASS'");
            int bottle = getArgFromMsg(msg, ',', 1).toInt();
            float mass = cells[bottle].get_units(5);
            esp.println(msg + ":" + String(mass) + ";");
            Serial.println("MASS end.");
        }

        // Tare weight sensor:
        // -------------------
        if (cmd == "TARE"){
            Serial.println("cmd == 'TARE'");
            int bottle = getArgFromMsg(msg, ',', 1).toInt();
            cells[bottle].tare();
            esp.print(msg + ":END;");
            Serial.println("TARE end.");
        }

        // Configure weight sensor:
        // ------------------------
        if (cmd == "CONFIG_SCALE"){
            Serial.println("cmd == 'CONFIG_SCALE'");
            int bottle = getArgFromMsg(msg, ',', 1).toInt();

            cleanLCD();
            lcd.setCursor(0, 0);
            lcd.print("Bouteille 1:");
            lcd.setCursor(0, 1);
            lcd.print("Retirer les poids");
            lcd.setCursor(0, 2);
            for (int i=10; i>0;i--){
                delay(1000);
                lcd.print(i);
            }
            cells[bottle].set_scale();
            cells[bottle].tare();

            cleanLCD();
            lcd.setCursor(0, 0);
            lcd.print("Masse de 1000g");
            lcd.setCursor(0, 1);
            for (int i=10; i>0;i--){
                delay(1000);
                lcd.print(i);
            }
            float mass_calibrate = 919;
            cells[bottle].calibrate_scale(mass_calibrate);
            Serial.println("scale =" + String(cells[bottle].get_scale()));
            Serial.println("offset =" + String(cells[bottle].get_offset()));


            cleanLCD();
            Serial.println("CONFIG_SCALE end.");
        }
    }

    // Send information when key on keypad is pressed:
    // -----------------------------------------------
    char key = keypad.getKey();
    
    
    if (key){
        Serial.println("Touche clavier utilisé.");

        if (key == '*'){
            keypad_number = "";
            lcd.setCursor(0, 3);
            lcd.print("    mL              ");
        }
        else if (key == '#'){
            //esp.write("KEYPAD,");
            esp.println("KEYPAD," + keypad_number + ";");
        }
        else {
            lcd.setCursor(4, 3);
            lcd.print("mL");
            keypad_number = keypad_number + String(key);
            lcd.setCursor(0, 3);
            lcd.print(keypad_number);
        }
    }
}
