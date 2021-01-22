
/*void photoISR()
{
   photo_tooth = !digitalRead(PHOTO_INTERRUPT);
         
   if(!photo_tooth)
   {
      if(weaving_direction)
      {
         photo_index--; //WDIR ++
      }
      else
      {
         photo_index++; //WDIR --
      }

      photo_index = photo_index % modulo;
      if(photo_index < 0) photo_index = modulo-1;
   }
}*/

void photoISR()
{
   photo_tooth = !digitalRead(PHOTO_INTERRUPT);
   photo_interrupt = true;
}
