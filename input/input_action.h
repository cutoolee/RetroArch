#ifndef INPUT_ACTION_H
#define INPUT_ACTION_H

static unsigned input_action_step(unsigned mask, unsigned period,
      int pressed, unsigned *phase)
{
   unsigned result;
   if (!pressed || !mask || mask > 0xffff || period > 60 || period == 1)
   {
      *phase = 0;
      return 0;
   }
   if (!period)
   {
      *phase = 0;
      return mask;
   }
   result = *phase < period / 2 ? mask : 0;
   *phase = (*phase + 1) % period;
   return result;
}

#endif
