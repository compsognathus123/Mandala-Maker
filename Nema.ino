
void doStep(boolean dir, float velocity)
{   
   digitalWrite(NEMA_DIR, dir);
   digitalWrite(NEMA_STEP, HIGH);
   delayMicroseconds(500);
   digitalWrite(NEMA_STEP, LOW);
   delayMicroseconds(500);
   
   delayMicroseconds(1/velocity * 3000);  
}

void doSteps(boolean dir, int n, float velocity)
{
   for(int i = 0; i < n; i++)
   {
      doStep(dir, velocity);
   }
}

void setMicrostepping(boolean ms)
{
   digitalWrite(NEMA_MS, ms);
}

void enableNema(boolean enabled)
{
   digitalWrite(NEMA_ENABLE, !enabled);
}

//Apply offset or remove offset
void doWeaverLaserAdjustment(boolean adjust)
{
   if(steps_laser_weaver < 0) 
   {
      int positive = sqrt(pow(steps_laser_weaver, 2));
      doSteps(!adjust, positive, VEL_MS);
   }
   else
   {
      doSteps(adjust, steps_laser_weaver, VEL_MS);
   }
}
