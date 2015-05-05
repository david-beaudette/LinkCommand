/*
  LinkCommand.h
  
  Manages commands received in the radio.
 */
#ifndef __LINKCOMMAND__
#define __LINKCOMMAND__

#include <EEPROM.h>
#include "Arduino.h"

#include "nano_rfid_hal.h"
#include "AccessEvent.h"
#include "AccessTable.h"

// Table update results
#define TABLE_UPDATE_NOMOD 0xD1
#define TABLE_UPDATE_MOD   0xD2
#define TABLE_UPDATE_ADDED 0xD3
#define TABLE_UPDATE_FULL  0xDF

class LinkCommand {
  public:
    LinkCommand(AccessTable *table, 
                EventList *event_list);
    int processCommand(byte *cmd, 
                       byte *reply, 
                       byte *reply_len, 
                       sys_state_t *system_state,
                       act_mode_t *act_mode);
    
  private:
    int dumpLogging(byte *reply, byte *reply_len);
    int tableUpdate(byte *cmd, 
                    byte *reply, 
                    byte *reply_len);
    int checkMemory(byte *reply, byte *reply_len);
    AccessTable *_table; 
    EventList *_event_list;
};


#endif // __LINKCOMMAND__
