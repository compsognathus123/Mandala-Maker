//Weaves Mandala, stored in FRAM @mandala_fram_adress
void weaveMandala()
{
   //Only start with laser on tooth
   if(!photo_tooth)
   {
       if(DEBUG) Serial.println("Position laser on tooth."); 
       lcd.clear();
       lcd.setCursor(0,1);
       lcd.print("Position laser on");
       lcd.setCursor(0,2);
       lcd.print("tooth to start.");

       delay(2000);
       return; 
   }
   
   if(DEBUG) Serial.println("Starting to weave.");
   if(DEBUG) Serial.print("\t steps_hole ");
   if(DEBUG) Serial.println(steps_hole);
   if(DEBUG) Serial.print("\t steps_tooth ");
   if(DEBUG) Serial.println(steps_tooth);
   if(DEBUG) Serial.print("\t steps_hole_full ");
   if(DEBUG) Serial.println(steps_hole_full);
   if(DEBUG) Serial.print("\t steps_tooth_full ");
   if(DEBUG) Serial.println(steps_tooth_full);
   if(DEBUG) Serial.print("\t steps_laser_weaver");
   if(DEBUG) Serial.println(steps_laser_weaver);
   
   enableNema(true);
   weaving_direction = false;
   weaving_string_length = 0;
   
   millis_weaving = millis();
   
   lcdNotifyStateChanged();
   
   //Start index of current string as photo_index
   readWordOnlyPositive(mandala_fram_adress + (current_string * 4), &photo_index);
   //Endex of current string as first weaving_index
   readWordOnlyPositive(mandala_fram_adress + (current_string * 4 + 2), &weaving_index);   

   weaving_string_length += getStringLength(photo_index, weaving_index, modulo, true) * frame_diameter/1000.0 + 0.005;

   //Move to inital position, from tooth to middle of hole
      setMicrostepping(true);
      //Move over init tooth
      while(photo_tooth)
      {
         doStep(weaving_direction, VEL_MS);
      }
      //To be sure hole is reached
      //doSteps(weaving_direction, 5, VEL_MS);   
      //Measure the hole
      int hole = whileDebounced(false, weaving_direction, steps_hole/2); 
      //Align with hole
      while(photo_tooth)
      {
         doStep(!weaving_direction, VEL_MS);
      }   
      //Center at hole
      doSteps(!weaving_direction, hole/2, VEL_MS);

   if(DEBUG) Serial.println("Approached initial position.");
   
   boolean weaving_done = false;
   
   while(!weaving_done)
   {
      if(DEBUG) Serial.println("------------------------");
      if(DEBUG) Serial.print("current_string: ");
      if(DEBUG) Serial.println(current_string);    
      if(DEBUG) Serial.print("weaving_index: ");
      if(DEBUG) Serial.println(weaving_index);
      if(DEBUG) Serial.print("photo_index: ");
      if(DEBUG) Serial.println(photo_index);
      if(DEBUG) Serial.print("weaving_string_length: ");
      if(DEBUG) Serial.println(weaving_string_length);
      
      moveToHole(weaving_index, photo_index); // One hole before actual hole

      //Let mechanix settle
      delay(500);
      
      setMicrostepping(true);
      
      //Makes sure hole delta 1 is reached, prevents bouncing
      doSteps(weaving_direction, steps_hole/3, VEL_MS);    //steps_hole/3
   
      //Align with tooth 0 no need for debounce yet
      while(!photo_tooth)
      {
         doStep(weaving_direction, VEL_MS);
      }
   
      //Makes sure tooth delta 0 is reached, prevents bouncing, double secured
      doSteps(weaving_direction, steps_tooth/10, VEL_MS);  //steps_tooth/2?? too big
   
      //Move over the tooth
      whileDebounced(true, weaving_direction, steps_tooth/2);
   
      //NEW TRY REMOVE MAYBE? 07.11.20 Makes sure hole delta 0 is reached, prevents bouncing
      doSteps(weaving_direction, 3, VEL_MS); 
      
      //Measure the hole
      int hole = whileDebounced(false, weaving_direction, steps_hole/2);
         
      if(DEBUG) Serial.print("Hole width: ");
      if(DEBUG) Serial.println(hole);
   
      //Align with hole
      while(photo_tooth)
      {
         doStep(!weaving_direction, VEL_MS);
      }
   
      //Center at hole
      doSteps(!weaving_direction, hole/2, VEL_MS);
   
      //Laser/Weaver Adjustment
      doWeaverLaserAdjustment(true);
   
      weaver_state = !weaver_state;
      setWeaver(weaver_state);

      //delay(500); //Could be reduced I guess
      
      int weaving_index_old = weaving_index;
   
      if(!weaver_state)
      {
         if(current_string == mandala_num_strings-1)
         {
            if(DEBUG) Serial.println("Done!!! <3");
            weaving_done = true;
            return;
         }
         else
         {
            current_string++;     
            readWordOnlyPositive(mandala_fram_adress + (current_string * 4), &weaving_index);
            //Add length of upcoming connection between endex and new index, 5mm for tooth
            weaving_string_length += getStringLength(weaving_index_old, weaving_index, modulo, false) * frame_diameter/1000.0 + 0.005;
         }
      }
      else
      {          
         readWordOnlyPositive(mandala_fram_adress + (current_string * 4) + 2, &weaving_index);
         //Add length of upcoming string, 5mm for tooth
         weaving_string_length += getStringLength(weaving_index_old, weaving_index, modulo, true) * frame_diameter/1000.0 + 0.005;
      }
      
      lcdNotifyStateChanged();
     
      doWeaverLaserAdjustment(false);

      //Check if button was pressed      
      handleUserInput();

      if(weaving_string_length > available_string)
      {
         if(DEBUG) Serial.println("Out of string.");
         setState(STATE_PAUSED);
         lcd.setCursor(20-4, 0);
         lcd.print("#OOS"); 
      }

      if(current_string == pause_string && weaver_state)
      {
         if(DEBUG) Serial.println("Paused at pause_string.");
         setState(STATE_PAUSED);
      }
      
      //Handling pause, resume and stop
      if(state != STATE_WEAVING)
      {
         //?lcdNotifyStateChanged();         
         while(state == STATE_PAUSED)
         {
            handleUserInput();
            delay(50);
         }
         
         //?lcdNotifyStateChanged();  
         //Resume Weaving          
         if(state == STATE_WEAVING)
         {
            if(weaving_string_length > available_string) weaving_string_length = 0;
            enableNema(true);
         }
         //Stop weaving
         else
         {
            fram.writeWord(FRAM_ADR_CURRENT_STRING, current_string);
            return;
         }
         
      }

     
   }
   
}

//while 'tooth'/'hole' move in direction 'dir' and minimum of 'steps_min' have to be moved --> prevents bouncing errors
int whileDebounced(boolean tooth, boolean dir, int steps_min)
{   
   int steps = 0;
                  
   do
   {      
      while((photo_tooth & tooth) || (!photo_tooth & !tooth))
      {
         doStep(dir, VEL_MS);
         steps++;
      }
   
      if(steps < steps_min)
      {
         if(DEBUG) Serial.print("Debounce accomplished");
         if(DEBUG) {Serial.print(" <"); Serial.print(steps); Serial.print("/"); Serial.print(steps_min); Serial.println(">! ");}
         steps = 0;
         while((!photo_tooth & tooth) || (photo_tooth & !tooth))
         {
            doStep(dir, VEL_MS);
         }  
      }
   } 
   while(steps < steps_min);   

   return steps;
}

//Moves laser from 'index_from' to beginning of the hole BEFORE 'index_to'
void moveToHole(int index_to, int index_from)
{
   setMicrostepping(false);
   
   int delta;
   int moved = 0;
   
   if(index_to > index_from)
   {
      delta = index_to - index_from;
      weaving_direction = false;   
   }
   else if(index_to < index_from)
   {
      delta = index_from - index_to;         
      weaving_direction = true;  
   }
   
   if(delta > modulo/2) 
   {
      delta = modulo - delta;
      weaving_direction = !weaving_direction;  
   }    

   //For soft movement
   /*float v_max = 10.0 - 9.75 * (1.0/delta);
   Serial.println(v_max);
   a = -4 * v_max / (delta*delta);
   b = 4 * v_max / delta;*/
   float v = 0.01; //0.001  
   int acc_limit = delta/2;   

   if(DEBUG) {Serial.print("From "); Serial.print(index_from); Serial.print(" To "); Serial.print(index_to); Serial.print(" Delta "); Serial.println(delta);}

   int steps_min = 100;
   photo_interrupt = false;
   
   while(moved < delta - 1 /*&& state == STATE_WEAVING*/)
   {
      boolean next_hole = false;
      int steps = 0;
      //float v = a * moved * moved + b * moved + 0.15; //Velocity polynome
      //float v = 0.5;          
      
      while(!next_hole)
      {
         if(photo_interrupt && !photo_tooth)
         {
            //OLD: in Halfsteps, /13 to approximate, better measure steps in future
            if(steps > (steps_hole_full + steps_tooth_full) /2) //Steps needed to move over hole and tooth
            {
               next_hole = true;
               moved++;
               photo_interrupt = false;
               if(DEBUG) {Serial.print(moved); Serial.print("("); Serial.print(steps); Serial.print(")["); Serial.print(v); Serial.print("]   |   ");}              
            }
            else
            {
               if(DEBUG) {Serial.print("DEBOUNCED!!"); Serial.print("("); + Serial.print(steps); Serial.print(")!! ");}
               photo_interrupt = false;
            }
         }
         else if(photo_interrupt && photo_tooth)
         {
            photo_interrupt = false;
         }
         
         if(moved < acc_limit && delta > 5)
         {
            v += 0.03;
         }
         else if(delta - moved < acc_limit && delta > 5)
         {
            v -= 0.03;
            if(v < 0) v = 0.05;
         }
         
         doStep(weaving_direction, v);
         steps++;
      }
      if(steps < steps_min) steps_min = steps;
   }
   
   if(DEBUG) Serial.println();
   if(DEBUG) {Serial.print("steps_min: "); Serial.println(steps_min);}
   photo_index = index_to;
}
