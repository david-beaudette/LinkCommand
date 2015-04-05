/*
  LinkCommand.h
  
  Manages commands received in the radio.
 */
#ifndef __LINKCOMMAND__
#define __LINKCOMMAND__

#include <EEPROM.h>
#include "Arduino.h"

#include "RF24.h"
#include "nano_rfid_hal.h"
#include "AccessEvent.h"
#include "AccessTable.h"

// Table update results
#define TABLE_UPDATE_NOMOD 0xD1
#define TABLE_UPDATE_MOD   0xD2
#define TABLE_UPDATE_ADDED 0xD3
#define TABLE_UPDATE_FULL  0xDF

// Communication link parameters
#define LINK_WAIT_NUMRETRY 1024*1024

class LinkCommand {
  public:
    LinkCommand(RF24 *radio, 
                AccessTable *table, 
                EventList *event_list);
    int processCommand(sys_state_t *systemState);
    
  private:
    int replyOk(void);
    int dumpLogging(void);
    int tableUpdate(void);
    int checkMemory(void);
    RF24 *_radio; 
    AccessTable *_table; 
    EventList *_event_list;
};


#endif // __LINKCOMMAND__
