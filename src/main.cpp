/* LORA CONTROLLER - DATA REQUEST */

/******* SUMMARY *****
  *PIN-OUT*
    https://resource.heltec.cn/download/Wireless_Stick_Lite_V3/HTIT-WSL_V3.png
    Inbuilt LoRa device, no external connections
*/

/******* INCLUDES ***********/
  #include <Arduino.h>
  #include "lora_config.h"
  #include <RadioLib.h>
  
/******* FUNCTION PROTOTYPES ***********/
  void setup();
  void initLoRa();

  void loop();

  void loraRx();

  void compileReport();
  void loraTx();

/******* GLOBAL VARS ***********/
  #define REPORTING_MODE 2 //0 = prod mode, 1 = include comments, 2 = dev (resests all, eg wipes EEPROM records etc)
  #define SERIAL_BAUD_RATE 115200

  #define LoRa_BUFFER 12 // bytes tx/rx each LoRa transmission
  uint8_t loraData[LoRa_BUFFER]; // array of all tx/rx LoRa data

  // Flag for current LoRa cycle - loops around from MIN_CURRENT_CYCLE to 255 
  // Prevents relays from doubling up on requests etc. This step is not neccesarry for simple systems (no relays, no cross transmissions), but has low overhead
  uint8_t currentCycle_g = 99;

  uint8_t rssi_g= 0; // Recived Signal Strength Indication (how strong is the LoRa signal being recieved)

  uint8_t PeripheralID_g;

/******* INSTANTIATED CLASS OBJECTS ***********/
  SX1262 radio = new Module(LoRa_NSS, LoRa_DIO1, LoRa_NRST, LoRa_BUSY); // see lora_config.h

/******* INIT - SETUP ***********/
  void setup(){
    #if REPORTING_MODE > 0
        Serial.begin(SERIAL_BAUD_RATE);
        Serial.println("Initializing ... ");
        delay(500);
    #endif 

    initLoRa();
    delay(500);

    #if REPORTING_MODE > 0
      Serial.println("Setup Complete.");
    #endif
//status = IDLE;
  }
  void initLoRa(){
    #if REPORTING_MODE > 0
      Serial.print(F("[SX1262] Initializing ... "));
    #endif
    int state = radio.begin(LoRa_CARRIER_FREQUENCY,
                            LoRa_BANDWIDTH,
                            LoRa_SPREADING_FACTOR,
                            LoRa_CODING_RATE,
                            LoRa_SYNC_WORD,
                            LoRa_OUTPUT_POWER,
                            LoRa_PREAMBLE_LENGTH );

    if (state == RADIOLIB_ERR_NONE) {
      #if REPORTING_MODE > 0
        Serial.println(F("LoRa Init success!"));
      #endif
    } else {
      #if REPORTING_MODE > 0
        Serial.print(F("LoRa Init failed, code "));
        Serial.println(state);
      #endif
      while (true);
    }
  }
 
/******* LOOP ***********/
  void loop() {
      loraRx();
  }
 
  void loraRx(){
    //loraData[x]:  cycle[0] | message-type[1] | peripheral_id[2] |

    int SX1262state = radio.receive(loraData, LoRa_BUFFER);

    if (SX1262state == RADIOLIB_ERR_NONE) {// packet was successfully received
      #if REPORTING_MODE > 0 
          Serial.print("PROCESSING Rx: ");
          for(byte i=0;i<LoRa_BUFFER;i++){
            Serial.print(int(loraData[i]));
            Serial.print(",");
          }
          Serial.println(" ");
      #endif

      rssi_g = radio.getRSSI();

      switch(loraData[1])  {
        case REQUESTTYPE_REPORT:{
          compileReport();
        }
        break;
        default:
              #if REPORTING_MODE > 0
                Serial.println("UNKOWN REQUEST TYPE : "+String(loraData[1]));
              #endif 
        break;
      }

    }
    #if REPORTING_MODE > 0
    else{  
      if (SX1262state == RADIOLIB_ERR_RX_TIMEOUT) {// timeout occurred while waiting for a packet
      //     Serial.println(F("timeout!"));
      } 
      else if (SX1262state == RADIOLIB_ERR_CRC_MISMATCH) {// packet was received, but is malformed
        Serial.println(F("CRC error!"));
      } 
      else {// some other error occurred
          Serial.print(F("failed, code "));
          Serial.println(SX1262state);
      }
    }
    #endif
  }

  void compileReport(){
        //REPORT:        cycle[0] | report-type[1] | peripheral_id[2] | rssi recieved from controller[3] | Battery (Raw 1024) [4,5] | sensor data[6...LoRa_BUFFER]

    loraData[0]=currentCycle_g ++;
    loraData[1]=RETURNTYPE_REPORT; // see LORA MESSAGE TYPE in config.h
    loraData[2]=PeripheralID_g;
    loraData[3]=abs(rssi_g); //Last RSSI reading (from Controller to Peripheral)
    loraData[4]=1; 
    loraData[5]=2;

    //Replace with function call to sensor-type-specific module
    loraData[6]=6; // Report Length: length includes this marker, so 1 if rest of report is blank.
    loraData[7]=77;
    loraData[8]=88;
    loraData[9]=99;
    loraData[10]=0;
    loraData[11]=111;

    loraTx();
  }

  void loraTx(){

    // wait for clear airways before attempting tx
    while(radio.getRSSI(false) > RSSI_INIT_TX_THRESHOLD){
      #if REPORTING_MODE > 0
        Serial.println("RSSI: "+ String(radio.getRSSI(false)));
      #endif
      delay(100);
    }

    int SX1262state = radio.transmit(loraData, LoRa_BUFFER);

    #if REPORTING_MODE > 0
      if (SX1262state == RADIOLIB_ERR_NONE) {// the packet was successfully transmitted
        Serial.println("TX success. Datarate: "+ String(radio.getDataRate()) + "bps");
      } else if (SX1262state == RADIOLIB_ERR_PACKET_TOO_LONG) {// the supplied packet was longer than 256 bytes
        Serial.println(F("too long!"));
      } else if (SX1262state == RADIOLIB_ERR_TX_TIMEOUT) {// timeout occured while transmitting packet
        Serial.println(F("timeout!"));
      } else {// some other error occurred
        Serial.println("TX Fail code: "+String(SX1262state));
      }
      Serial.print("RECIEVING: ");
    #endif 
  }
 
/*END*/
