
void initLaserPhoto()
{
   pinMode(LASER, OUTPUT);
   pinMode(PHOTO_INTERRUPT, INPUT);
   
   attachInterrupt(digitalPinToInterrupt(PHOTO_INTERRUPT), photoISR, CHANGE);

   digitalWrite(LASER, HIGH);
}
