/*
void capISR()
{
   cap_interrupt = true;
}

boolean initCapSense()
{      
   pinMode(CAP_INTERRUPT, INPUT_PULLUP);
   pinMode(CAP_RESET, OUTPUT);
   
   boolean success = cap.begin();
   
   cap.writeRegister(0x2A, 0x80);  // Disable Multitouch
   cap.writeRegister(0x41, 0x39);  // 0x41 default 0x39 use 0x41  — Set "speed up" setting back to off
   cap.writeRegister(0x44, 0x41);  // 0x44 default 0x40 use 0x41  — Set interrupt on press but not release
   cap.writeRegister(0x28, 0x00);  // 0x28 default 0xFF use 0x00  — Turn off interrupt repeat on button hold
   
   attachInterrupt(digitalPinToInterrupt(CAP_INTERRUPT), capISR, FALLING);

   return success;
}

void touchPlay()
{
   if(state == STATE_READY)
   {                  
      setState(STATE_WEAVING);
      weaveMandala();      
   }

   if(state == STATE_WEAVING)
   {
      
   }

   if(state == STATE_CALIBRATING)
   {           
      state = STATE_WEAVING; 
      lcdNotifyStateChanged();
      weaveMandala();         
   }

   if(state == STATE_PAUSED)
   {
      state = STATE_WEAVING;
      lcdNotifyStateChanged();
      Serial.println("Resumed weaving.");
         
   }
}

void touchStop()
{
   if(state == STATE_READY)
   {         
      weaver_state = HIGH;
      setWeaver(weaver_state);   
   }

   if(state == STATE_WEAVING || state == STATE_PAUSED)
   {         
//      nfc_pull = true;
      enableNema(false);
      current_string = 0;
      Serial.println("Stopped Weaving."); 
      state = STATE_READY;
      lcdNotifyStateChanged();
   }

   if(state == STATE_CALIBRATING)
   {  
      Serial.println("Stopped Calibrating."); 
      enableNema(false);
      state = STATE_READY;
      lcdNotifyStateChanged();
   }

   
}

void touchPause()
{
   if(state == STATE_READY)
   {
   }

   if(state == STATE_WEAVING)
   {         
      //TODO: Restart from last woven node
      //nfc_pull = true;
      enableNema(false); //disable Nema
      
      Serial.println("Paused Weaving.");
      state = STATE_PAUSED;
      lcdNotifyStateChanged();
   }

   if(state == STATE_CALIBRATING)
   {  
      
   }

   if(state == STATE_PAUSED)
   {     
   }
}

void touchCalibrate()
{
   if(state == STATE_READY)
   {
      measureHoleToothWidth();
      Serial.println("Adjust weaver angle now.");
      
      calibrating_mode = -1;
      state = STATE_CALIBRATING; 
      lcdNotifyStateChanged();  
   }

   if(state == STATE_WEAVING)
   {   
   }

   if(state == STATE_CALIBRATING)
   {  
      if(calibrating_mode == 0)
      {
         Serial.println("Adjust weaver position now.");
         steps_laser_weaver = 0;
      }
      if(calibrating_mode == 1)
      {
         Serial.print("Weaver/Laser adjustment: ");
         Serial.println(steps_laser_weaver);
         fram.writeWord(FRAM_ADR_STEPS_LASERWEAVER, steps_laser_weaver);
         setWeaver(true);
         touchStop();
      }
      
      calibrating_mode++;
   }

   if(state == STATE_PAUSED)
   {     
      //Calibrating mode in paused, manual tuning?
   }
}

void touchMinus()
{
  if(state == STATE_READY)
   {
      enableNema(true);
      setMicrostepping(false);
      doSteps(false, 2, 0.1); 
      delay(200);

      //If only single touch for small movements
      //Clear interrupt flag in main controlregister
      unsigned char main_reg = cap.readRegister(0x00);
      main_reg &= 0xFE;
      cap.writeRegister(0x00, main_reg); 
      
      float v = 0.1;
      
      while(cap.readRegister(CAP1188_SENINPUTSTATUS) == 32)
      {
         doSteps(false, 5, v);
         if(v < 15) v += 0.1;
         
          //Clear interrupt flag in main controlregister
         unsigned char main_reg = cap.readRegister(0x00);
         main_reg &= 0xFE;
         cap.writeRegister(0x00, main_reg); 
      }

      enableNema(false);
   }
   
   if(state == STATE_CALIBRATING)
   {
      if(calibrating_mode == 0 || calibrating_mode == 2 || calibrating_mode == 3)  //Adjust Weaver angle
      {
         if(servo_pulse - 3 > 0)
         {
            servo_pulse -= 3;
            Serial.println(servo_pulse);
            updateWeaver();
         }
      }
      else if(calibrating_mode == 1)   //Adjust Weaver position
      {         
         doSteps(false, 10, VEL_MS);
         steps_laser_weaver -= 5;
         Serial.println(steps_laser_weaver);
      }
   }   
}

void touchPlus()
{
   if(state == STATE_READY)
   {
      enableNema(true);
      setMicrostepping(false);
      doSteps(true, 2, 0.1); 
      delay(200);

      //If only single touch for small movements
      //Clear interrupt flag in main controlregister
      unsigned char main_reg = cap.readRegister(0x00);
      main_reg &= 0xFE;
      cap.writeRegister(0x00, main_reg); 
      
      float v = 0.1;
      
      while(cap.readRegister(CAP1188_SENINPUTSTATUS) == 16)
      {
         doSteps(true, 5, v);
         if(v < 15) v += 0.1;
         
          //Clear interrupt flag in main controlregister
         unsigned char main_reg = cap.readRegister(0x00);
         main_reg &= 0xFE;
         cap.writeRegister(0x00, main_reg); 
      }

      enableNema(false);
   }
   
   if(state == STATE_CALIBRATING)
   {
      if(calibrating_mode == 0 || calibrating_mode == 2 || calibrating_mode == 3)  //Adjust Weaver angle
      {
         if(servo_pulse + 3 < 180)
         {
            servo_pulse += 3;
            Serial.println(servo_pulse);
            updateWeaver();
         }
      }
      else if(calibrating_mode == 1)   //Adjust Weaver position
      {         
         doSteps(true, 10, VEL_MS);
         steps_laser_weaver += 5;  
         Serial.println(steps_laser_weaver);      
      }
   }
   
}


//Make sure to call handleCapsense() regulary to detect touch events, and also to clear capsense interrupt register (when cap_interrupt == false)
void handleCapSense()
{
   if(cap_interrupt)
   {      
      unsigned char touched = cap.readRegister(CAP1188_SENINPUTSTATUS);
      Serial.print("touched: ");
      Serial.println(touched);
      //CHANGE NUMBERS
      switch(touched)
      {
         case 1:     //Play
            cap_interrupt = false; 
            //touchPlay();
            //buttonClick();
            break;
         case 2:     //Stop
           // touchStop();
            //buttonBack();
            break;
         case 4:     //Pause
            touchPause();
            break;
         case 8:     //Calibrate
            touchCalibrate();
            break;
         case 16:    //Plus
            //touchPlus();
            //rotaryTick(true);
            break;
         case 32:    //Minus
            //touchMinus();
            //rotaryTick(false);
            break;
      }
      
      cap_interrupt = false; 
   }
   else
   {
      //Clear interrupt flag in main controlregister
      unsigned char main_reg = cap.readRegister(0x00);
      main_reg &= 0xFE;
      cap.writeRegister(0x00, main_reg);      
   }
}

*/
