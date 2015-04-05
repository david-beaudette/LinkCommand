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
LinkCommand link(&table, &eventList);

// System state variable
sys_state_t state;  

void setup() {
  SetPins();
  SPI.begin();
  Serial.begin(serialRate);
  
  Serial.println("nano-rfid-commutator");
  Serial.println("Configuring NRF24L01+.");
  RadioConfig(&radio);
  
  // Signal self-test success
  FlashLed(grnLedPin, quickFlash, 3);
  
  // Set initial state
  state = IDLE;
}

void loop() {
  // Initialise command buffer with 8 bytes, which 
  // is 1 more than the longest command (table update)
  byte cmd[7];
  byte cmd_len;
  // Processed length
  int  prc_len;
  // Initialise reply buffer with 10 bytes, which 
  // is 1 more than the longest reply (dump logging)
  byte reply[10];
  byte reply_len;
  
  // Update timer
  t.update();
    
  // Check for received packet
  if(radio.available())
  {
    // Check number of bytes received
    cmd_len = radio.getDynamicPayloadSize();
    if(cmd_len > 0)
    {   
      // Read all command bytes from the receive buffer
      radio.read(&cmd, cmd_len);
      
      // Flush buffers and switch to TX mode
      radio.stopListening();
      
      // Let LinkCommand handle this command
      prc_len = link.processCommand(cmd, reply, 
                                    &reply_len, &state);
      if(prc_len == -1) {
        // Signal error in processing
        Serial.println("Error processing received command.");
        FlashLed(redLedPin, slowFlash, 5);
        return;
      }
      // Reply to sender
      if(!radio.write(reply, reply_len))   {
        printf("Unable to reply to sender.\n\r");
      }        
      // Check if received packet contained another command
      if(prc_len < cmd_len) {
        Serial.println("At least one command was ignored in the packet...");
      }
      // Return to receiving state
      radio.startListening();
    }
  }
}
