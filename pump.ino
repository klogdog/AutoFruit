// WhiteBox Labs -- Tentacle Shield -- I2C asynchronous example -- YUN only
// https://www.whiteboxes.ch/tentacle
//
//
// This sample code was written on an Arduino YUN, and depends on it's Bridge library to
// communicate wirelessly. For Arduino Mega, Uno etc, see the respective examples.
// It will allow you to control up to 8 Atlas Scientific devices through the I2C bus.
//
// This example shows how to take readings from the sensors in an asynchronous way, completely
// without using any delays. This allows to do other things while waiting for the sensor data.
// To demonstrate the behaviour, we will blink a simple led in parallel to taking the readings.
// Notice how the led blinks at the desired frequency, not disturbed by the other tasks the
// Arduino is doing.
//
// USAGE:
//---------------------------------------------------------------------------------------------
// - Set all your EZO circuits to I2C before using this sketch.
//    - You can use the "tentacle-steup.ino" sketch to do so)
//    - Make sure each circuit has a unique I2C ID set
// - Change the variables TOTAL_CIRCUITS, channel_ids and channel_names to reflect your setup
// - To talk to the Yun console, select your Yun's name and IP address in the Port menu.
//    - The Yun will only show up in the Ports menu, if your computer is on the same Network as the Yun.
//
//---------------------------------------------------------------------------------------------
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//---------------------------------------------------------------------------------------------


#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>
#define TFT_CS     8
#define TFT_RST    9  // you can also connect this to the Arduino reset
                      // in which case, set this #define pin to 0!
#define TFT_DC     7

// Option 2: use any pins but a little slower!
#define TFT_SCLK 6   // set these to be whatever pins you like! (SCL on sainsmart)
#define TFT_MOSI 10   // set these to be whatever pins you like! (SDA on sainsmart)
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);



#include <Wire.h>                              // enable I2C.

#define TOTAL_CIRCUITS 1                       // <-- CHANGE THIS | set how many I2C circuits are attached to the Tentacle

const unsigned int serial_host  = 9600;        // set baud rate for host serial monitor(pc/mac/other)
const unsigned int send_readings_every = 5000; // set at what intervals the readings are sent to the computer (NOTE: this is not the frequency of taking the readings!)
unsigned long next_serial_time;

char sensordata[30];                          // A 30 byte character array to hold incoming data from the sensors
byte sensor_bytes_received = 0;               // We need to know how many characters bytes have been received
byte code = 0;                                // used to hold the I2C response code.
byte in_char = 0;                             // used as a 1 byte buffer to store in bound bytes from the I2C Circuit.

int channel_ids[] = {99};        // <-- CHANGE THIS. A list of I2C ids that you set your circuits to.
char *channel_names[] = {"PH"};   // <-- CHANGE THIS. A list of channel names (must be the same order as in channel_ids[]) - only used to designate the readings in serial communications
String readings[TOTAL_CIRCUITS];               // an array of strings to hold the readings of each channel
int channel = 0;                              // INT pointer to hold the current position in the channel_ids/channel_names array

const unsigned int reading_delay = 1000;      // time to wait for the circuit to process a read command. datasheets say 1 second.
unsigned long next_reading_time;              // holds the time when the next reading should be ready from the circuit
boolean request_pending = false;              // wether or not we're waiting for a reading

float currentpH = 0;
const int pHDownPump = 11;
const int pHUpPump = 12;
unsigned long coolDown = 0;
unsigned long pHUpTimer = 0;
unsigned long pHDownTimer = 0;
boolean pumpOnDown = false;
boolean pumpOnUp = false;

void setup() {
  tft.initR(INITR_BLACKTAB); 
  uint16_t time = millis();
  tft.fillScreen(ST7735_BLACK);
  time = millis() - time;
  tft.fillScreen(ST7735_BLACK);
  tft.setTextSize(3);
  delay(1000);  
  pinMode(pHDownPump,OUTPUT);
  pinMode(pHUpPump,OUTPUT);
  Wire.begin();                    // enable I2C port.
  next_serial_time = millis() + send_readings_every;  // calculate the next point in time we should do serial communications
  digitalWrite(pHDownPump,HIGH);
  digitalWrite(pHUpPump,HIGH);
  do_sensor_readings();
  delay(10000);
}



void loop() {
  Serial.println(currentpH);
  do_sensor_readings();
  delay(1000);
  currentpH = readings[0].toFloat(); 
  tft.setCursor(0, 0);
  tft.setTextWrap(true);
  tft.fillScreen(ST7735_BLACK);
  tft.println("pH"); 
  tft.println(" ");   
  tft.print(currentpH, 1);  
  if (currentpH > 6.00){
    if (millis() >= coolDown){
    pHDown();
     }
  }
  if (currentpH < 5.60){
    if (millis() >= coolDown){
    pHUp();
    }
  }  
  if (pumpOnDown == true){
    if (millis() >= pHDownTimer){
      digitalWrite(pHDownPump,HIGH);
      pumpOnDown = false;
    }
   }
  if(pumpOnUp == true){   
    if (millis() >= pHUpTimer){
      digitalWrite(pHUpPump,HIGH);
      pumpOnUp = false;
    }    
  }

}




void pHDown(){
  digitalWrite(pHDownPump,LOW);
  pHDownTimer = millis() + 2000;
  coolDown = millis() + 60000;
  pumpOnDown = true;
}


void pHUp(){
  digitalWrite(pHUpPump,LOW);
  pHUpTimer = millis() + 2000;
  coolDown = millis() + 60000;
  pumpOnUp = true;
}

// do serial communication in a "asynchronous" way
void do_serial() {
  if (millis() >= next_serial_time) {                // is it time for the next serial communication?
    for (int i = 0; i < TOTAL_CIRCUITS; i++) {       // loop through all the sensors
      Serial.print(channel_names[i]);                // print channel name
      Serial.print(':');
      Serial.print(readings[i]);                     // print the actual reading
      Serial.println("");                            // jump to a new line
    }
    next_serial_time = millis() + send_readings_every;
  }
}


// take sensor readings in a "asynchronous" way
void do_sensor_readings() {
  if (request_pending) {                          // is a request pending?
    if (millis() >= next_reading_time) {          // is it time for the reading to be taken?
      receive_reading();                          // do the actual I2C communication
    }
  } else {                                        // no request is pending,
    channel = (channel + 1) % TOTAL_CIRCUITS;     // switch to the next channel (increase current channel by 1, and roll over if we're at the last channel using the % modulo operator)
    request_reading();                            // do the actual I2C communication
  }
}



// Request a reading from the current channel
void request_reading() {
  request_pending = true;
  Wire.beginTransmission(channel_ids[channel]); // call the circuit by its ID number.
  Wire.write('r');                    // request a reading by sending 'r'
  Wire.endTransmission();                   // end the I2C data transmission.
  next_reading_time = millis() + reading_delay; // calculate the next time to request a reading
}



// Receive data from the I2C bus
void receive_reading() {
  sensor_bytes_received = 0;                        // reset data counter
  memset(sensordata, 0, sizeof(sensordata));        // clear sensordata array;

  Wire.requestFrom(channel_ids[channel], 48, 1);    // call the circuit and request 48 bytes (this is more then we need).
  code = Wire.read();

  while (Wire.available()) {          // are there bytes to receive?
    in_char = Wire.read();            // receive a byte.

    if (in_char == 0) {               // if we see that we have been sent a null command.
      Wire.endTransmission();         // end the I2C data transmission.
      break;                          // exit the while loop, we're done here
    }
    else {
      sensordata[sensor_bytes_received] = in_char;  // load this byte into our array.
      sensor_bytes_received++;
    }
  }

  switch (code) {                       // switch case based on what the response code is.
    case 1:                             // decimal 1  means the command was successful.
      readings[channel] = sensordata;
      break;                              // exits the switch case.

    case 2:                             // decimal 2 means the command has failed.
      readings[channel] = "error: command failed";
      break;                              // exits the switch case.

    case 254:                           // decimal 254  means the command has not yet been finished calculating.
      readings[channel] = "reading not ready";
      break;                              // exits the switch case.

    case 255:                           // decimal 255 means there is no further data to send.
      readings[channel] = "error: no data";
      break;                              // exits the switch case.
  }
  request_pending = false;                  // set pending to false, so we can continue to the next sensor
}
