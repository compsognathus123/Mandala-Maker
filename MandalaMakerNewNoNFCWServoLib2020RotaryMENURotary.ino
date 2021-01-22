#include <Wire.h>
//#include <Adafruit_PN532.h>
//#include <Adafruit_CAP1188.h>
#include <FRAM_MB85RC_I2C.h>
#include <Servo.h>
#include <LiquidCrystal_I2C.h>
//#include <Math.h>

#define DEBUG 0
#define DEBUG_MANDALA_FRAM_CREATION 0

//--------------Pins------------------------
//#define NFC_RESET 4
//#define NFC_INTERRUPT 7
//#define CAP_RESET  9 //Remove
//#define CAP_INTERRUPT 1 //Remove
#define PHOTO_INTERRUPT 0
#define LASER 8
#define NEMA_STEP 10
#define NEMA_DIR 14
#define NEMA_MS 15
#define NEMA_ENABLE 16
#define SERVO A0
#define BUTTON_INTERRUPT 7
#define BUTTON_ROTARY 6
#define ROTARYA 1
#define ROTARYB 5

//--------------FRAM-------------------------
#define FRAM_ADR_MANDALA_CREATION 1000    //MANDALA_DATA_LENGTH Byte
#define FRAM_ADR_MODULO 10                //2 Byte
#define FRAM_ADR_TIMES 12                 //4 Byte
#define FRAM_ADR_MANDALA_DATA_LENGTH 16   //2 Byte
#define FRAM_ADR_MANDALA_DATA_VALID 18    //1 Byte
#define FRAM_ADR_STEPS_PHOTOLASER 19      //1 Byte
#define FRAM_ADR_STEPS_HOLE 20            //1 Byte
#define FRAM_ADR_STEPS_LASERWEAVER 21     //2 Byte
#define FRAM_ADR_STEPS_TOOTH 23           //1 Byte
#define FRAM_ADR_STEPS_HOLE_FULL 24       //1 Byte
#define FRAM_ADR_STEPS_TOOTH_FULL 25      //1 Byte
#define FRAM_ADR_MANDALA_FRAM_ADRESS 26   //2 Byte
#define FRAM_ADR_MANDALA_NUM_STRINGS 28   //2 Byte
#define FRAM_ADR_MANDALA_OVERALL_LENGTH 30   //2 Byte
#define FRAM_ADR_AVAILABLE_STRING 32      //2 Byte
#define FRAM_ADR_FRAME_DIA 34             //4 Byte
#define FRAM_ADR_SERVO_HIGH 38            //2 Byte
#define FRAM_ADR_SERVO_LOW 40             //2 Byte
#define FRAM_ADR_CURRENT_STRING 42        //2 Byte
#define FRAM_ADR_OPTIMIZATION 44          //4 Byte
#define FRAM_ADR_PAUSE_STRING 48          //2 BYte
#define MANAGE_WP false

#define SERIAL_DEBUG 0

FRAM_MB85RC_I2C fram(0x50, false, A3);

//-------------LCD------------------------
LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

//---------------Servo-----------------------
#define SERVO_PULSE_LOW 25    //Now in degrees
#define SERVO_PULSE_HIGH 60   //Now in degrees

boolean weaver_state;
int servo_pulse_low;
int servo_pulse_high;
int servo_pulse;
Servo servo;

//---------------CapSense--------------------
//Adafruit_CAP1188 cap = Adafruit_CAP1188(CAP_RESET);
//volatile boolean cap_interrupt;

//---------------Rotary/Buttons----------------
#define BUTTON_OKAY 1
#define BUTTON_BACK 2
#define BUTTON_NONE 0
volatile byte button_interrupt;
volatile boolean rotary_changed;


//---------------Photo------------------------
volatile boolean photo_tooth;
volatile int photo_index;
volatile boolean photo_interrupt;


//---------------General----------------------
#define STATE_SETUP 1
#define STATE_READY 2
#define STATE_WEAVING 3
#define STATE_PAUSED 4
#define STATE_MANDALA 5
#define STATE_CREATE_MANDALA 6
#define STATE_SETTINGS 7
#define STATE_CALIBRATING 8
#define STATE_CALIBRATING_STEPS 9

#define VEL_MS 0.5
#define VEL_NORMAL 1.0
#define VEL_MAX 10.0


long wait_for_serial;

//outdated, unused
unsigned char mandala_data_valid;
int mandala_data_length;
int toofewsteps = 0;
byte steps_photolaser;

//Mandala Variables
int modulo;
float times;
int optimization;  //in mm Normalized with frame_diameter = 1m
int mandala_fram_adress;
int mandala_num_strings;
int mandala_overall_length;   //Normalized with frame_diameter = 1m

//Weaving Variables
int current_string;
byte steps_tooth;
byte steps_hole;
byte steps_tooth_full;
byte steps_hole_full;
int steps_laser_weaver;

int available_string; //m
int frame_diameter; //in mm
int pause_string;

long millis_weaving;
long millis_loop;

boolean weaving_direction = false;
int weaving_index;
float weaving_string_length; //m

//Navigation
byte state;
byte calibrating_mode;

/* TODO/Care 
 * 
 *   Changed: 
 *    FRAM init: load steps_tooth
 *    MoveToHole v += 0.3 instead of 0.2
 *    
 *    7.11.20  disabled NFC: init, pull, handleNFC
 *    8.11.20  Added Servo Lib for continous positioning with timers, use angle(deg) now instead of pulse-duration
 *    17.11.20 Noticed potential FRAM reading bug, getting several -1 readings while weaving. Added readWordOnlyPositive() as a fix.
 *    7.12.20  Added Mandala FRAM creation alogrithm
 *    10.12.20 calculateOverallLength() added in MandalaCreation. Checking if available_string is exceeded while weaving, pausing then.
 *    13.12.20 Added LCD functionality
 *    08.01.21 Change optimization and frame_diameter to int in mm (weaving_string_length remains float?)
 *
 *   TODO:
 *     -Does getStringLength() provide proper values?!
 *     -Check if pause works properly, even with removing frame while being paused.
 *     -Stop weaving_millis while paused.
 *     -Align with center before calibrating laser_offset
 *     -Pause at String X
 *     -Confirm mandala creation, specially optimization
 *     
 */

void setup()
{   
   //SERIAL COM
   wait_for_serial = millis();
   Serial.begin(9600);
   while (!Serial && millis() - wait_for_serial < 3000);      
   Wire.begin();

   lcd.init();
   lcd.clear();
   lcd.backlight();
   lcd.noAutoscroll();
   setState(STATE_SETUP);

   //BUTTON AND ROTARY
   pinMode(ROTARYA, INPUT_PULLUP);
   pinMode(ROTARYB, INPUT_PULLUP);
   pinMode(BUTTON_INTERRUPT, INPUT_PULLUP);
   pinMode(BUTTON_ROTARY, INPUT_PULLUP);
   attachInterrupt(digitalPinToInterrupt(ROTARYA), rotaryISR, FALLING);
   attachInterrupt(digitalPinToInterrupt(BUTTON_INTERRUPT), buttonISR, FALLING);

   //NEMA
   pinMode(NEMA_DIR, OUTPUT);
   pinMode(NEMA_STEP, OUTPUT);
   pinMode(NEMA_ENABLE, OUTPUT);
   pinMode(NEMA_MS, OUTPUT);   
   enableNema(false);

   //SERVO
   //pinMode(SERVO, OUTPUT);  
   lcdPrintSetup("Attaching servo...");
   servo.attach(SERVO); 
   weaver_state = HIGH;
   setWeaver(weaver_state);   

   //FRAM
   delay(100);
   if(DEBUG) Serial.println("---------------Init FRAM------------------------");
   lcdPrintSetup("Init. FRAM...");
   fram.begin();
   
   lcdPrintSetup("Load. FRAM values..");
   loadValuesFromFRAM();

   //NFC
   /*boolean success = false;
   while(!success)
   {
      success = initNFC();      
   }*/

   initLaserPhoto();
   //lcdPrintSetup("Init. CapSense...");
   //initCapSense();   

   if(DEBUG) Serial.println("------------------Setup done.--------------------");
   lcdPrintSetup("Setup done!");

   //Create Mandala
   if(0)
   {
      int cfm_success = createMandala(320, 185, 0.28);
   }
   
   //measureTolerance();
   //printMandala();

   //available_string = 250;
   //frame_diameter = 0.4;
   //current_string = 0;

   setState(STATE_READY);

   

}

void loop(void) 
{
   millis_loop = millis();
   
   //handleCapSense();  
   handleUserInput();
   
   //readMandalaFRAMTest(millis_loop);   
   //handleNFC();
   //test();
   //servotest();

   

}


void measureHoleToothWidth()
{
   digitalWrite(NEMA_ENABLE, LOW); //enable Nema
   
   //---------------Microstepping 16th---------------------
   setMicrostepping(true);
   
   int hole = 0;
   int tooth = 0;
   
   for(int i = 0; i < 10; i++)
   {
      //Align with tooth
      while(!photo_tooth)
      {
         doStep(weaving_direction, VEL_MS);
      }

      //Move at the edge of tooth
      while(photo_tooth)
      {
         tooth++;
         doStep(weaving_direction, VEL_MS);
      }

      //Move through hole
      while(!photo_tooth)
      {
         hole++;
         doStep(weaving_direction, VEL_MS);
      }
   }

   hole /= 10;
   tooth /= 10;
   
   steps_hole = hole;
   steps_tooth = tooth;


   //--------------Full 2nd----------------
   setMicrostepping(false);
   hole = 0;
   tooth = 0;
   
   for(int i = 0; i < 10; i++)
   {
      //Align with tooth
      while(!photo_tooth)
      {
         doStep(weaving_direction, VEL_NORMAL);
      }

      //Move at the edge of tooth
      while(photo_tooth)
      {
         tooth++;
         doStep(weaving_direction, VEL_NORMAL);
      }

      //Move through hole
      while(!photo_tooth)
      {
         hole++;
         doStep(weaving_direction, VEL_NORMAL);
      }
   }

   hole /= 10;
   tooth /= 10;
   
   steps_hole_full = hole;
   steps_tooth_full = tooth;
   
   fram.writeByte(FRAM_ADR_STEPS_HOLE, steps_hole);   
   fram.writeByte(FRAM_ADR_STEPS_TOOTH, steps_tooth);
   fram.writeByte(FRAM_ADR_STEPS_HOLE_FULL, steps_hole_full);   
   fram.writeByte(FRAM_ADR_STEPS_TOOTH_FULL, steps_tooth_full);
   
   if(DEBUG) Serial.print("steps_hole = ");
   if(DEBUG) Serial.println(steps_hole);   
   if(DEBUG) Serial.print("steps_tooth = ");
   if(DEBUG) Serial.println(steps_tooth);   
   if(DEBUG) Serial.print("steps_hole_full = ");
   if(DEBUG) Serial.println(steps_hole_full);   
   if(DEBUG) Serial.print("steps_tooth_full = ");
   if(DEBUG) Serial.println(steps_tooth_full);   

   setMicrostepping(true);
   doSteps(!weaving_direction, steps_hole/2, VEL_MS);

}

void setWeaver(boolean dir)
{   
   if(dir)
   {
      servo_pulse = servo_pulse_high;
   }
   else
   {
      servo_pulse = servo_pulse_low;
   }

   servo.write(servo_pulse);
   delay(750);
   
   /*long duration = millis();

   while(millis() - duration < 750)
   {
      digitalWrite(SERVO, HIGH);
      delayMicroseconds(servo_pulse);
      digitalWrite(SERVO, LOW);
      delay(20);
   }*/
}

void updateWeaver()
{   
   /*long duration = millis();

   while(millis() - duration < 300)
   {
      digitalWrite(SERVO, HIGH);
      delayMicroseconds(servo_pulse);
      digitalWrite(SERVO, LOW);
      delay(15);
   }*/
   
   servo.write(servo_pulse);
   delay(750);
}


bool floatCompare(float f1, float f2)
{
    if (abs(f1 - f2) <= 1.0e-5f)
        return true;
    return abs(f1 - f2) <= 1.0e-5f * max(abs(f1), abs(f2));
}

int32_t bbbbInt(byte b0, byte b1, byte b2, byte b3)
{
   return ((int32_t)((((int32_t)(((int32_t) b0 << 8) | b1) << 8) | b2) << 8)) | b3;
}
    
uint16_t bytebyteInt(byte msb, byte lsb)
{
   return ((uint16_t) msb << 8) | lsb;
}

byte int32(int32_t a, byte index)
{
   return a >> ((3-index) * 8);
}

byte intMSB(int a)
{
   return (a >> 8);
}

byte intLSB(int a)
{
   return (byte)a;
}
