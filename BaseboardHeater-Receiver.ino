#include <DHT.h>
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
#include <AccelStepper.h>
//#include "DigitalIO.h"

#define DHTPIN 8     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
#define MOTOR_DIR_PIN A3
#define MOTOR_STEP_PIN A2
#define MOTOR_ENABLE_PIN A4
#define MOTOR_RADIUS 1325 // How far the motor should turn
#define TEMP_HYSTERESIS 1 // Range of degrees before changing temperature
#define RADIO_CS 9
#define RADIO_EN 10
#define STATUS_LED 13

DHT dht(DHTPIN, DHTTYPE);
RF24 radio(RADIO_EN,RADIO_CS);
// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };
AccelStepper stepper(AccelStepper::DRIVER,MOTOR_STEP_PIN,MOTOR_DIR_PIN);
uint32_t _lastreadtime;
int curTemperature = 0;
int targetTemperature = 0;
int changeDelay = 0;
bool heatingMode = false;

void setup() {
  Serial.begin(57600);
  printf_begin();
  printf("=========== Loading ===========\n");
  dht.begin();
  stepper.setEnablePin(MOTOR_ENABLE_PIN);
  stepper.disableOutputs(); // Save power and noise
  stepper.setMaxSpeed(3000);
  stepper.setAcceleration(1000);
  //
  // Setup and configure rf radio
  //

  radio.begin();
   
 // Set the proper comm channels
  radio.openWritingPipe(pipes[0]);
  radio.openReadingPipe(1,pipes[1]);
  radio.startListening();
  radio.printDetails();
  pinMode(STATUS_LED, OUTPUT); // Test LED
 
}

void loop() {
  // Send the temperature
  
      radio.stopListening();
      uint8_t data[32];  // we'll transmit a 32 byte packet
      data[0] = dht.readHumidity();
      curTemperature = dht.readTemperature(true);
      data[1] = curTemperature;
      radio.write( &data, sizeof(data) );
      radio.startListening();
      if(targetTemperature == 0) {targetTemperature = curTemperature;} // Init the temps
      
      // Look for commands
      uint8_t command_data[32];  // we'll receive a 32 byte packet
      if(radio.available())
      {
        radio.read( &command_data, sizeof(command_data) );
        targetTemperature = command_data[0]; // First byte in packet is the target temperature
        printf("Target temperature = %d",targetTemperature);
          
      }
        

     printf("Current temperature is: %d degrees, target is: %d \n\r",curTemperature,targetTemperature);
     // Temperature control code
     
     // if current < target, turn heater on (enable heating mode)
     // if current >= target, turn heater off (cooling mode)
     // if cooling mode is on, and current + range < target, turn heater on (enable heating mode)
     
      // if in heating mode
      // when temp = target, turn off (cool)
      
        if(curTemperature >= targetTemperature) {
          heatingMode = false;
          turnHeaterOff();
          printf("Turn down the temperature or stay even");
        
        }
     
   
      if(curTemperature < (targetTemperature - TEMP_HYSTERESIS)){
          heatingMode = true;
          turnHeaterOn();
          printf("Turn UP the temperature");
          digitalWrite(STATUS_LED,HIGH);
        }
      
      
      delay(200);
      digitalWrite(STATUS_LED,LOW);
}

void turnHeaterOn(){
  
    // Position is at zero (off)
    if(stepper.currentPosition() == 0){
      stepper.enableOutputs();
     stepper.runToNewPosition(MOTOR_RADIUS);
     stepper.disableOutputs();
  
    
  }
}
void turnHeaterOff(){
  
    if (stepper.currentPosition() != 0) {
      stepper.enableOutputs(); 
      stepper.runToNewPosition(0);
      stepper.disableOutputs();
     
  }
 
}

