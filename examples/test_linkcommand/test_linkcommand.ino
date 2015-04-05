/* La Fabrique

   LinkCommand class test on Arduino Nano 
   by David Beaudette
   
   Code that answers commands from the access server.
   
*/

#include <SPI.h>
#include <EEPROM.h>

#include <Event.h>
#include <Timer.h>

// Dummy radio implementation for this test to run without
// a sender
#include "RF24.h"

#include "nano_rfid_hal.h"
#include "AccessEvent.h"
#include "AccessTable.h"
#include "LinkCommand.h"

// Declare dummy radio
RF24 radio;

// Declare timer object and global counter
Timer t;

// Using implicit zero-initialisation  
unsigned long EventList::event_list_counter;

// Declare table of events
EventList eventList(&t, 50);

// Declare table of users
AccessTable table;

// Declare commmand manager
LinkCommand *link;

// System state variable
sys_state_t state;  

void setup() {
  SetPins();
  SPI.begin();
  Serial.begin(serialRate);
  
  Serial.println("nano-rfid-commutator");
  Serial.println("Configuring NRF24L01+.");
  link = new LinkCommand(&radio, &table, &eventList);
  
  // Signal self-test success
  FlashLed(grnLedPin, quickFlash, 3);
  
  // Set initial state
  state = IDLE;
}

void loop() {
  // Update timer
  t.update();
    
  // Check for received packet
  if (radio.available())
  {
    if(!link->processCommand(&state)) {
      Serial.println("Fatal error with received command.");
      Stall();
    }
  }
}
