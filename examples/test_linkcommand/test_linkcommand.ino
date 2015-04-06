/** La Fabrique

   LinkCommand class test on Arduino Nano 
   by David Beaudette
   
   Code that answers commands from the access server.
   
**/

#include <SPI.h>
#include <EEPROM.h>

#include <Event.h>
#include <Timer.h>

#include "nano_rfid_hal.h"
#include "AccessEvent.h"
#include "AccessTable.h"
#include "LinkCommand.h"

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

// Test configuration
int num_test_events =  5; // Number of events logged before dump command is executed
int num_users_added = 20; // Number of users added by table update command

// Initialise command buffer with 8 bytes, which 
// is 1 more than the longest command (table update)
byte cmd[7];
// Processed length
int  prc_len;
// Initialise reply buffer with 10 bytes, which 
// is 1 more than the longest reply (dump logging)
byte reply[10];
byte reply_len;

void setup() {
  SetPins();
  Serial.begin(serialRate);
  
  Serial.println(F("test_linkcommand"));
  
  // Signal self-test success
  FlashLed(grnLedPin, slowFlash, 1);
  
  // Set initial state
  state = IDLE;
}

void loop() {
  int test_output = 0;
  // Check each command
  if(CheckAutoCmd() == 0) {
    Serial.println(F("AUTO command validated."));
  }
  else {
    Serial.println(F("AUTO command test failed."));
    test_output = -1;
  }
  if(CheckEnableCmd() == 0) {
    Serial.println(F("ENABLE command validated."));
  }
  else {
    Serial.println(F("ENABLE command test failed."));
    test_output = -1;
  }
  if(CheckDisableCmd() == 0) {
    Serial.println(F("DISABLE command validated."));
  }
  else {
    Serial.println(F("DISABLE command test failed."));
    test_output = -1;
  }
  if(CheckDumpLoggingCmd() == 0) {
    Serial.println(F("DUMPLOGGING command validated."));
  }
  else {
    Serial.println(F("DUMPLOGGING command test failed."));
    test_output = -1;
  }
  // Wait for user input before performing tests
  // involving EEPROM
  // Wait for user input
  Serial.println(F("Send character to perform tests involving EEPROM."));
  while(Serial.available() <= 0) {
    delay(500);
  }
  if(CheckMemoryClearCmd() == 0) {
    Serial.println(F("MEMORYCLEAR command validated."));
  }
  else {
    Serial.println(F("MEMORYCLEAR command test failed."));
    test_output = -1;
  }
  if(CheckTableUpdateCmd() == 0) {
    Serial.println(F("TABLEUPDATE command validated."));
  }
  else {
    Serial.println(F("TABLEUPDATE command test failed."));
    test_output = -1;
  }
  if(CheckMemoryCheckCmd() == 0) {
    Serial.println(F("MEMORYCHECK command validated."));
  }
  else {
    Serial.println(F("MEMORYCHECK command test failed."));
    test_output = -1;
  }
  // Display test result
  if(test_output == 0) {
    Serial.println(F("All tests succeeded."));
  }
  else {
    Serial.println(F("One or more tests failed."));
  }
  Stall();
}

int CheckAutoCmd() {
  int test_output = 0;
  // CMD_AUTO
  cmd[0] = CMD_AUTO;
  // Handle this command
  prc_len = link.processCommand(cmd, reply, &reply_len, &state);
  
  // Check function outputs
  if(prc_len != 1) {
    Serial.println(F("LinkCommand should return 1 processed byte for a auto command."));
    test_output = -1;
  }
  if(reply_len != 1) {
    Serial.println(F("LinkCommand should reply with 1 byte for a auto command."));
    test_output = -1;
  }
  if(reply[0] != REPLY_OK) {
    Serial.println(F("LinkCommand should reply Ok."));
    test_output = -1;
  }
  // Check the effect of the command for each state
  state  = ENABLED;
  link.processCommand(cmd, reply, &reply_len, &state);
  if(state != IDLE) {
    Serial.println(F("LinkCommand state not set properly by auto command."));
    test_output = -1;
  }
  state  = DISABLED;
  link.processCommand(cmd, reply, &reply_len, &state);
  if(state != IDLE) {
    Serial.println(F("LinkCommand state not set properly by auto command."));
    test_output = -1;
  }
  state  = ACTIVATED;
  link.processCommand(cmd, reply, &reply_len, &state);
  if(state != ACTIVATED) {
    Serial.println(F("LinkCommand state not set properly by auto command."));
    test_output = -1;
  }
  state  = IDLE;
  link.processCommand(cmd, reply, &reply_len, &state);
  if(state != IDLE) {
    Serial.println(F("LinkCommand state not set properly by auto command."));
    test_output = -1;
  }
  state  = TRIGGEREDONCE;
  link.processCommand(cmd, reply, &reply_len, &state);
  if(state != TRIGGEREDONCE) {
    Serial.println(F("LinkCommand state not set properly by auto command."));
    test_output = -1;
  } 
  return test_output;
}

int CheckEnableCmd() {
   int test_output = 0;
 // CMD_ENABLE
  cmd[0] = CMD_ENABLE;
  // Handle this command
  prc_len = link.processCommand(cmd, reply, &reply_len, &state);
  
  // Check function outputs
  if(prc_len != 1) {
    Serial.println(F("LinkCommand should return 1 processed byte for enable command."));
    test_output = -1;
  }
  if(reply_len != 1) {
    Serial.println(F("LinkCommand should reply with 1 byte for enable command."));
    test_output = -1;
  }
  if(reply[0] != REPLY_OK) {
    Serial.println(F("LinkCommand should reply Ok."));
    test_output = -1;
  }
  // Check the effect of the command for each state
  state  = ENABLED;
  link.processCommand(cmd, reply, &reply_len, &state);
  if(state != ENABLED) {
    Serial.println(F("LinkCommand state not set properly by enable command."));
    test_output = -1;
  }
  state  = DISABLED;
  link.processCommand(cmd, reply, &reply_len, &state);
  if(state != ENABLED) {
    Serial.println(F("LinkCommand state not set properly by enable command."));
    test_output = -1;
  }
  state  = ACTIVATED;
  link.processCommand(cmd, reply, &reply_len, &state);
  if(state != ENABLED) {
    Serial.println(F("LinkCommand state not set properly by enable command."));
    test_output = -1;
  }
  state  = IDLE;
  link.processCommand(cmd, reply, &reply_len, &state);
  if(state != ENABLED) {
    Serial.println(F("LinkCommand state not set properly by enable command."));
    test_output = -1;
  }
  state  = TRIGGEREDONCE;
  link.processCommand(cmd, reply, &reply_len, &state);
  if(state != ENABLED) {
    Serial.println(F("LinkCommand state not set properly by enable command."));
    test_output = -1;
  } 
  return test_output;
}

int CheckDisableCmd() {
  int test_output = 0;
  // CMD_DISABLE
  cmd[0] = CMD_DISABLE;
  // Handle this command
  prc_len = link.processCommand(cmd, reply, &reply_len, &state);
  
  // Check function outputs
  if(prc_len != 1) {
    Serial.println(F("LinkCommand should return 1 processed byte for disable command."));
    test_output = -1;
  }
  if(reply_len != 1) {
    Serial.println(F("LinkCommand should reply with 1 byte for disable command."));
    test_output = -1;
  }
  if(reply[0] != REPLY_OK) {
    Serial.println(F("LinkCommand should reply Ok."));
    test_output = -1;
  }
  // Check the effect of the command for each state
  state  = ENABLED;
  link.processCommand(cmd, reply, &reply_len, &state);
  if(state != DISABLED) {
    Serial.println(F("LinkCommand state not set properly by disable command."));
    test_output = -1;
  }
  state  = DISABLED;
  link.processCommand(cmd, reply, &reply_len, &state);
  if(state != DISABLED) {
    Serial.println(F("LinkCommand state not set properly by disable command."));
    test_output = -1;
  }
  state  = ACTIVATED;
  link.processCommand(cmd, reply, &reply_len, &state);
  if(state != DISABLED) {
    Serial.println(F("LinkCommand state not set properly by disable command."));
    test_output = -1;
  }
  state  = IDLE;
  link.processCommand(cmd, reply, &reply_len, &state);
  if(state != DISABLED) {
    Serial.println(F("LinkCommand state not set properly by disable command."));
    test_output = -1;
  }
  state  = TRIGGEREDONCE;
  link.processCommand(cmd, reply, &reply_len, &state);
  if(state != DISABLED) {
    Serial.println(F("LinkCommand state not set properly by disable command."));
    test_output = -1;
  } 
  return test_output;
}

int CheckMemoryClearCmd() {
  int test_output = 0;
  // CMD_MEMORYCLEAR
  cmd[0] = CMD_MEMORYCLEAR;
  // Handle this command
  Serial.print(F("Clearing memory..."));
  prc_len = link.processCommand(cmd, reply, &reply_len, &state);
  Serial.println(F(" done."));

  // Check function outputs
  if(prc_len != 1) {
    Serial.println(F("LinkCommand should return 1 processed byte for memory clear command."));
    test_output = -1;
  }
  if(reply_len != 1) {
    Serial.println(F("LinkCommand should reply with 1 byte for memory clear command."));
    test_output = -1;
  }
  if(reply[0] != REPLY_OK) {
    Serial.println(F("LinkCommand should reply Ok."));
    test_output = -1;
  }
  if(table.getNumUsers() != 0) {
    Serial.println(F("Memory was not cleared by command."));
    test_output = -1;
  }
  return test_output;
}

int CheckTableUpdateCmd() {
  int test_output = 0;
  byte tag[] = {0x10, 0x20, 0x30, 0x40};
  cmd[0] = CMD_UPDATETABLE;
  
  // Add new authorized users to table
  Serial.println(F("Adding new users to table using CMD_UPDATETABLE."));
  for(int i = 0; i < num_users_added; i++) {
    cmd[1] = num_users_added-i;
    cmd[2] = 1;
    cmd[3] = tag[0]+i;
    cmd[4] = tag[1]+i;
    cmd[5] = tag[2]+i;
    cmd[6] = tag[3]+i;
    
    // Handle this command
    prc_len = link.processCommand(cmd, reply, &reply_len, &state);
    
    // Check function outputs
    if(prc_len != 7) {
      Serial.println(F("LinkCommand should return 7 processed bytes for table update command."));
      test_output = -1;
    }
    if(reply_len != 3) {
      Serial.println(F("LinkCommand should reply with 3 bytes for table update command."));
      test_output = -1;
    }
    if(reply[0] != REPLY_OK) {
      Serial.println(F("LinkCommand should reply Ok."));
      test_output = -1;
    }
    if(reply[1] != CMD_UPDATETABLE) {
      Serial.println(F("LinkCommand should reply with table update command."));
      test_output = -1;
    }
    if(reply[2] != TABLE_UPDATE_ADDED) {
      Serial.println(F("Table update user should be added."));
      test_output = -1;
    }
    if(table.getUserAuth(&cmd[3]) != 1) {
      Serial.println(F("Table was not built correctly."));
      test_output = -1;
    }
  }
  if(table.getNumUsers() != num_users_added) {
    Serial.println(F("Table size not as expected."));
    test_output = -1;
  }
  // Add the same users to table
  Serial.println(F("Adding existing users to table using CMD_UPDATETABLE."));
  for(int i = 0; i < num_users_added; i++) {
    cmd[1] = num_users_added-i;
    cmd[2] = 1;
    cmd[3] = tag[0]+i;
    cmd[4] = tag[1]+i;
    cmd[5] = tag[2]+i;
    cmd[6] = tag[3]+i;
    
    // Handle this command
    prc_len = link.processCommand(cmd, reply, &reply_len, &state);
    
    // Check function outputs
    if(prc_len != 7) {
      Serial.println(F("LinkCommand should return 7 processed bytes for table update command."));
      test_output = -1;
    }
    if(reply_len != 3) {
      Serial.println(F("LinkCommand should reply with 3 bytes for table update command."));
      test_output = -1;
    }
    if(reply[0] != REPLY_OK) {
      Serial.println(F("LinkCommand should reply Ok."));
      test_output = -1;
    }
    if(reply[1] != CMD_UPDATETABLE) {
      Serial.println(F("LinkCommand should reply with table update command."));
      test_output = -1;
    }
    if(reply[2] != TABLE_UPDATE_NOMOD) {
      Serial.println(F("Table update user should not be modified."));
      test_output = -1;
    }
    if(table.getUserAuth(&cmd[3]) != 1) {
      Serial.println(F("Table should not have been modified."));
      test_output = -1;
    }
  }
  if(table.getNumUsers() != num_users_added) {
    Serial.println(F("Table size not as expected."));
    test_output = -1;
  }
  // Change user authorization
  Serial.println(F("Changing authorizations in table using CMD_UPDATETABLE."));
  for(int i = 0; i < num_users_added; i++) {
    cmd[1] = num_users_added-i;
    cmd[2] = 0; // Changed to not authorized
    cmd[3] = tag[0]+i;
    cmd[4] = tag[1]+i;
    cmd[5] = tag[2]+i;
    cmd[6] = tag[3]+i;
    
    // Handle this command
    prc_len = link.processCommand(cmd, reply, &reply_len, &state);
    
    // Check function outputs
    if(prc_len != 7) {
      Serial.println(F("LinkCommand should return 7 processed bytes for table update command."));
      test_output = -1;
    }
    if(reply_len != 3) {
      Serial.println(F("LinkCommand should reply with 3 bytes for table update command."));
      test_output = -1;
    }
    if(reply[0] != REPLY_OK) {
      Serial.println(F("LinkCommand should reply Ok."));
      test_output = -1;
    }
    if(reply[1] != CMD_UPDATETABLE) {
      Serial.println(F("LinkCommand should reply with table update command."));
      test_output = -1;
    }
    if(reply[2] != TABLE_UPDATE_MOD) {
      Serial.println(F("Table update user authorization should be modified."));
      test_output = -1;
    }
    if(table.getUserAuth(&cmd[3]) != 0) {
      Serial.println(F("Table was not updated correctly."));
      test_output = -1;
    }
  }
  if(table.getNumUsers() != num_users_added) {
    Serial.println(F("Table size not as expected."));
    test_output = -1;
  }
  return test_output;
}

int CheckMemoryCheckCmd() {
  int test_output = 0;
  int memory_size;
  int memory_used;
  // Process command
  cmd[0] = CMD_MEMORYCHECK;
  prc_len = link.processCommand(cmd, reply, &reply_len, &state);
  
  // Check function outputs
  if(prc_len != 1) {
    Serial.println(F("LinkCommand should return 1 processed byte for memory check command."));
  }
  if(reply_len != 6) {
    Serial.println(F("LinkCommand should reply with 6 bytes for memory check command."));
  }
  if(reply[0] != REPLY_OK) {
    Serial.println(F("LinkCommand should reply Ok."));
  }
  if(reply[1] != CMD_MEMORYCHECK) {
    Serial.println(F("LinkCommand should reply with memory check command."));
  }
  memory_size = (int(reply[2]) << 8) + int(reply[3]);
  memory_used = (int(reply[4]) << 8) + int(reply[5]);
  Serial.print(F("Reported memory size: "));
  Serial.println(memory_size);
  if(memory_size != MAX_USER_SIZE) {
    test_output = -1;
  }
  Serial.print(F("Reported memory used: "));
  Serial.println(memory_used);
  if(memory_used != num_users_added) {
    test_output = -1;
  }
  return test_output;
}

int CheckDumpLoggingCmd() {
  int test_output = 0;
  // Add a few events to the list
  int event_type = 0x30;
  byte tag[] = {0x10, 0x20, 0x30, 0x40};
  for(int i = 0; i < num_test_events; i++) {
    Serial.print("Adding test event ");
    Serial.print(i+1);
    Serial.print(" of ");
    Serial.print(num_test_events);
    Serial.println(".");
    // Update timer
    t.update();
    
    // Add an event to the list
    eventList.addEvent(event_type, tag);  

    // Modify event_type
    event_type = (0x34 == event_type) ? 0x30 : (event_type+1);
    
    // Increment user tag
    tag[0] += 1;
    tag[1] += 2;
    tag[2] += 3;
    tag[3] += 4;
    
    // Wait one second to get different timestamps
    delay(1000);
  }  
    
  // Check what is returned by a dump logging command
  cmd[0] = CMD_DUMPLOGGING;
  
  for(int i = 0; i < num_test_events; i++) {
    // Handle this command
    prc_len = link.processCommand(cmd, reply, &reply_len, &state);
    
    // Check function outputs
    if(prc_len != 1) {
      Serial.println(F("LinkCommand should return 1 processed byte for a dump logging command."));
      test_output = -1;
    }
    if(reply_len != 9) {
      Serial.println(F("LinkCommand should reply with 9 bytes for a dump logging command."));
      test_output = -1;
    }
    if(reply[0] != REPLY_OK) {
      Serial.println(F("LinkCommand should reply Ok."));
      test_output = -1;
    }
    if(reply[1] != CMD_DUMPLOGGING) {
      Serial.println(F("LinkCommand should reply with dump logging command."));
      test_output = -1;
    }
    if(reply[2] != (num_test_events-i)) {
      Serial.println(F("LinkCommand should reply with remaining number of events."));
      test_output = -1;
    }
    if(reply[3] != (0x30+i)) {
      Serial.println(F("LinkCommand returned wrong event type."));
      test_output = -1;
    }
    if(reply[4] != (0x10+i*1)) {
      Serial.println(F("LinkCommand returned wrong tag number."));
      test_output = -1;
    }
    if(reply[5] != (0x20+i*2)) {
      Serial.println(F("LinkCommand returned wrong tag number."));
      test_output = -1;
    }
    if(reply[6] != (0x30+i*3)) {
      Serial.println(F("LinkCommand returned wrong tag number."));
      test_output = -1;
    }
    if(reply[7] != (0x40+i*4)) {
      Serial.println(F("LinkCommand returned wrong tag number."));
      test_output = -1;
    }
    Serial.print("Event ");
    Serial.print(i);
    Serial.print(" elapsed time is ");
    Serial.print(reply[8]);
    Serial.println(" seconds.");
    if(reply[8] != (num_test_events-i-1)) {
      Serial.println("LinkCommand returned wrong elapsed time.");
      test_output = -1;
    }
  }
  return test_output;
}

