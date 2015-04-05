/**
  LinkCommand.cpp
  
  Defines the list of user ID's and associated authorizations.
**/

#include "LinkCommand.h"

LinkCommand::LinkCommand(RF24 *radio, 
                         AccessTable *table, 
                         EventList *event_list) {
  // Assign class pointers                             
  _radio = radio;
  _table = table;
  _event_list = event_list;  

 // Configure as slave in the communication protocol
  _radio->begin();
  _radio->setRetries(retryDelay, retryCount);
  _radio->setPayloadSize(payloadSize);
  _radio->openWritingPipe(pipes[1]);
  _radio->openReadingPipe(1,pipes[0]);
  _radio->setChannel(radioChannel);
  _radio->enableDynamicPayloads();
  _radio->setAutoAck(true);
  _radio->setDataRate(dataRate);
  
  _radio->startListening();
  _radio->printDetails();
  
}

int LinkCommand::processCommand(sys_state_t *systemState) { 
  int replyResult;
  
  // Read the command byte from the receive buffer
  byte rx_buf;
  _radio->read(&rx_buf, 1);
  
  switch(rx_buf) {
    case 0xA0:
      // Switch to auto mode if enabled or disabled
      if(*systemState < ACTIVATED) {*systemState = IDLE;}
      replyResult = this->replyOk();
      break;
      
    case 0xA1:
      // Switch to enabled mode
      *systemState = ENABLED;
      replyResult = this->replyOk();
      break;  
      
    case 0xA2:
      // Switch to disabled mode
      *systemState = DISABLED;
      replyResult = this->replyOk();
      break;  
      
    case 0xA3:
      replyResult = this->dumpLogging();
      break;  
      
    case 0xA4:
      replyResult = this->tableUpdate();
      break;
           
    case 0xA5:
      replyResult = this->checkMemory();
      break;  
      
    case 0xA6:
      _table->clearTable(); 
      replyResult = this->replyOk();
      break; 
  }

  // Resume listening so we catch the next command.
  _radio->startListening();
  return replyResult;
}

// Send standard reply with radio
int LinkCommand::replyOk() {
  // Send back system state (if this function is
  // called it means we're Ok)
  byte tx_buf = 0xAF;
  
  // Flush buffers and switch to TX mode
  _radio->stopListening();
  if(!_radio->write(&tx_buf, 1))   {
    printf("replyOk-Err: Unable to send back data.\n\r");
    return -1;
  }
}

// Process a series of table update commands
int LinkCommand::dumpLogging(void) {
  AccessEvent *event_ptr;
  int retry_iter;
  
  // Initialize the message buffers 
  byte rx_buf[7] = {0,0,0,0,0,0,0};
  
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
  byte tx_buf[9] = {0xAF,0xA3,0,0,0,0,0,0,0};
  
  // Validate list size has at least one logged event
  int list_size = _event_list->getListSize();
  if(list_size <= 0) {
    // Flush buffers and switch to TX mode
    _radio->stopListening();
    
    // No event to report, send message as is
    if(!_radio->write(&tx_buf, 9))   {
      printf("dumpLogging-Err: Unable to send back data.\n\r");
      return -1;
    }
  }
  // Send all logged events (sender should send one dump logging 
  // command per event)
  for(int i = 0; i < list_size; i++) {
    // Flush buffers and switch to TX mode
    _radio->stopListening();
  
    // Retrieve event from list and prepare buffer
    event_ptr = _event_list->getEvent();
    tx_buf[2] = list_size - i;
    tx_buf[3] = event_ptr->type;
    tx_buf[4] = event_ptr->tag[0];
    tx_buf[5] = event_ptr->tag[1];
    tx_buf[6] = event_ptr->tag[2];
    tx_buf[7] = event_ptr->tag[3];
    tx_buf[8] = event_ptr->time;
    
    // Reply to sender
    if(!_radio->write(&tx_buf, 9))   {
      printf("dumpLogging-Err: Unable to send back data.\n\r");
      return -1;
    }
    if(i < (list_size-1)) {
      // Setup and wait for next table entry
      _radio->startListening();
      while(!_radio->available() && 
            retry_iter < LINK_WAIT_NUMRETRY) {
        retry_iter++;
        delay(1);
      }
      if(retry_iter == LINK_WAIT_NUMRETRY) {
        // Communication failed, abort and wait for master
        // to initiate another command
        printf("dumpLogging-Err: failed to get next dump logging, link down?\n\r");
        return -1;
      }
      _radio->read(&rx_buf[0], 1);
      delay(20);
    }
  }
}

// Process a series of table update commands
int LinkCommand::tableUpdate(void) {
  int update_result;
  int retry_iter = 0;
  // Initialize the message buffers 
  byte rx_buf[7] = {0,0,0,0,0,0,0};
  // Initialize transmit buffer with:
  // [0] the current state of the Arduino
  // [1] the table update command code
  // [2] to be set to the update result
  byte tx_buf[3] = {0xAF,0xA4,0};
  
  // Read the whole buffer sent with first command:
  // [0] the table update command code (still in radio FIFO)
  // [1] remaining table entries including this one
  // [2] if user is authorized (1) or not (0)
  // [3-6] user tag id
  _radio->read(&rx_buf[0], 7);
  
  int table_count = rx_buf[1];
  for (int i = 1; i < table_count; i++) {
    // Flush buffers and switch to TX mode
    _radio->stopListening();
    
    // Update user entry in table
    if(_table->getUserAuth(&rx_buf[3]) < 0) {
      // User does not exist, add user
      if(_table->addUser(&rx_buf[3], rx_buf[2]) < 0) {
        // Table full
        tx_buf[2] = TABLE_UPDATE_FULL;
      }
      else {
        // Successfully added
        tx_buf[2] = TABLE_UPDATE_ADDED;
      }
    }
    else {        
      if(_table->setUserAuth(&rx_buf[3], rx_buf[2]) < 1) {
        // Authorization was not modified
        tx_buf[2] = TABLE_UPDATE_NOMOD;
      }
      else {
        // Authorization was modified
        tx_buf[2] = TABLE_UPDATE_MOD;
      }
    }
    // Send result
    if(!_radio->write(&tx_buf[0], 3))   {
      printf("tableUpdate-Err: Unable to send back data.\n\r");
      return -1;
    }
      
    if(i < (table_count-1)) {
      // Setup and wait for next table entry
      _radio->startListening();
      while(!_radio->available() && 
            retry_iter < LINK_WAIT_NUMRETRY) {
        retry_iter++;
        delay(1);
      }
      if(retry_iter == LINK_WAIT_NUMRETRY) {
        // Communication failed, abort and wait for master
        // to initiate another command
        printf("tableUpdate-Err: failed to get next table update, link down?\n\r");
        return -1;
      }
      _radio->read(&rx_buf[0], 7);
      delay(20);
    }
  }
  return 0;
}

int LinkCommand::checkMemory() {
  unsigned int lsb, msb;
  printf("Link_Command-Msg: Received check memory command.\n\r");
  // Initialize transmit buffer with:
  // [0] the current state of the Arduino
  // [1] the check memory command code 0xA5
  // [2] LSB of the total number of users allowed in table
  // [3] MSB of the total number of users allowed in table
  // [4] LSB of the current number of users in table
  // [5] MSB of the current number of users in table
  byte tx_buf[6] = {0xAF,0xA5,0,0,0,0};
  _table->getNumUsers(&lsb, &msb);
  tx_buf[2] = MAX_USER_SIZE & 0xFF;
  tx_buf[3] = (MAX_USER_SIZE & 0xFF00) << 8;
  tx_buf[4] = lsb & 0xFF;
  tx_buf[5] = msb & 0xFF;
  
  _radio->stopListening();
  if(!_radio->write(&tx_buf[0], 6))  {
    printf("checkMemory-Err: Unable to send back data.\n\r");
    return -1;
  }
  return 0;
}
