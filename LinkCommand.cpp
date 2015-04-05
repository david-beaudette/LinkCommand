/**
  LinkCommand.cpp
  
  Defines the list of user ID's and associated authorizations.
**/

#include "LinkCommand.h"

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
                                sys_state_t *systemState) { 

  switch(cmd[0]) {
    case 0xA0:
      // Switch to auto mode if enabled or disabled
      if(*systemState < ACTIVATED) {*systemState = IDLE;}
      reply[0]  = 0xAF;
      *reply_len = 0x01;
      return 1;
      
    case 0xA1:
      // Switch to enabled mode
      *systemState = ENABLED;
      reply[0]  = 0xAF;
      *reply_len = 0x01;
      return 1;
      
    case 0xA2:
      // Switch to disabled mode
      *systemState = DISABLED;
      reply[0]  = 0xAF;
      *reply_len = 0x01;
      return 1;
      
    case 0xA3:
      // More elaborate command managed in its own routine
      return this->dumpLogging(reply, reply_len);
      
    case 0xA4:
      // More elaborate command managed in its own routine
      return this->tableUpdate(cmd, reply, reply_len);
           
    case 0xA5:
      // More elaborate command managed in its own routine
      return this->checkMemory(reply, reply_len);
      
    case 0xA6:
      _table->clearTable(); 
      reply[0]  = 0xAF;
      *reply_len = 0x01;
      return 1;
      
    default:
      *reply_len = 0x00;
      return -1;
  }
}


// Process a series of table update commands
int LinkCommand::dumpLogging(byte *reply, byte *reply_len) {
  AccessEvent *event_ptr;
  // This shall send back:
  // [0] the current state of the Arduino
  // [1] the dump logging command code 0xA3
  // [2] number of remaining events, including the event currently sent
  // [3] event type:
  //    0x30 : ‘Attempt’ (first authorization in double authorization mode).
  //    0x31 : ‘Confirm’ (authorized user activated relay).
  //    0x32 : ‘Logout’  (authorized user deactivated relay).
  //    0x33 : ‘Fail’    (unauthorized user card detected).
  //    0x34 : ‘Unknown’ (unknown user card detected).
  // [4-7] user tag for this event
  // [8] number of seconds elapsed since event happened
  *reply_len = 0x09;
  reply[0]  = 0xAF;
  reply[1]  = 0xA3;
  
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

// Process a series of table update commands
int LinkCommand::tableUpdate(byte *cmd, 
                             byte *reply, 
                             byte *reply_len) {
  int update_result;

  // The command contains 7 bytes:
  // [0] the table update command code
  // [1] remaining table entries including this one
  // [2] if user is authorized (1) or not (0)
  // [3-6] user tag id
  
  // First check that the number of table entries is not 0 (error)
  int table_count = cmd[1];
  if(table_count == 0) {
    *reply_len = 0x00;
    return -1;
  }
  // The transmit buffer should contain:
  // [0] the current state of the Arduino
  // [1] the table update command code
  // [2] to be set to the update result
  reply[0] = 0xAF;
  reply[1] = 0xA4;
  *reply_len = 0x03;
  
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

int LinkCommand::checkMemory(byte *reply, byte *reply_len) {
  unsigned int lsb, msb;
  // Initialize transmit buffer with:
  // [0] the current state of the Arduino
  // [1] the check memory command code 0xA5
  // [2] LSB of the total number of users allowed in table
  // [3] MSB of the total number of users allowed in table
  // [4] LSB of the current number of users in table
  // [5] MSB of the current number of users in table
  reply[0] = 0xAF;
  reply[1] = 0xA5;
  *reply_len = 0x06;
  
  // Retrieve table usage
  _table->getNumUsers(&lsb, &msb);
  reply[2] = MAX_USER_SIZE & 0xFF;
  reply[3] = (MAX_USER_SIZE & 0xFF00) << 8;
  reply[4] = lsb & 0xFF;
  reply[5] = msb & 0xFF;

  // 1 byte was processed for this command
  return 1;
}
