/*
 * 
 *    readWord() return Values from arduino/twi.c
   uint8_t  0 .. success
 *          1 .. length to long for buffer
 *          2 .. address send, NACK received
 *          3 .. data send, NACK received
 *          4 .. other twi error (lost bus arbitration, bus error, ..)
 * 
 * 
 */
void loadValuesFromFRAM()
{   
   fram.readByte(FRAM_ADR_MANDALA_DATA_VALID, &mandala_data_valid);
   fram.readWord(FRAM_ADR_MANDALA_DATA_LENGTH, &mandala_data_length);
   fram.readWord(FRAM_ADR_MODULO, &modulo);
   times = readFloat(FRAM_ADR_TIMES);
   fram.readByte(FRAM_ADR_STEPS_PHOTOLASER, &steps_photolaser);
   fram.readByte(FRAM_ADR_STEPS_HOLE, &steps_hole);
   fram.readByte(FRAM_ADR_STEPS_TOOTH, &steps_tooth);
   fram.readWord(FRAM_ADR_STEPS_LASERWEAVER, &steps_laser_weaver);   
   fram.readByte(FRAM_ADR_STEPS_HOLE_FULL, &steps_hole_full); 
   fram.readByte(FRAM_ADR_STEPS_TOOTH_FULL, &steps_tooth_full);
   fram.readWord(FRAM_ADR_MANDALA_FRAM_ADRESS, &mandala_fram_adress);  
   fram.readWord(FRAM_ADR_MANDALA_NUM_STRINGS, &mandala_num_strings);  
   fram.readWord(FRAM_ADR_MANDALA_OVERALL_LENGTH, &mandala_overall_length); 
   fram.readWord(FRAM_ADR_AVAILABLE_STRING, &available_string); 
   fram.readWord(FRAM_ADR_FRAME_DIA, &frame_diameter);
   fram.readWord(FRAM_ADR_SERVO_HIGH, &servo_pulse_high); 
   fram.readWord(FRAM_ADR_SERVO_LOW, &servo_pulse_low); 
   fram.readWord(FRAM_ADR_CURRENT_STRING, &current_string); 
   fram.readWord(FRAM_ADR_OPTIMIZATION, &optimization); 
   fram.readWord(FRAM_ADR_PAUSE_STRING, &pause_string); 

   if (DEBUG)
   {
   //Serial.print("data valid ");
   //Serial.print(mandala_data_valid);
   //Serial.print("\t data length ");
   //Serial.print(mandala_data_length);
   Serial.print("\t modulo ");
   Serial.println(modulo);
   Serial.print("\t times ");
   Serial.println(times);
   Serial.print("\t steps_hole ");
   Serial.println(steps_hole);
   Serial.print("\t steps_tooth ");
   Serial.println(steps_tooth);
   Serial.print("\t steps_hole_full ");
   Serial.println(steps_hole_full);
   Serial.print("\t steps_tooth_full ");
   Serial.println(steps_tooth_full);
   Serial.print("\t steps_laser_weaver ");
   Serial.println(steps_laser_weaver);
   Serial.print("\t mandala_fram_adress ");
   Serial.println(mandala_fram_adress);
   Serial.print("\t mandala_num_strings ");
   Serial.println(mandala_num_strings);
   Serial.print("\t mandala_overall_length ");
   Serial.println(mandala_overall_length);
   Serial.print("\t available_string ");
   Serial.println(available_string);
   Serial.print("\t servo_pulse_high ");
   Serial.println(servo_pulse_high);
   Serial.print("\t servo_pulse_low ");
   Serial.println(servo_pulse_low);
   Serial.print("\t current_string ");
   Serial.println(current_string);
   Serial.print("\t optimization ");
   Serial.println(optimization);
   Serial.print("\t pause_string ");
   Serial.println(pause_string);
   }
}

void readWord(int adress, int* value)
{
   /*unsigned char msb, lsb;
   
   uint8_t result1 = fram.readByte(adress, &msb);
   uint8_t result2 = fram.readByte(adress + 1, &lsb);

   if(result1 || result2)
   {
      Serial.print("!!!WARNING, FRAM READING ERROR!!! Errorcode:");
      Serial.print(result1);
      Serial.print("/");
      Serial.println(result2);      
   }
   
   *value = bytebyteInt(msb, lsb);*/

   uint8_t result = fram.readWord(adress, value);

   if(result)
   {
      Serial.print("!!!WARNING, FRAM READING ERROR!!! Errorcode:"); 
      Serial.print(result);        
      Serial.print(" @"); 
      Serial.println(adress);        
   }   
}

float readFloat(int adress)
{   
   byte *phelp = new byte[4];
   fram.readArray(adress, 4, phelp);

   return *(float*)phelp;
}

void writeFloat(int adress, float val)
{   
   byte *phelp = new byte[4];
   phelp = (byte*) &val;
   fram.writeArray(adress, 4, phelp);
}

/* 
 * Introduced because of potenital FRAM reading errors.
 * To be used while weaving, to make sure proper values are read. 
 */
void readWordOnlyPositive(int adress, int* value)
{   
   uint8_t result = fram.readWord(adress, value);
   
   while(*value < 0 || result)
   {         
      result = fram.readWord(adress, value);
               
      Serial.print("!!!WARNING, FRAM READING ERROR!!! Read:");
      Serial.print(*value);
      Serial.print(" @");
      Serial.print(adress);
      Serial.print(" Errorcode: ");
      Serial.println(result);
   }  
   
   /* Changed msb/lsb since creating mandala onboard instead of smartphone&nfc.!
      
     
   unsigned char msb, lsb;
   
   uint8_t result1 = fram.readByte(adress, &msb);
   uint8_t result2 = fram.readByte(adress + 1, &lsb);
   
   *value = bytebyteInt(msb, lsb);
   
   if(*value < 0 || result1 || result2)
   {
      Serial.print("!!!WARNING, FRAM READING ERROR!!! Read:");
      Serial.print(*value);
      Serial.print(" @");
      Serial.print(adress);
      Serial.print(" Errorcode: ");
      Serial.print(result1);
      Serial.print("/");
      Serial.println(result2); 
           
      while(*value < 0 || result1 || result2)
      {
         result1 = fram.readByte(adress, &msb);
         result2 = fram.readByte(adress + 1, &lsb);
         *value = bytebyteInt(msb, lsb);
                  
         Serial.print("!!!WARNING, FRAM READING ERROR!!! Read:");
         Serial.print(*value);
         Serial.print(" @");
         Serial.print(adress);
         Serial.print(" Errorcode: ");
         Serial.print(result1);
         Serial.print("/");
         Serial.println(result2); 
      }
      
   }*/
}

void printMandala()
{
   unsigned int index = 0;
   unsigned int endex = 0;
   
   for(int i = 0; i < mandala_num_strings; i++)
   {
      readWord(mandala_fram_adress + (i * 4), &index);
      readWord(mandala_fram_adress + (i * 4 + 2), &endex);
      Serial.print(i);
      Serial.print(":\t");
      Serial.print(index);
      Serial.print("\t");
      Serial.println(endex);      
   }
}


void readMandalaFRAMTest(long millis_loop)
{
   unsigned int index = 0;
   unsigned int endex = 0;
   
   for(int i = 0; i < mandala_num_strings; i++)
   {
      readWord(mandala_fram_adress + (i * 4), &index);
      readWord(mandala_fram_adress + (i * 4 + 2), &endex);
      
      if(index < 0)
      {
        Serial.print(i); 
        Serial.print("(index): "); 
        Serial.print(index); 
        Serial.print(" millis: "); 
        Serial.println(millis_loop); 
      }
      if(endex < 0)
      {
        Serial.print(i); 
        Serial.print("(endex): "); 
        Serial.print(endex); 
        Serial.print(" millis: "); 
        Serial.println(millis_loop); 
      }
   }
}
