/*
void nfcISR()
{
   nfc_interrupt = true;
}

bool initNFC()
{
   #ifdef DEBUG
   Serial.println("-----------------NFC Init-------------------");
   #endif
   
   nfc.begin();   
   attachInterrupt(digitalPinToInterrupt(NFC_INTERRUPT), nfcISR, FALLING);
   
   uint32_t versiondata = nfc.getFirmwareVersion();
         
   #ifdef DEBUG
   Serial.print("Found chip PN5");
   Serial.println((versiondata >> 24) & 0xFF, HEX);
   Serial.print("Firmware ver. ");
   Serial.print((versiondata >> 16) & 0xFF, DEC);
   Serial.print('.');
   Serial.println((versiondata >> 8) & 0xFF, DEC);
   #endif   

   nfc.SAMConfig();
   nfc.begin();

   return versiondata;
}

void handleNFC()
{
   //Pull every 400ms if enabled
   if(nfc_pull && nfc_timer + 400 < millis())
   {
      //Serial.println("pull");
      nfc_timer = millis();
      nfc_buffer[0] = 0x4A;
      nfc_buffer[1] = 1;
      nfc_buffer[2] = 0;
      nfc.sendCommandCheckAck(nfc_buffer, 3, 10);
   }
   
   if(nfc_interrupt)
   {            
      nfc_response_length = 32;   
      nfc_interrupt = false;
      nfc.readdata(nfc_buffer, sizeof(nfc_buffer));

      if (nfc_buffer[0] == 0 && nfc_buffer[1] == 0 && nfc_buffer[2] == 0xff) 
      {
         uint8_t length = nfc_buffer[3];
         
         if (nfc_buffer[4] == (uint8_t)(~length + 1)) 
         {
            //Serial.println("Tag inlisted");
            nfc._inListedTag = nfc_buffer[8];
            
            uint8_t selectApdu[] = { 0x00, // CLA 
                             0xA4, // INS 
                             0x04, // P1  
                             0x00, // P2  
                             0x07, // Length of AID  
                             0xF0, 0x00, 0x00, 0x00, 0x0A, 0x01, 0x01, // AID defined on Android App 
                             0x00  // Le
                             };                             
   
            boolean success = nfc.inDataExchange(selectApdu, sizeof(selectApdu), nfc_response, &nfc_response_length);            
            
            if(success) 
            {  
               #ifdef DEBUG 
               Serial.print("APDU length "); + Serial.print(nfc_response_length); 
               Serial.print(": [");
              
               for(int i = 0; i < nfc_response_length; i++)
               {
                  Serial.print(nfc_response[i]) + Serial.print("\t");
               }  
               Serial.println("]");
               #endif
               
               switch(nfc_response[0])
               {
                  case NFC_SEND_STATUS:
                           nfc_buffer[0] = NFC_SEND_STATUS;
                           nfc_buffer[1] = state;  //Machine state
                           nfc_buffer[2] = intMSB(modulo);  //Modulo MSB
                           nfc_buffer[3] = intLSB(modulo);  //Modulo LSB
                           nfc_buffer[4] = int32(times, 0);  //Times 0
                           nfc_buffer[5] = int32(times, 1);  //Times 1
                           nfc_buffer[6] = int32(times, 2);  //Times 2
                           nfc_buffer[7] = int32(times, 3);  //Times 3
                           nfc_buffer[8] = intMSB(current_string);  //Current string MSB
                           nfc_buffer[9] = intLSB(current_string);  //Current string LSB
                           nfc_buffer[10] = steps_tooth;  //Steps per tooth
                           //nfc_buffer[11] = intMSB(weaving_duration);  //Weaving duration MSB 
                           //nfc_buffer[12] = intLSB(weaving_duration);  //Weaving duration LSB
                                          
                           nfc.inDataExchange(nfc_buffer, 13, nfc_response, &nfc_response_length);
                           
                           #ifdef DEBUG 
                           Serial.println("Status sent."); 
                           #endif
                         
                           break;
                           
                  case NFC_RECIEVE_MANDALA:
                           recieveMandalaViaNFC();                           
                           break;
                  
                  case NFC_DELTA_PHOTO:
                           steps_photolaser = nfc_response[1];
                           fram.writeByte(FRAM_ADR_STEPS_PHOTOLASER, steps_photolaser);
                           nfc_buffer[0] = NFC_SUCCESS;
                           nfc_buffer[1] = NFC_DELTA_PHOTO;
                           
                           nfc.inDataExchange(nfc_buffer, 2, nfc_response, &nfc_response_length);
                           break;
                  case NFC_CURRENT_STRING:
                           current_string = bytebyteInt(nfc_response[1], nfc_response[2]);
                           Serial.print("current string ");
                           Serial.println(current_string);
                           nfc_buffer[0] = NFC_SUCCESS;
                           nfc_buffer[1] = NFC_CURRENT_STRING;
                           
                           nfc.inDataExchange(nfc_buffer, 2, nfc_response, &nfc_response_length);
                           break;
               }               
            }
         }
      }
   }
}

void recieveMandalaViaNFC()
{   
   boolean success = false;
   boolean transfer_complete = false;
   int expected_string = 0;
   
   mandala_data_length = bytebyteInt(nfc_response[1], nfc_response[2]);
   modulo = bytebyteInt(nfc_response[3], nfc_response[4]);
   times = bbbbInt(nfc_response[5], nfc_response[6], nfc_response[7], nfc_response[8]);

   fram.writeWord(FRAM_ADR_MANDALA_DATA_LENGTH, mandala_data_length);
   fram.writeWord(FRAM_ADR_MODULO, modulo);
   fram.writeLong(FRAM_ADR_TIMES, times);

   #ifdef DEBUG
   Serial.println("---------Recieving Mandala--------");
   Serial.println(mandala_data_length);
   Serial.println(modulo);
   Serial.println(times);
   #endif
   
   nfc_buffer[0] = NFC_RECIEVE_MANDALA;
   nfc_buffer[1] = 0;
   nfc_buffer[2] = 0;   
   
   nfc.inDataExchange(nfc_buffer, 3, nfc_response, &nfc_response_length);    
   
   while(!transfer_complete)
   {        
      fram.writeByte(mandala_fram_adress + expected_string * 4, nfc_response[3]); //28 length
      fram.writeByte(mandala_fram_adress + expected_string * 4 + 1, nfc_response[4]); //28 length
      fram.writeByte(mandala_fram_adress + expected_string * 4 + 2, nfc_response[5]); //28 length
      fram.writeByte(mandala_fram_adress + expected_string * 4 + 3, nfc_response[6]); //28 length
      
      Serial.print(expected_string);
      Serial.print(" | ");

      for(int i = 3; i < 7; i++)   //<28
      { 
         Serial.print(nfc_response[i]);
         Serial.print("\t");
      }
      Serial.println("");
      
      expected_string += 1; //7
   
      if(expected_string * 4 >= mandala_data_length)
      {
         Serial.println("all strings there");
         transfer_complete = true;   
         printMandala();
      } 
      else
      {
         nfc_buffer[0] = NFC_RECIEVE_MANDALA;
         nfc_buffer[1] = intMSB(expected_string);
         nfc_buffer[2] = intLSB(expected_string);
         
         success = nfc.inDataExchange(nfc_buffer, 3, nfc_response, &nfc_response_length); 
         Serial.println(nfc_response_length);
                
         if(!success)
         {
            fram.writeByte(FRAM_ADR_MANDALA_DATA_VALID, 0x00);
            Serial.println("Mandala transfer interrupted!"); 
            transfer_complete = true;
         }
      }                       
   }

   if(transfer_complete && success)
   {
      nfc_buffer[0] = NFC_SUCCESS;
      nfc_buffer[1] = NFC_RECIEVE_MANDALA;
      nfc.inDataExchange(nfc_buffer, 2, nfc_response, &nfc_response_length);  
      
      fram.writeByte(FRAM_ADR_MANDALA_DATA_VALID, 0x01);  
      Serial.println("Success, transfer complete!");     
   }   
}*/
