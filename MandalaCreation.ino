#define ERR_no_endex_available_to_remove_by_index 0b00000001
#define ERR_no_endex_available_to_remove_by_endex 0b00000010
#define ERR_check_endex_array 0b00000100

int cfm_modulus;
float cfm_times;
int cfm_optimization;

int string_array_sindex;   //adress: index | endex
int endex_counts;          //adress: endex | num_indexe | adress (string_array_sendex)
int string_array_sendex;   //adress: endex | num_indexe | index0 | index1 | ...
int string_array;          //adress: string_number | index | endex
int error;

/*void setup() 
{
   Serial.begin(9600);
   while(!Serial);
   
   Wire.begin();
   fram.begin();

   int fram_adress = 0;
   int num_strings = 0;
   int success = createMandala(320, 111, 0.28, &fram_adress, &num_strings);

   Serial.print("New Mandala @");
   Serial.print(fram_adress, HEX);
   Serial.print(" with ");
   Serial.print(num_strings);
   Serial.print(" Strings. Error: ");
   Serial.print(success, BIN);
}*/


int createMandala(int new_cfm_modulus, float new_cfm_times, int new_cfm_optimization)
{   
   cfm_modulus = new_cfm_modulus;
   cfm_times = new_cfm_times;
   cfm_optimization = new_cfm_optimization;
   Serial.println(new_cfm_times, 8);
    
   //Calculate memory adresses  
   string_array_sindex = FRAM_ADR_MANDALA_CREATION;
   endex_counts = FRAM_ADR_MANDALA_CREATION + cfm_modulus * 2;
   string_array_sendex = endex_counts + cfm_modulus * 4;
   
   //Clear memory
   clearFRAMMemory(string_array_sindex, cfm_modulus * 2, 0xFF);
   clearFRAMMemory(endex_counts, cfm_modulus * 4, 0x00);
   
   if(DEBUG_MANDALA_FRAM_CREATION) Serial.println(string_array_sindex);
   if(DEBUG_MANDALA_FRAM_CREATION) Serial.println(endex_counts);

   if(DEBUG_MANDALA_FRAM_CREATION) Serial.println("Memory adresses calculated, space cleared.");

   lcdMandalaCreation("Creating Strings");
   //Calculate sorting strings. Check length.
   int n = createStringArray();
   
   if(DEBUG_MANDALA_FRAM_CREATION) Serial.println("String array created.");

   lcdMandalaCreation("Removing Duplicates");
   //Remove duplicate strings
   n -= removeDuplicates(n);

   lcdMandalaCreation("Preparing Arrays");
   //Calculates adresses to array that stores: num_indexes (for string_array_sendex) and the according indexes. Returns length of string_array_sendex array
   int string_array_sendex_length = calculateEndexArrayAdresses(endex_counts + cfm_modulus * 4);
   clearFRAMMemory(string_array_sendex, string_array_sendex_length, 0x00);   
   if(DEBUG_MANDALA_FRAM_CREATION) Serial.println("Endex Array Adresses calculated.");
   
   //For each string in string_array_sindex, add index to string_array_sendex and increment endex_count accordingly.
   fillSEndexArray();
   if(DEBUG_MANDALA_FRAM_CREATION) Serial.println("SEndex Array Filled.");

   //Check for consistency in endex Array, TODO! between endex and index array
   checkEndexArray();
   
   //Calculate FRAM Adresses and clear Bytes
   string_array = string_array_sendex + string_array_sendex_length;   
   clearFRAMMemory(string_array, cfm_modulus * 4, 0xFF);
   
   lcdMandalaCreation("Connecting Strings");
   //Sort and connect strings to weaveable mandala.
   connectStrings(n);   
   if(DEBUG_MANDALA_FRAM_CREATION) Serial.println("Strings connected for weaving.");

   printMandala(n);  

   int overall_length = (int)calculateOverallLength(n) + 1;

   //Write meta information to FRAM
   fram.writeWord(FRAM_ADR_MANDALA_FRAM_ADRESS, string_array);
   fram.writeWord(FRAM_ADR_MANDALA_NUM_STRINGS, n);
   fram.writeWord(FRAM_ADR_MANDALA_OVERALL_LENGTH, overall_length);
   fram.writeWord(FRAM_ADR_MODULO, new_cfm_modulus);
   writeFloat(FRAM_ADR_TIMES, new_cfm_times);
   fram.writeWord(FRAM_ADR_OPTIMIZATION, new_cfm_optimization); //normalized

   //Update variables
   mandala_num_strings = n;
   mandala_fram_adress = string_array;
   mandala_overall_length = overall_length;
   modulo = new_cfm_modulus;
   times = new_cfm_times;
   optimization = new_cfm_optimization;

   if(DEBUG)
   {
      Serial.print("New Mandala in FRAM@");
      Serial.print(string_array);
      Serial.print(" with ");
      Serial.print(n);
      Serial.print(" Strings. ");
      Serial.print(overall_length);
      Serial.print("(m) overall string length. Error: ");
      Serial.println(error, BIN);   
   }         
  
   return error;
}

//Normalized with radius = 0.5
float calculateOverallLength(int num_strings)
{
   float string_length = 0;
   int index, endex, index_next = 0;
   
   for(int i = 0; i < num_strings - 1; i++)
   {
      fram.readWord(string_array + 4 * i, &index);
      fram.readWord(string_array + 4 * i + 2, &endex);
      fram.readWord(string_array + 4 * i + 4, &index_next);
      
      //Length of string i, TO CHECK!!!
      string_length += getStringLength(index, endex, cfm_modulus, true);
      
      //Length of connection between strings i and i+1
      string_length += getStringLength(endex, index_next, cfm_modulus, false);
   }

   //Add length of last string
   fram.readWord(string_array + 4 * num_strings - 1, &index);
   fram.readWord(string_array + 4 * num_strings - 1 + 2, &endex);
   string_length += getStringLength(index, endex, cfm_modulus, true);

   return string_length;
}

void checkEndexArray()
{
   int endex = 0;
   int endex_count = 0;
   int endex_count_array = 0;
   int endex_adr = 0;
   int index = 0;
   
   for(int i = 0; i < cfm_modulus; i++)
   {
      fram.readWord(endex_counts + i * 4, &endex_count);
      fram.readWord(endex_counts + i * 4 + 2, &endex_adr);

      if(endex_count != 0x0000)
      {         
         if(endex_adr != 0x0000)
         {         
            fram.readWord(endex_adr, &endex_count_array);

            if(endex_count_array == endex_count)
            {
               if(DEBUG_MANDALA_FRAM_CREATION) Serial.print("Endex ");
               if(DEBUG_MANDALA_FRAM_CREATION) Serial.print(i);
               if(DEBUG_MANDALA_FRAM_CREATION) Serial.print("\t count:");
               if(DEBUG_MANDALA_FRAM_CREATION) Serial.print(endex_count);
               if(DEBUG_MANDALA_FRAM_CREATION) Serial.print("\t indexe:");
               
               for(int j = 0; j < endex_count; j++)
               {
                  fram.readWord(endex_adr + 2 + j * 2, &index);
                  if(DEBUG_MANDALA_FRAM_CREATION) Serial.print("\t");
                  if(DEBUG_MANDALA_FRAM_CREATION) Serial.print(index);
                  
                  if(index < 0)
                  {
                     if(DEBUG_MANDALA_FRAM_CREATION) Serial.println("index < 0");
                     error |= ERR_check_endex_array;
                  }
               }        
               if(DEBUG_MANDALA_FRAM_CREATION) Serial.println("");       
            }
            else
            {
               if(DEBUG_MANDALA_FRAM_CREATION) Serial.print("Counts dont match: ");
               if(DEBUG_MANDALA_FRAM_CREATION) Serial.print(endex_count);
               if(DEBUG_MANDALA_FRAM_CREATION) Serial.print("\t");
               if(DEBUG_MANDALA_FRAM_CREATION) Serial.println(endex_count_array);
               if(DEBUG_MANDALA_FRAM_CREATION) Serial.print("\t ");
               if(DEBUG_MANDALA_FRAM_CREATION) Serial.println(i);
               error |= ERR_check_endex_array;
            }
            //fram.writeWord(endex_adr + 2 + endex_count * 2, i);
            //endex_count++;
            //fram.writeWord(endex_adr * 2, endex_count);
         }
         else
         {            
            if(DEBUG_MANDALA_FRAM_CREATION) Serial.print("Count, but no Endex adress: ");
            if(DEBUG_MANDALA_FRAM_CREATION) Serial.println(i);
            error |= ERR_check_endex_array;
         }
      }
      else
      {
         if(DEBUG_MANDALA_FRAM_CREATION) Serial.print("No Endex: ");
         if(DEBUG_MANDALA_FRAM_CREATION) Serial.println(i);
      }
   }
   //TODO! Check for consistency between index array and endex array..
}

//Calculates adresses for array that stores: num_indexes (for endexarray) and the according indexes. Returns length of string_array_sendex array
int calculateEndexArrayAdresses(int start_adr)
{
   int count = 0;
   int adress = start_adr;

   fram.writeWord(endex_counts + 2, adress);
   
   for(int i = 0; i < cfm_modulus; i++)
   {
      fram.writeWord(endex_counts + i * 4 + 2, adress);
      fram.readWord(endex_counts + i * 4, &count);
      adress += 2 + count * 2;      
   }

   return adress - start_adr;
}

//For each string in string_array_sindex, add index to string_array_sendex and increment endex_count accordingly.
void fillSEndexArray()
{
   int endex = 0;
   int endex_adr = 0;
   int endex_count = 0;
   
   if(DEBUG_MANDALA_FRAM_CREATION) Serial.println("Fill array now.");

   for(int i = 0; i < cfm_modulus; i++)
   {
      fram.readWord(string_array_sindex + i * 2, &endex);
      
      if(endex != 0xFFFF)
      {         
         //fram.readWord(endex_counts + i * 4, &endex_count);
         fram.readWord(endex_counts + endex * 4 + 2, &endex_adr);
         fram.readWord(endex_adr, &endex_count);
         endex_count++;
         
         if(DEBUG_MANDALA_FRAM_CREATION) Serial.print("Endex count for index: ");
         if(DEBUG_MANDALA_FRAM_CREATION) Serial.print(i);
         if(DEBUG_MANDALA_FRAM_CREATION) Serial.print("\tendex:");
         if(DEBUG_MANDALA_FRAM_CREATION) Serial.print(endex);
         if(DEBUG_MANDALA_FRAM_CREATION) Serial.print("\t");
         if(DEBUG_MANDALA_FRAM_CREATION) Serial.println(endex_count);
         
         fram.writeWord(endex_adr + endex_count * 2, i);
         fram.writeWord(endex_adr, endex_count);
      }
      else
      {
         if(DEBUG_MANDALA_FRAM_CREATION) Serial.println("Endex = 0xFFFF");
      }
   }
   
   if(DEBUG_MANDALA_FRAM_CREATION) Serial.println("Array filled.");   
}

void clearFRAMMemory(uint16_t framAddr, int items, byte value)
{   
   //Send packets with length of 28 byte
   for(int j = 0; j < items/28 + 1; j++)
   {
      fram.I2CAddressAdapt(framAddr + j * 28);

      int n; 
      if(j < items/28)
      {
         n = 28;
      }
      else
      {
         n = items%28;
      }
      for(int i = 0; i < n; i++) 
      {
         Wire.write(value);
      }
      Wire.endTransmission();
   }  
}

boolean setFinalString(int pos, int index, int endex)
{
  fram.writeWord(string_array + pos * 4, index);
  fram.writeWord(string_array + pos * 4 + 2, endex);
}

int getEndexByIndex(int index)
{
  int endex = 0;
  fram.readWord(string_array_sindex + index * 2, &endex);
  
  return endex;
}

//Returns the last index of indexes stored for each endex.
int getIndexByEndex(int endex)
{  
   int endex_count = 0;
   int endex_adr = 0;
   int index = 0xFFFF;  //If none available
  
   //In endex_counts array
   int endex_num = 0;
   fram.readWord(endex_counts + endex * 4, &endex_num);
   
   if(endex_num != 0x0000)
   {      
      fram.readWord(endex_counts + endex * 4 + 2, &endex_adr);
      fram.readWord(endex_adr, &endex_count);
   
      if(endex_count > 0)
      {   
         fram.readWord(endex_adr + endex_count * 2, &index);
      }
   }
  
   return index;
}

      int moved = 0;
      
void removeByIndex(int index)
{  
   int endex = 0;
   int endex_adr = 0;
   int endex_count = 0;
   
   fram.readWord(string_array_sindex + index * 2, &endex);
   fram.readWord(endex_counts + endex * 4 + 2, &endex_adr);   
   fram.readWord(endex_adr, &endex_count);
  
   if(endex_count > 0)
   {   
      //Find index in endex array and remove it
      int current_index = 0;
      for(int i = 0; i < endex_count; i++)
      {
         fram.readWord(endex_adr + 2 + i * 2, &current_index);
         if(current_index == index)
         {
            if(i != endex_count - 1)
            {
               int last_endex = 0;
               fram.readWord(endex_adr + endex_count * 2, &last_endex); 
               fram.writeWord(endex_adr + 2 + i * 2, last_endex); 
               moved++;
               if(DEBUG_MANDALA_FRAM_CREATION) Serial.print(moved);  
               if(DEBUG_MANDALA_FRAM_CREATION) Serial.println(" INDEXes MOVED in endexarray!");              
            }
         }
      } 
      endex_count--;
      fram.writeWord(endex_adr, endex_count); 
      fram.writeWord(endex_counts + endex * 4, endex_count); 

      //Remove endex from index array
      fram.writeWord(string_array_sindex + index * 2, 0xFFFF);
   } 
   else
   {
      if(DEBUG_MANDALA_FRAM_CREATION) Serial.println("no_endex_available_to_remove_by_index");
      error |= ERR_no_endex_available_to_remove_by_index;
   }
   
   fram.writeWord(string_array_sindex + index * 2, 0xFFFF);
  /*fram.writeWord(string_array_sendex + getEndexByIndex(index) * 2, 0xFFFF);  
  fram.writeWord(string_array_sindex + index * 2, 0xFFFF);*/
}

//Removes the last index of indexes stored for each endex, decrement endex_count by 1.
void removeByEndex(int endex)
{
  int endex_count = 0;
  int endex_adr = 0;
  int index = 0;

  fram.readWord(endex_counts + endex * 4 + 2, &endex_adr);
  fram.readWord(endex_adr, &endex_count);

  if(endex_count > 0)
  {   
      fram.readWord(endex_adr + endex_count * 2, &index); 
      fram.writeWord(string_array_sindex + index * 2, 0xFFFF);
      endex_count--;
      fram.writeWord(endex_adr, endex_count); 
      fram.writeWord(endex_counts + endex * 4, endex_count); 
  } 
  else
  {
      if(DEBUG_MANDALA_FRAM_CREATION) Serial.println("no endex available to remove by endex");
      error |= ERR_no_endex_available_to_remove_by_endex;
  }
}

//Normalized with radius = 0.5, Either length of connection between two strings(false) or length of strings(true)
float getStringLength(int index, int endex, int mod, bool of_string)
{
  float alpha;   
  if(index > endex)
  {
    alpha = index - endex;
    if(alpha > mod/2)
    {
      alpha = mod - index + endex;
    }
  }
  else
  {
    alpha = endex - index;
    if(alpha > mod/2)
    {
      alpha = mod - endex + index;
    }
  }
  
  alpha = alpha/mod * PI;

  //If string and not connection
  if(of_string)
  {
      alpha = sin(alpha);
  }
 
  return alpha;
}

//Create sorting string arrays. Remove short strings.
//Return number of sorting strings created.            
int createStringArray()
{
  int n = 0;
  fram.writeWord(string_array_sindex, 0xFFFF);
  fram.writeWord(string_array_sendex, 0xFFFF);  
  
  int index = 0;
  int endex = 0;
  int num_endexes;
  
  for(int i = 1 ; i < cfm_modulus; i++)
  {
    index = i;
    endex = (long)(cfm_times * (long)index) % cfm_modulus;
    
    /*Serial.print(round(cfm_times * (long)index));
    Serial.print("\t");
    Serial.print(cfm_times * (long)index);
    Serial.print("\t");
    Serial.println((long(cfm_times * (long)index)));*/
    
    
    if(getStringLength(index, endex, cfm_modulus, true) > cfm_optimization/1000.0)
    {
      boolean duplicate = false;
      
      if(DEBUG_MANDALA_FRAM_CREATION) Serial.print(index);
      if(DEBUG_MANDALA_FRAM_CREATION) Serial.print("\t");
      if(DEBUG_MANDALA_FRAM_CREATION) Serial.print(endex);
      if(DEBUG_MANDALA_FRAM_CREATION) Serial.print("\t");
      if(DEBUG_MANDALA_FRAM_CREATION) Serial.print(getStringLength(index, endex, cfm_modulus, true));
      
      fram.writeWord(string_array_sindex + index * 2, endex); 
           
      fram.readWord(endex_counts + endex * 4, &num_endexes); 
      num_endexes++;         
      fram.writeWord(endex_counts + endex * 4, num_endexes);
                   
      if(DEBUG_MANDALA_FRAM_CREATION) Serial.print(" Indexes for endex: ");
      if(DEBUG_MANDALA_FRAM_CREATION) Serial.println( num_endexes);   
      n++;     
    }
    else
    {
      if(DEBUG_MANDALA_FRAM_CREATION) Serial.print(index);
      if(DEBUG_MANDALA_FRAM_CREATION) Serial.print("\t");
      if(DEBUG_MANDALA_FRAM_CREATION) Serial.print(endex);
      if(DEBUG_MANDALA_FRAM_CREATION) Serial.print("\t");
      if(DEBUG_MANDALA_FRAM_CREATION) Serial.print(getStringLength(index, endex, cfm_modulus, true));
      if(DEBUG_MANDALA_FRAM_CREATION) Serial.println(" too short");
      
      fram.writeWord(string_array_sindex + index * 2, 0xFFFF);
      //fram.writeWord(string_array_sendex + endex * 2, 0xFFFF);      
    }
  }
  
  return n;
}

int removeDuplicates(int num_strings)
{
   int num_duplicates = 0;
   int endex = 0;
   int endexendex = 0;
   int num_endexes = 0;

   for(int i = 1; i < cfm_modulus; i++)
   {      
      fram.readWord(string_array_sindex + i * 2, &endex);
      
      if(endex != 0xFFFF)
      {         
         fram.readWord(string_array_sindex + endex * 2, &endexendex);
   
         //For each index is checked wether there is a string, that starts with same index as it's endex. If corresponding endex 'endexendex' equals our index it means, a duplicate is identified and removed.
         if(endexendex == i)
         {
            fram.writeWord(string_array_sindex + endex * 2, 0xFFFF);
            num_duplicates++;
            
            fram.readWord(endex_counts + endexendex * 4, &num_endexes);      
            num_endexes--;         
            fram.writeWord(endex_counts + endexendex * 4, num_endexes);    
            if(DEBUG_MANDALA_FRAM_CREATION) Serial.print("Endex remved, new count: "); 
            if(DEBUG_MANDALA_FRAM_CREATION) Serial.println(num_endexes);
         }
      }      
   }

   if(DEBUG_MANDALA_FRAM_CREATION) Serial.print("Duplicates removed: ");
   if(DEBUG_MANDALA_FRAM_CREATION) Serial.println(num_duplicates);
   return num_duplicates;
}

int connectStrings(int number_of_strings)
{
   int n = 0;
   int pos = 0;  
   
   do
   {
      pos++;
   }
   while(getEndexByIndex(pos) == 0xFFFF);
   
   //Set first string in final array
   setFinalString(n, pos, getEndexByIndex(pos));
   int remove_pos = pos;
   pos = getEndexByIndex(pos);
   removeByIndex(remove_pos);
   n++;
   boolean done = false;
   boolean found_next = false;
   boolean dir = true;
   int step_size = 1;
   int pos_next = 0;  
   
   while(n < number_of_strings)  //Or number + 1??? Naah..
   {       
      while(!found_next)
      {
         ///CAREERERER
         if(dir)
         {
            pos_next = (pos + step_size) % cfm_modulus;
         }
         else
         {
            pos_next = pos - step_size;
            if(pos_next < 0) pos_next = cfm_modulus - step_size;
            
            step_size++;
         }
         dir = !dir;
         
         if(getEndexByIndex(pos_next) != 0xFFFF && pos_next != pos)
         {
            /*Serial.println("index");
            Serial.print(step_size);
            Serial.print("\t");
            Serial.print(pos);
            Serial.print("\t");
            Serial.println(pos_next);*/
            setFinalString(n, pos_next, getEndexByIndex(pos_next));
            found_next = true;
            pos = getEndexByIndex(pos_next);
            removeByIndex(pos_next);
         }
         else if(getIndexByEndex(pos_next) != 0xFFFF && pos_next != pos)
         {        
            
            /*Serial.println("endex");
            Serial.print(step_size);
            Serial.print("\t");
            Serial.print(pos);
            Serial.print("\t");
            Serial.println(pos_next);*/
            int index = getIndexByEndex(pos_next);
            setFinalString(n, pos_next, index);
            found_next = true;
            pos = index;
            removeByEndex(pos_next);
            if(DEBUG_MANDALA_FRAM_CREATION) Serial.print("Used Endex: ");
            if(DEBUG_MANDALA_FRAM_CREATION) Serial.println(index);
         }
      }

    dir = true;
    found_next = false;
    step_size = 1;
    n++;     
   }
}

void printMandala(int n)
{
   int index;
   int endex;
   
   if(DEBUG_MANDALA_FRAM_CREATION) Serial.print("Mandala cfm_modulus ");
   if(DEBUG_MANDALA_FRAM_CREATION) Serial.print(cfm_modulus);
   if(DEBUG_MANDALA_FRAM_CREATION) Serial.print("\tcfm_times ");
   if(DEBUG_MANDALA_FRAM_CREATION) Serial.print(cfm_times);
   if(DEBUG_MANDALA_FRAM_CREATION) Serial.print("\tn ");
   if(DEBUG_MANDALA_FRAM_CREATION) Serial.println(n);
   
   for(int i = 0; i < n; i++)
   {
      fram.readWord(string_array + i * 4, &index);
      fram.readWord(string_array + i * 4 + 2, &endex);
      if(DEBUG_MANDALA_FRAM_CREATION) Serial.print(index);
      if(DEBUG_MANDALA_FRAM_CREATION) Serial.print("\t");
      if(DEBUG_MANDALA_FRAM_CREATION) Serial.println(endex);
   }
   /*Serial.println("########################### SIndex | SEndex #########################");
   for(int i = 0; i < cfm_modulus; i++)
   {
      fram.readWord(string_array_sindex + i * 2, &index);
      fram.readWord(string_array_sendex + i * 2, &endex);
      Serial.print(i);
      Serial.print(":\t");
      Serial.print(index);
      Serial.print("\t");
      Serial.println(endex);
   }*/
}
