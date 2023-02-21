/* DBF THRUST STAND CONTROLLER FOR ARDUINO NANO
 * 
 * OPERATION OF THRUST STAND:
 *
 * Insert SD card, then reset Arduino Nano.
 * Status light should be red until the SD card is opened, then turn green.
 * If status light never turns green, data will not write to card.
 * Inserting a card without reset will not allow writing until the next reset.
 * Green light does not imply that data is correct, 
 * but does imply that load cell was recognized.
 * 
 * Data will always write to Serial Monitor.
 * Lack of SD card or USB cable will not interfere with each other.
 * Loss of power will not interfere with data saving to SD card
 * Serial to .txt: https://freeware.the-meiers.org/
 * Serial to excel: Not implemented :(
 * 
 * Data on SD card will be named "THRUST_XX.TXT".
 * XX is the number of the test performed.
 * The Arduino Nano counts tests 1-by-1, increasing,  
 * and does not reset the counter.
 * 
 * Data format: (Milliseconds since reset), (Thrust in grams)
 */

// HX722 Libraries
#include <HX711_ADC.h>
#if defined(ESP8266)|| defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif

// Display library
//#include <LiquidCrystal_I2C.h>
//#include <Wire.h>

// MicroSD Libraries
#include <SPI.h>
#include <SD.h>

// SD pins:
const int HX711_dout = 4; //mcu > HX711 dout pin
const int HX711_sck = 5; //mcu > HX711 sck pin

//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);

const int calVal_eepromAdress = 0;
unsigned long t = 0;

// SD File
File myFile;
long index;
String filename;

// Status LEDs
#define RED A1
#define GREEN A2

void setup() {
    pinMode(RED, OUTPUT);
    pinMode(GREEN, OUTPUT);
    greenLight(false);
    Serial.begin(57600); delay(10);
    Serial.println();
    Serial.println("Starting...");

    LoadCell.begin();
    //LoadCell.setReverseOutput(); //uncomment to turn a negative output value to positive
    float calibrationValue; // calibration value (see example file "Calibration.ino")
    //calibrationValue = 696.0; // uncomment this if you want to set the calibration value in the sketch
#if defined(ESP8266)|| defined(ESP32)
    //EEPROM.begin(512); // uncomment this if you use ESP8266/ESP32 and want to fetch the calibration value from eeprom
#endif
    EEPROM.get(calVal_eepromAdress, calibrationValue); // uncomment this if you want to fetch the calibration value from eeprom

    // Start the load cell
    unsigned long stabilizingtime = 2000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
    boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
    LoadCell.start(stabilizingtime, _tare);
    if (LoadCell.getTareTimeoutFlag()) {
        Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
        while (1);
    }
    else {
        LoadCell.setCalFactor(calibrationValue); // set calibration value (float)
        Serial.println("Startup is complete");
    }

    // Initialize SD card
    if (!SD.begin(10)) {
        Serial.println("initialization failed!");
    }
    Serial.println("initialization done.");

    // File index
    EEPROM.get(calVal_eepromAdress + 4, index);
    index++;
    EEPROM.put(calVal_eepromAdress + 4, index);
    
    // Open file, turn green LED on if successful
    filename = "thrust_" + String(index % 100) + ".txt";
    myFile = SD.open(filename, FILE_WRITE);
    delay(500);
    Serial.println("Attempting to open file: " + filename + " --> " + myFile);
    if (myFile) {
        Serial.println("Open SD file passed!");
        myFile.println("Start of test " + String(index));
        myFile.close();
        greenLight(true);
    } else {
        Serial.println("Open SD file failed!");
    }
}

void loop() {
    static boolean newDataReady = 0;
    const int serialPrintInterval = 0; //increase value to slow down serial print activity
    
    // check for new data/start next conversion:
    if (LoadCell.update()) newDataReady = true;
    
    // get smoothed value from the dataset:
    if (newDataReady) {
        if (millis() > t + serialPrintInterval) {
            float i = LoadCell.getData();
            t = millis();
            Serial.println("" + String(t) + ", " + String(i, 4));
            newDataReady = 0;
            
            // Write data to file
            myFile = SD.open(filename, FILE_WRITE);
            if (myFile) {
                myFile.println("" + String(t) + ", " + String(i, 4));
                myFile.close();
            }
        }
    }
}

// Multiplexed 2x1 LED matrix controller
// Green light turns on if SD setup is completed
void greenLight(bool do_green) {
    digitalWrite(RED, !do_green);
    digitalWrite(GREEN, do_green);
}
