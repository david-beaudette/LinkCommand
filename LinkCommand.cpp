/**
  LinkCommand.cpp
  
  Defines the list of user ID's and associated authorizations.
**/

#include "LinkCommand.h"

/** Set internal pointers to access table and event list. **/
LinkCommand::LinkCommand(AccessTable *table, 
                         EventList *event_list) {
  // Assign class pointers                             
  _table = table;
  _event_list = event_list;  
}

/**
  Processes a single received command and prepares the data 
  that should be sent back. The returned
  value from this function allows the caller to check if
  all received bytes were processed. If not, the buffer probably
  contained more than one command.
  
  @return >0: the number of bytes processed\n
          -1: an error has occured while processing
**/
int LinkCommand::processCommand(byte *cmd, 
                                byte *reply, 
                                byte *reply_len, 
                                sys_state_t *system_state,
                                act_mode_t  *act_mode) { 

  switch(cmd[0]) {
    case CMD_CHECK:
      // Just acknowledge with OK
      reply[0]  = REPLY_OK;
      *reply_len = 1;
      return 1;
      
    case CMD_DOUBLEACT:
      // Switch to double activation mode
      *act_mode = DOUBLE;
      reply[0]  = REPLY_OK;
      *reply_len = 1;
      return 1;
      
    case CMD_SIMPLEACT:
      // Switch to double activation mode
      *act_mode = SINGLE;
      reply[0]  = REPLY_OK;
      *reply_len = 1;
      return 1;
      
    case CMD_AUTO:
      // Switch to auto mode if enabled or disabled
      if(*system_state < ACTIVATED) {*system_state = IDLE;}
      reply[0]  = REPLY_OK;
      *reply_len = 1;
      return 1;
      
    case CMD_ENABLE:
      // Switch to enabled mode
      *system_state = ENABLED;
      reply[0]  = REPLY_OK;
      *reply_len = 1;
      return 1;
      
    case CMD_DISABLE:
      // Switch to disabled mode
      *system_state = DISABLED;
      reply[0]  = REPLY_OK;
      *reply_len = 1;
      return 1;
      
    case CMD_DUMPLOGGING:
      // More elaborate command managed in its own routine
      return this->dumpLogging(reply, reply_len);
      
    case CMD_UPDATETABLE:
      // More elaborate command managed in its own routine
      return this->tableUpdate(cmd, reply, reply_len);
           
    case CMD_MEMORYCHECK:
      // More elaborate command managed in its own routine
      return this->checkMemory(reply, reply_len);
      
    case CMD_MEMORYCLEAR:
      _table->clearTable(); 
      reply[0]  = REPLY_OK;
      *reply_len = 1;
      return 1;
      
    default:
      *reply_len = 0;
      return -1;
  }
}

/** Process a single dump logging command. **/
int LinkCommand::dumpLogging(byte *reply, byte *reply_len) {
  AccessEvent *event_ptr;
  // This shall send back:
  // [0] the current state of the Arduino
  // [1] the dump logging command code
  // [2] number of remaining events, including the event currently sent
  // [3] event type:
  //    0x30 : ‘Attempt’ (first authorization in double authorization mode).
  //    0x31 : ‘Confirm’ (authorized user activated relay).
  //    0x32 : ‘Logout’  (authorized user deactivated relay).
  //    0x33 : ‘Fail’    (unauthorized user card detected).
  //    0x34 : ‘Unknown’ (unknown user card detected).
  // [4-7] user tag for this event
  // [8] number of seconds elapsed since event happened
  *reply_len = 9;
  reply[0]  = REPLY_OK;
  reply[1]  = CMD_DUMPLOGGING;
  
  // Validate list size has at least one logged event
  int list_size = _event_list->getListSize();
  if(list_size <= 0) {
    // No event to report
    reply[2] = 0;
    return 1;
  }
  // Retrieve event from list and prepare buffer
  event_ptr = _event_list->getEvent();
  reply[2] = list_size;
  reply[3] = event_ptr->type;
  reply[4] = event_ptr->tag[0];
  reply[5] = event_ptr->tag[1];
  reply[6] = event_ptr->tag[2];
  reply[7] = event_ptr->tag[3];
  reply[8] = event_ptr->time;
  
  // 1 byte was processed for this command
  return 1;
}

/** Process a single table update command. **/
int LinkCommand::tableUpdate(byte *cmd, 
                             byte *reply, 
                             byte *reply_len) {
  int update_result;

  // The command contains 7 bytes:
  // [0] the table update command code
  // [1] remaining table entries including this one
  // [2] if user is authorized (1) or not (0)
  // [3-6] user tag id
    Serial.print(F("Tag to be updated: "));
    for(int j = 0; j < 4; j++) {
      Serial.print(cmd[3+j], HEX);
      if(j < 3) {
        Serial.print(", ");
      }
    }
    Serial.println(" ");
  
  // First check that the number of table entries is not 0 (error)
  int table_count = cmd[1];
  if(table_count == 0) {
    *reply_len = 0;
    return -1;
  }
  // The transmit buffer should contain:
  // [0] the current state of the Arduino
  // [1] the table update command code
  // [2] to be set to the update result
  reply[0] = REPLY_OK;
  reply[1] = CMD_UPDATETABLE;
  *reply_len = 3;
  
  // Update user entry in table
  if(_table->getUserAuth(&cmd[3]) < 0) {
    // User does not exist, add user
    if(_table->addUser(&cmd[3], cmd[2]) < 0) {
      // Table full
      reply[2] = TABLE_UPDATE_FULL;
    }
    else {
      // Successfully added
      reply[2] = TABLE_UPDATE_ADDED;
    }
  }
  else {        
    if(_table->setUserAuth(&cmd[3], cmd[2]) < 1) {
      // Authorization was not modified
      reply[2] = TABLE_UPDATE_NOMOD;
    }
    else {
      // Authorization was modified
      reply[2] = TABLE_UPDATE_MOD;
    }
  }
  // 7 bytes were processed for this command
  return 7;
}

/** Process a single check memory command. **/
int LinkCommand::checkMemory(byte *reply, byte *reply_len) {
  unsigned int lsb, msb;
  // Initialize transmit buffer with:
  // [0] the current state of the Arduino
  // [1] the check memory command code 0xA5
  // [2] LSB of the total number of users allowed in table
  // [3] MSB of the total number of users allowed in table
  // [4] LSB of the current number of users in table
  // [5] MSB of the current number of users in table
  reply[0] = REPLY_OK;
  reply[1] = CMD_MEMORYCHECK;
  *reply_len = 6;
  
  // Retrieve table usage
  _table->getNumUsers(&lsb, &msb);
  reply[2] = (MAX_USER_SIZE & 0xFF00) << 8;
  reply[3] = MAX_USER_SIZE & 0xFF;
  reply[4] = msb & 0xFF;
  reply[5] = lsb & 0xFF;
  /*Serial.print(F("Memory size: MSB "));
  Serial.print(reply[2]);
  Serial.print(F(", LSB "));
  Serial.println(reply[3]);
  Serial.print(F("Memory used: MSB "));
  Serial.print(reply[4]);
  Serial.print(F(", LSB "));
  Serial.println(reply[5]);*/
  // 1 byte was processed for this command
  return 1;
}
