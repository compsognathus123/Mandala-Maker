/*void servotest()
{
   Serial.println("ENDELSS SERVOTEST");
   while(1)
   {      
      setWeaver(true);
      delay(1000);
      setWeaver(false);
      delay(1000);
   }
}
//Sucessfull for 300 moves
void test()
{
   enableNema(true);
   setMicrostepping(true);
   
   //Move to inital position, from tooth to middle of hole
      //Move over init tooth
      while(photo_tooth)
      {
         doStep(weaving_direction, VEL_MS);
      }
      //To be sure hole is reached
      doSteps(weaving_direction, 5, VEL_MS);   
      //Measure the hole
      int hole = whileDebounced(false, weaving_direction, steps_hole/2);   
      //Align with hole
      while(photo_tooth)
      {
         doStep(!weaving_direction, VEL_MS);
      }   
      //Center at hole
      doSteps(!weaving_direction, hole/2, VEL_MS);
      
      Serial.println("init Pos reached");
      delay(3000);
      
   while(1)
   {
      moveToHole(101, 0);      
      setMicrostepping(true);
      delay(500);
      //To be sure hole is reached
      doSteps(weaving_direction, 40, VEL_MS);  
      
      delay(1000);
      
      moveToHole(319, 100); 
      setMicrostepping(true);
      delay(500);
      //To be sure hole is reached
      doSteps(weaving_direction, 40, VEL_MS);  
      
      delay(1000);
   }
}

void setAngle(int angle)
{
   long duration = millis();

   int pulse = map(angle, 0, 180, 700, 2000);

   while(millis() - duration < 200)
   {
      digitalWrite(10, HIGH);
      delayMicroseconds(pulse);
      digitalWrite(10, LOW);
      delay(15);
   }
}

void initWeavingState()
{      
   //nfc_pull = false;
   enableNema(true);

   //Endex of current string as first weaving_index, weaver at high position
   readWord(mandala_fram_adress + (current_string * 4 + 2), &weaving_index);
   
   Serial.println("Play"); 
   Serial.print("Current string/weaving_index: ");
   Serial.print(current_string);
   Serial.print("\t");
   Serial.println(weaving_index);

   //printMandala();    
                 
}

void handleWeaving()
{   
   Serial.print("String: ");
   Serial.println(current_string);    
   Serial.print("Weaving index: ");
   Serial.println(weaving_index);

   moveToHole(weaving_index, photo_index); // One hole before actual hole

   if(state != STATE_WEAVING) return;
   setMicrostepping(true);
   
   //Makes sure hole delta 1 is reached, prevents bouncing
   doSteps(weaving_direction, 20, VEL_MS);    //steps_hole/4  

   //Align with tooth 0 no need for debounce yet
   while(!photo_tooth)
   {
      doStep(weaving_direction, VEL_MS);
   }

   //Makes sure tooth delta 0 is reached, prevents bouncing, double secured
   doSteps(weaving_direction, 10, VEL_MS);  //steps_tooth/2 

   //Move over the tooth
   whileDebounced(true, weaving_direction, steps_tooth/2);

   //Measure the hole
   int hole = whileDebounced(false, weaving_direction, steps_hole/2);
      
   Serial.print("Hole width: ");
   Serial.println(hole);

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
   
   delay(500);
   
   int weaving_index_old = weaving_index;

   if(!weaver_state)
   {
      if(current_string == mandala_num_strings)
      {
         Serial.println("Done!!! <3");
      }
      else
      {
         current_string++;     
         readWord(mandala_fram_adress + (current_string * 4), &weaving_index);
      }
   }
   else
   {          
      readWord(mandala_fram_adress + (current_string * 4) + 2, &weaving_index);
   }
  
   doWeaverLaserAdjustment(false);
}

void handleWeavingOLD()
{
   setMicrostepping(false);
   int delta;
   int old_delta = 0;
   int steps = 0;
   boolean started = false;

   Serial.print("String: ");
   Serial.println(current_string);    
   Serial.print("\t Weaving index: ");
   Serial.println(weaving_index);
    
   do
   {      
      if(weaving_index > photo_index)
      {
         delta = weaving_index - photo_index;
         if(delta > modulo/2) //Muss hier gemacht werden da weaving_direction jederzeit konsistent sein muss wegen dem photo_interrupt?? 
         {
            delta = modulo - delta;
            weaving_direction = true;  
         }    
         else
         {
            weaving_direction = false;            
         }
      }
      else if(weaving_index < photo_index)
      {
         delta = photo_index - weaving_index;         
         if(delta > modulo/2) //Muss hier gemacht werden da weaving_direction jederzeit konsistent sein muss wegen dem photo_interrupt??
         {
            delta = modulo - delta;
            weaving_direction = false;  
         }    
         else
         {
            weaving_direction = true;            
         }
      }
      else if(weaving_index == photo_index)
      {
         delta = 0;
         Serial.print("delta zero should not occur![");
         Serial.print(photo_index);
         Serial.print("]");  
      }
    
   
     
      if(delta > 1)
      {
         //if(delta <= 10) delay((10 - delta) * 2);
         
         doStep(weaving_direction, VEL_NORMAL);
         steps++;
        // Serial.println("delta > 1 step");
      }

      int deltadelta = old_delta - delta;
      //Handle wrong photo interrupts
      if(deltadelta == 1)
      {
         if(steps < 4) toofewsteps++;
         //Proper move done
         old_delta = delta;
         Serial.print("\t");
         Serial.print(delta);
         Serial.print("(");
         Serial.print(steps);
         Serial.print(")");
         Serial.print("[");
         Serial.print(photo_index);
         Serial.print("]");
         steps = 0;
      }
      else if(deltadelta > 1)   //More than one photo_interrupt after doing one step
      {
         //One photo backwards to compensate error
         if(!weaving_direction)
         {
            photo_index -= (deltadelta - 1); //WDIR ++
         }
         else
         {
            photo_index += (deltadelta - 1); //WDIR --
         }
         delta++; //so loop does not exit!
         
         Serial.print("\t");
         Serial.print("Compensated! (");
         Serial.print(old_delta - delta);
         Serial.print(") p_i[");
         Serial.print(photo_index);
         Serial.print("]");
      }
      else if(deltadelta < 0)   
      {
         if(started) //mysterious
         {
            Serial.print("\t dd<0andStarted");
            Serial.print(delta);
            Serial.print("(");
            Serial.print(steps);
            Serial.print(")");
            Serial.print("[");
            Serial.print(photo_index);
            Serial.print("].");
         }
         else //delta negative at weaving start
         {
            old_delta = delta;
            Serial.println("first Move");
            Serial.print("\t");
            Serial.print(delta);
            started = true;
         }
      }
      
      handleCapSense(); //Check buttons
   } 
   while(delta > 1 && state == STATE_WEAVING);

   
   Serial.println("");
   Serial.print("Too few steps: ");
   Serial.println(toofewsteps);
   if(state != STATE_WEAVING) return;
   
   setMicrostepping(true);
   
   //Makes sure hole delta 1 is reached, prevents bouncing
   doSteps(weaving_direction, 20, VEL_MS);    //steps_hole/4  

   //Align with tooth 0 no need for debounce yet
   while(!photo_tooth)
   {
      doStep(weaving_direction, VEL_MS);
   }

   //Makes sure tooth delta 0 is reached, prevents bouncing, double secured
   doSteps(weaving_direction, 10, VEL_MS);  //steps_tooth/2 

   //Move over the tooth
   whileDebounced(true, weaving_direction, steps_tooth/2);

   //Measure the hole
   int hole = whileDebounced(false, weaving_direction, steps_hole/2);
      
   Serial.print("Hole width: ");
   Serial.println(hole);

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
   
   delay(500);
   
   int weaving_index_old = weaving_index;

   if(!weaver_state)
   {
      if(current_string == mandala_num_strings)
      {
         Serial.println("Done!!! <3");
      }
      else
      {
         current_string++;     
         readWord(mandala_fram_adress + (current_string * 4), &weaving_index);
      }
   }
   else
   {          
      readWord(mandala_fram_adress + (current_string * 4) + 2, &weaving_index);
  
      Serial.print("weaving index: ");
      Serial.println(weaving_index);   
   }
  
   doWeaverLaserAdjustment(false);

   photo_index = weaving_index_old;
    
}

int measureTolerance()
{
   enableNema(true);
   setMicrostepping(true);

   int fwd = 0;
   int bwd = 0;
   
   for(int i = 0; i < 10; i++)
   {
      int fwd_loop = 0;
      int bwd_loop = 0;
      
      while(!photo_tooth)
      {
         doStep(true, VEL_MS);
      }

      while(photo_tooth)
      {
         doStep(false, VEL_MS);
         bwd_loop++;
      }
      
      while(!photo_tooth)
      {
         doStep(false, VEL_MS);
      }
      
      while(photo_tooth)
      {
         doStep(true, VEL_MS);
         fwd_loop++;
      }
      fwd += fwd_loop;
      bwd += bwd_loop;
      Serial.print(fwd_loop);
      Serial.print("\t");
      Serial.println(bwd_loop);
   }
   
   fwd /= 10;
   bwd /= 10;
   
   Serial.print("Tolerance fwd/bwd: ");
   Serial.print(fwd);
   Serial.print("\t");
   Serial.println(bwd);
   
   setMicrostepping(false);
   
}*/
