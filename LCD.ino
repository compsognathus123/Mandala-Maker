#define INT 1
#define BYTE 2
#define FLOAT 3
#define LONG 3

int menu_index;
void *rotary_var;
int rotary_var_type;
int rotary_stepsize;
int rotary_min, rotary_max;

void (*lcd_update_method)(void);

//Mandala Creation #CM
volatile int new_optimization;
volatile int new_modulo;
volatile int new_times_lsi, new_times_msi; //Most significant int, least significant int

//Calibration #C
volatile int new_anglehigh, new_anglelow, new_laseroffset;

//Settings #S
volatile int new_available_string, new_current_string, new_pause_string;
volatile int new_frame_diameter;


void lcdPrintSetup(char *text)
{
   lcdClearLine(1);
   lcd.setCursor(1, 1);
   lcd.print(text);
}

void lcdMandalaCreation(char *text)
{
   lcdClearLine(3);
   lcd.setCursor(0, 3);
   lcd.print("*");
   lcd.print(text);
}

void lcdClearLine(byte line)
{
   lcd.setCursor(0, line);
   lcd.print("                    ");
}

void setState(int new_state)
{
   state = new_state;
   lcdNotifyStateChanged();

   if(state != STATE_PAUSED && state != STATE_SETUP && state != STATE_WEAVING)
   {
      menu_index = 0;
      lcd.setCursor(0,menu_index);
      lcd.print("~");      
   }
}

void configureRotary(void *var, int var_type, float stepsize, int minval, int maxval, void *update_method(void))
{
   rotary_var = var;
   rotary_var_type = var_type;
   rotary_stepsize = stepsize;
   rotary_min = minval;
   rotary_max = maxval;
   lcd_update_method = update_method;

   if(var != &menu_index)
   {      
      lcd.setCursor(0,menu_index);
      lcd.print("?");
   }
} 

void handleUserInput()
{
   if(rotary_changed)
   {       
      rotary_changed = false;  
      (*lcd_update_method)();
   }

   if(button_interrupt)
   {
      if(button_interrupt == BUTTON_OKAY)
      {
         button_interrupt = BUTTON_NONE;
         Serial.println("Button OKAY");
         buttonOkay();
      }
      else if(button_interrupt == BUTTON_BACK)
      {
         button_interrupt = BUTTON_NONE;
         Serial.println("Button BACK");
         buttonBack();
      }
      delay(75); //Cheap debounce
   }  
}

void buttonISR()
{
   if(!digitalRead(BUTTON_ROTARY))
   {
      button_interrupt = BUTTON_OKAY;
   }
   else
   {
      button_interrupt = BUTTON_BACK;
   }
}

int16_t bounce;


void rotaryISR()
{   
   /*while(bounce != 0xF000)
   {
      bounce = (bounce<<1) | digitalRead(ROTARYA) | 0xe000;
   }   
   EIFR |= 0b100;*/
    
   rotary_changed = true;
   bool dir = !(digitalRead(ROTARYA) != digitalRead(ROTARYB));
   
   if(rotary_var_type == INT)
   {
      int val = *((int*)rotary_var);
      
      if(dir && val <= rotary_max - rotary_stepsize)
      {
         (*((int*)rotary_var)) += rotary_stepsize;
      }
      else if(!dir && val >= rotary_min + rotary_stepsize)
      {
         (*((int*)rotary_var)) -= rotary_stepsize;
      }
      else if(!dir && val == rotary_min && rotary_var != &menu_index)
      {
         (*((int*)rotary_var)) = rotary_max;
      }
      else if(dir && val == rotary_max && rotary_var != &menu_index)
      {
         (*((int*)rotary_var)) = rotary_min;
      }
   }/*
   else if(rotary_var_type == FLOAT)
   {
      float val = *((float*)rotary_var);
      
      if(dir && val <= rotary_max - rotary_stepsize)
      {
         *((float*)rotary_var) += rotary_stepsize;
      }
      else if(!dir && val >= rotary_min + rotary_stepsize)
      {
         *((float*)rotary_var) -= rotary_stepsize;
      }
      else if(!dir && floatCompare(val, rotary_min))
      {
         (*((float*)rotary_var)) = rotary_max;
      }
      else if(dir && floatCompare(val, rotary_max))
      {
         (*((float*)rotary_var)) = rotary_min;
      }
   }
   else if(rotary_var_type == BYTE)
   {
      byte val = *((byte*)rotary_var);
      byte stepVal = (byte)round(rotary_stepsize);
      
      if(dir && val <= rotary_max - stepVal)
      {
         (*((byte*)rotary_var)) += stepVal;
      }
      else if(!dir && val >= rotary_min + stepVal)
      {
         (*((byte*)rotary_var)) -= stepVal;
      }
      else if(!dir && val == rotary_min)
      {
         (*((byte*)rotary_var)) = rotary_max;
      }
      else if(dir && val == rotary_max)
      {
         (*((byte*)rotary_var)) = rotary_min;
      }
   }
   else if(rotary_var_type == LONG)
   {
      long val = *((long*)rotary_var);
      int stepVal = round(rotary_stepsize);
      
      if(dir && val <= rotary_max - stepVal)
      {
         (*((long*)rotary_var)) += stepVal;
      }
      else if(!dir && val >= rotary_min + stepVal)
      {
         (*((long*)rotary_var)) -= stepVal;
      }
      else if(!dir && val == rotary_min)
      {
         (*((long*)rotary_var)) = rotary_max;
      }
      else if(dir && val == rotary_max)
      {
         (*((long*)rotary_var)) = rotary_min;
      }
   }*/
   
   Serial.println(*((int*)rotary_var));
   EIFR |= 0b100;
   //(*lcd_update_method)();
}

void buttonBack()
{
   if(state == STATE_SETTINGS)
   {
      if(rotary_var == &menu_index)
      {
         setState(STATE_READY);
      }
      else
      {
         setState(STATE_SETTINGS);
      }
   }
   else if(state == STATE_CALIBRATING)
   {
      if(rotary_var == &menu_index)
      {
         setState(STATE_READY);
      }
      else
      {
         setState(STATE_CALIBRATING);
      }
   }   
   else if(state == STATE_CALIBRATING_STEPS)
   {      
      setState(STATE_CALIBRATING);
   }
   else if(state == STATE_MANDALA)
   {
      setState(STATE_READY);
   }
   else if(state == STATE_CREATE_MANDALA)
   {
      if(rotary_var == &menu_index)
      {
         setState(STATE_MANDALA);
      }
      else
      {
         setState(STATE_CREATE_MANDALA);
      }
   }
   else if(state == STATE_WEAVING)
   {     
      setState(STATE_PAUSED);
   }
   else if(state == STATE_PAUSED)
   {     
      setState(STATE_READY);
   }
   else if(state == STATE_READY)
   {
      weaver_state = HIGH;
      setWeaver(weaver_state); 
   }
}

void buttonOkay()
{            
   if(state == STATE_READY)
   {
      switch(menu_index)
      {
         //Start Weaving
         case 0:  setState(STATE_WEAVING);
                  configureRotary(&menu_index, INT, 1, 0, 3, updateCursorLCD);
                  weaveMandala();
                  setState(STATE_READY);
                  break;
         //Mandala
         case 1:  setState(STATE_MANDALA);
                  break;
         //Settings
         case 2:  
                  setState(STATE_SETTINGS);
                  break;
         //Calibrating
         case 3:  setState(STATE_CALIBRATING);
                  break;
         default: break;
      }                  
   }   
   else if(state == STATE_WEAVING)
   {
      setState(STATE_PAUSED);
   }
   else if(state == STATE_PAUSED)
   {
      setState(STATE_WEAVING);
   }
   else if(state == STATE_MANDALA)
   {
      setState(STATE_CREATE_MANDALA);
   }
   else if(state == STATE_CREATE_MANDALA)
   {
      if(rotary_var != &menu_index)
      {
         if(rotary_var == &new_optimization)
         {
            optimization = new_optimization;
            fram.writeWord(FRAM_ADR_OPTIMIZATION, optimization);
         }
         
         if(rotary_var == &new_times_msi)
         {
            configureRotary(&new_times_lsi, INT, 1, 0, 999, &updateTimesLsiLCD);            
         }
         else
         {
            configureRotary(&menu_index, INT, 1, 0, 3, &updateCursorLCD);            
         }
      }
      else
      {            
         switch(menu_index)
         {
            //Modulo
            case 0:  configureRotary(&new_modulo, INT, 1, 0, 999, &updateModuloLCD);                     
                     break;
            //Times
            case 1:  configureRotary(&new_times_msi, INT, 1, 1, 999, &updateTimesMsiLCD);
                     break;
            //Optimization
            case 2:  configureRotary(&new_optimization, INT, 5, 0, 1000, &updateOptimizationLCD);
                     break;
            //Create...
            case 3:  float new_times = new_times_msi + new_times_lsi/1000.0;
                     createMandala(new_modulo, new_times, new_optimization);
                     setState(STATE_MANDALA);
                     break;
            default: break;
         }
      }
   }
   else if(state == STATE_CALIBRATING)
   {
      if(rotary_var != &menu_index)
      {
         //Store new values in FRAM and update variables
         if(rotary_var == &new_anglehigh)
         {
            servo_pulse_high = new_anglehigh;
            fram.writeWord(FRAM_ADR_SERVO_HIGH, servo_pulse_high); 
         }
         else if(rotary_var == &new_anglelow)
         {
            servo_pulse_low = new_anglelow;
            fram.writeWord(FRAM_ADR_SERVO_LOW, servo_pulse_low); 
         }
         else if(rotary_var == &new_laseroffset)
         {
            steps_laser_weaver = new_laseroffset;
            fram.writeWord(FRAM_ADR_STEPS_LASERWEAVER, steps_laser_weaver);
         }
         
         configureRotary(&menu_index, INT, 1, 0, 3, updateCursorLCD);
      }
      else
      {            
         switch(menu_index)
         {
            //Measure Steps...
            case 0:  measureHoleToothWidth();    
                     setState(STATE_CALIBRATING_STEPS);              
                     break;
            //Angle high
            case 1:  configureRotary(&new_anglehigh, INT, 1, 0, 180, &updateAngleHighLCD);
                     break;
            //Angle low
            case 2:  configureRotary(&new_anglelow, INT, 1, 0, 180, &updateAngleLowLCD);
                     break;
            //Laser Offset
            case 3:  configureRotary(&new_laseroffset, INT, 1, 0, 200, &updateLaserOffsetLCD);
                     break;
            default: break;
         }
      }
   }
   else if(state == STATE_CALIBRATING_STEPS)
   {      
      setState(STATE_CALIBRATING);  
   }
   else if(state == STATE_SETTINGS)
   {
      if(rotary_var != &menu_index)
      {         
         //Store new values in FRAM and update variables
         if(rotary_var == &new_current_string)
         {
            current_string = new_current_string; 
            fram.writeWord(FRAM_ADR_CURRENT_STRING, current_string);
         }
         else if(rotary_var == &new_frame_diameter)
         {
            frame_diameter = new_frame_diameter;
            fram.writeWord(FRAM_ADR_FRAME_DIA, frame_diameter);
         }
         else if(rotary_var == &new_available_string)
         {
            available_string = new_available_string;
            fram.writeWord(FRAM_ADR_AVAILABLE_STRING, available_string);
         }
         else if(rotary_var == &new_pause_string)
         {
            pause_string = new_pause_string;
            fram.writeWord(FRAM_ADR_PAUSE_STRING, pause_string);
         }
         
         configureRotary(&menu_index, INT, 1, 0, 3, updateCursorLCD);
      }
      else
      {            
         switch(menu_index)
         {
            //Current String
            case 0:  configureRotary(&new_current_string, INT, 1, 0, mandala_num_strings-1, &updateCurrentStringLCD);                     
                     break;
            //Frame Diameter
            case 1:  configureRotary(&new_frame_diameter, INT, 5, 0, 2000, &updateFrameDiameterLCD);
                     break;
            //Available String
            case 2:  configureRotary(&new_available_string, INT, 1, 0, 999, &updateAvailableStringLCD);
                     break;
            //Pause String
            case 3:  configureRotary(&new_pause_string, INT, 1, 0, mandala_num_strings-1, &updatePauseStringLCD);
                     break;
            default: break;
         }
      }
   }

   //If value is set, change cursor back to ->
   if(rotary_var == &menu_index && state != STATE_PAUSED && state != STATE_WEAVING)
   {      
      lcd.setCursor(0,menu_index);
      lcd.print("~");
   }
   
}

void updateCursorLCD()
{
   for(int i = 0; i < rotary_max+1; i++)
   {
      lcd.setCursor(0,i);
      if(i == menu_index)
      {
         lcd.print("~");
      }
      else
      {         
         lcd.print(" ");
      }
   }
}

void updateCurrentStringLCD()
{
   lcd.setCursor(9,0);
   lcd.print("         ");
   lcd.setCursor(9,0);
   lcd.print(new_current_string);
   lcd.print("(");
   int stringdex = 0;
   fram.readWord(mandala_fram_adress + (new_current_string) * 4, &stringdex);
   lcd.print(stringdex);
   lcd.print(")");
      
}
void updatePauseStringLCD()
{
   lcd.setCursor(11,3);
   lcd.print("     ");
   lcd.setCursor(11,3);
   if(new_pause_string)
   {
      lcd.print(new_pause_string);
   }
   else
   {         
      lcd.print("none");
   }
}
void updateFrameDiameterLCD()
{
   lcd.setCursor(12,1);
   lcd.print("      ");
   lcd.setCursor(12,1);
   lcd.print(new_frame_diameter);
   lcd.print("mm");      
}
void updateAvailableStringLCD()
{
   lcd.setCursor(14,2);
   lcd.print("    ");
   lcd.setCursor(14,2);
   lcd.print(new_available_string);
   lcd.print("m");
}
void updateModuloLCD()
{      
   lcd.setCursor(9,0);
   lcd.print("        ");
   lcd.setCursor(9,0);
   lcd.print(new_modulo);
}
void updateTimesMsiLCD()
{   
   lcd.setCursor(8,1);
   lcd.print("   .");
   lcd.setCursor(11-log10(new_times_msi+1),1); //Adjust position depending on display-size of value
   lcd.print(new_times_msi);
}
void updateTimesLsiLCD()
{
   lcd.setCursor(12,1);
   lcd.print("000   ");
   lcd.setCursor(15-log10(new_times_lsi+1),1); //Adjust position depending on display-size of value
   if(new_times_lsi != 0) lcd.print(new_times_lsi);
}
void updateOptimizationLCD()
{   
   lcd.setCursor(7,2);
   lcd.print("          ");
   lcd.setCursor(7,2);
   if(new_optimization == 0) 
   {
      lcd.print("none");           
   }
   else
   {
      lcd.print(round(new_optimization*(frame_diameter/1000.0)));
      lcd.print("mm"); 
   }
}
void updateAngleHighLCD()
{
   servo.write(new_anglehigh);
   lcd.setCursor(9,1);
   lcd.print("         "); 
   lcd.setCursor(9,1);
   lcd.print(new_anglehigh);
   lcd.print(" (deg)"); 
}
void updateAngleLowLCD()
{
   servo.write(new_anglelow);
   lcd.setCursor(9,2);
   lcd.print("         "); 
   lcd.setCursor(9,2);
   lcd.print(new_anglelow);
   lcd.print(" (deg)"); 
}
void updateLaserOffsetLCD()
{   
   //Align with center first..
   lcd.setCursor(14,3);
   lcd.print("      "); 
   lcd.setCursor(14,3);
   lcd.print(new_laseroffset);
}



//Prepare LCD for new state
void lcdNotifyStateChanged()
{
   if(state == STATE_WEAVING)
   {
      int stringindex = 0;
      int stringendex = 0;
      fram.readWord(mandala_fram_adress + (current_string) * 4, &stringindex);
      fram.readWord(mandala_fram_adress + (current_string) * 4 + 2, &stringendex);
      
      lcd.clear();
      lcd.setCursor(20-2, 0);
      lcd.print("#W"); 
      lcd.setCursor(0,0);
      lcd.print("String: ");
      lcd.print(current_string);  
      lcd.print("/");
      lcd.print(mandala_num_strings);  

      millis_loop = millis();
      lcd.setCursor(0,1);
      lcd.print("Time: ");  
      lcd.print((millis_loop-millis_weaving)/1000 / 60);
      lcd.print("m");  
      lcd.print((millis_loop-millis_weaving)/1000 % 60); 
      lcd.print("s");  
            
      lcd.setCursor(1,2);
      lcd.print("");  
      lcd.print(weaving_string_length);
      lcd.print("|");
      if (available_string < mandala_overall_length * (frame_diameter/1000.0))
      {
         lcd.print(available_string);
         lcd.print("|");
      }
      lcd.print(mandala_overall_length * (frame_diameter/1000.0));
      lcd.print("m");
     
      lcd.setCursor(1,3);
      lcd.print(stringindex); 
      lcd.print(" -> ");    
      lcd.print(stringendex); 
       
   }
   else if(state == STATE_PAUSED)
   {
      if(DEBUG) Serial.println("State: Paused.");
      lcd.setCursor(20-2, 0);
      lcd.print("#P"); 
          
      lcdClearLine(1); 
      lcd.setCursor(0,1);
      lcd.print("Enter to resume.");
      lcdClearLine(2); 
      lcd.setCursor(0,2);
      lcd.print("Back to stop.");
      
      enableNema(false);
   }
   else if(state == STATE_READY)
   {
      menu_index = 0;
      if(DEBUG) Serial.println("State: Ready.");
      configureRotary(&menu_index, INT, 1, 0, 3, updateCursorLCD);
      
      lcd.clear();
      lcd.setCursor(20-2, 0);
      lcd.print("#R"); 
        
      lcd.setCursor(1,0);
      lcd.print("Start Weaving...");
      lcd.setCursor(1,1);
      lcd.print("Mandala...");      
      lcd.setCursor(1,2);
      lcd.print("Settings...");
      lcd.setCursor(1,3);
      lcd.print("Calibrate...");

      enableNema(false);
         
   }
   else if(state == STATE_CALIBRATING)
   {      
      if(DEBUG) Serial.println("State: Calibrating.");
      configureRotary(&menu_index, INT, 1, 0, 3, updateCursorLCD);
      
      new_anglehigh = servo_pulse_high;
      new_anglelow = servo_pulse_low;
      new_laseroffset = steps_laser_weaver;
      
      lcd.clear();
      lcd.setCursor(20-2, 0);
      lcd.print("#C"); 
      lcd.setCursor(1,0);
      lcd.print("Measure Steps..."); 
      
      lcd.setCursor(1,1);
      lcd.print("AngleH: "); 
      lcd.print(new_anglehigh);
      lcd.print(" (deg)"); 
      
      lcd.setCursor(1,2);
      lcd.print("AngleL: "); 
      lcd.print(new_anglelow);
      lcd.print(" (deg)"); 
      
      lcd.setCursor(1,3);
      lcd.print("Laser Offs.: "); 
      lcd.print(new_laseroffset);
      
   }
   else if(state == STATE_CALIBRATING_STEPS)
   {      
      menu_index = 0;
      if(DEBUG) Serial.println("State: Calibrating, Steps.");
      configureRotary(&menu_index, INT, 1, 0, 0, updateCursorLCD);
      
      lcd.clear();
      lcd.setCursor(20-3, 0);
      lcd.print("#CS"); 
      
      lcd.setCursor(1,0);
      lcd.print("StepsHF: "); 
      lcd.print(steps_hole_full); 
      lcd.setCursor(1,1);
      lcd.print("StepsTF: "); 
      lcd.print(steps_tooth_full); 
      lcd.setCursor(1,2);
      lcd.print("StepsHM: "); 
      lcd.print(steps_hole); 
      lcd.setCursor(1,3);
      lcd.print("StepsTM: "); 
      lcd.print(steps_tooth); 
   
   }
   else if(state == STATE_SETUP)
   {
      menu_index = 1;
      updateCursorLCD();
      if(DEBUG) Serial.println("State: Setup.");
      lcd.clear();
      lcd.setCursor(20-6, 0);
      lcd.print("#Setup");
   }
   else if(state == STATE_SETTINGS)
   {      
      if(DEBUG) Serial.println("State: Settings.");
      configureRotary(&menu_index, INT, 1, 0, 3, updateCursorLCD);
      
      lcd.clear();
      lcd.setCursor(20-2, 0);
      lcd.print("#S");

      new_current_string = current_string;
      new_frame_diameter = frame_diameter;
      new_available_string = available_string;
      new_pause_string = pause_string;

      lcd.setCursor(1,0);
      lcd.print("String: ");
      lcd.print(new_current_string);
      lcd.print("(");
      int stringdex = 0;
      fram.readWord(mandala_fram_adress + (new_current_string) * 4, &stringdex);
      lcd.print(stringdex);
      lcd.print(")");
      
      lcd.setCursor(1,1);
      lcd.print("Frame Dia: ");
      lcd.print(new_frame_diameter);
      lcd.print("mm");
      
      lcd.setCursor(1,2);
      lcd.print("Thread Roll: ");
      lcd.print(new_available_string);
      lcd.print("m");
      
      lcd.setCursor(1,3);
      lcd.print("Pause at: ");
      if(new_pause_string)
      {
         lcd.print(new_pause_string);
      }
      else
      {         
         lcd.print("none");
      }
   }
   else if(state == STATE_MANDALA)
   {      
      menu_index = 0;
      if(DEBUG) Serial.println("State: Mandala.");
      configureRotary(&menu_index, INT, 1, 0, 0, updateCursorLCD);
      lcd.clear();
      lcd.setCursor(20-2, 0);
      lcd.print("#M");

      lcd.setCursor(1, 0);
      lcd.print("Create...");
      
      lcd.setCursor(0,1);
      lcd.print(modulo);
      lcd.print("|");
      lcd.print(times * 1.0, 3);
      
      lcd.setCursor(0, 2);
      lcd.print("Strings: ");
      lcd.print(mandala_num_strings);
      
      lcd.setCursor(0, 3);
      lcd.print("Length: ");
      lcd.print(round(mandala_overall_length * (frame_diameter/1000.0)));
      lcd.print("m|");
      lcd.print(mandala_overall_length);
      lcd.print("m");
   }
   else if(state == STATE_CREATE_MANDALA)
   {      
      if(DEBUG) Serial.println("State: Create Mandala.");
      configureRotary(&menu_index, INT, 1, 0, 3, updateCursorLCD);
      lcd.clear();
      lcd.setCursor(20-3, 0);
      lcd.print("#CM");

      new_modulo = modulo;
      new_times_msi = (int) times;
      new_times_lsi = (times-new_times_msi) * 1000.0;
      new_optimization = optimization;

      lcd.setCursor(1,0);
      lcd.print("Modulo: ");
      lcd.print(new_modulo);
      
      lcd.setCursor(1,1);
      lcd.print("Times: ");
      updateTimesMsiLCD();
      updateTimesLsiLCD();
      
      lcd.setCursor(1,2);
      lcd.print("Opt.: ");
      updateOptimizationLCD();
      
      lcd.setCursor(1,3);
      lcd.print("Create...");
   }
   
}
