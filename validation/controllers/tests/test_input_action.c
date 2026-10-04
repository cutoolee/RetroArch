#include <assert.h>
#include <stdio.h>
#include <boolean.h>
#include "input/input_action.h"

int main(void)
{
   unsigned phase[2] = {0, 0};
   unsigned frame, on = 0;
   unsigned abc = (1U << 0) | (1U << 8) | (1U << 9);
   for (frame = 0; frame < 60; frame++)
   {
      unsigned output = input_action_step(abc, 6, true, &phase[0]);
      assert(output == (frame % 6 < 3 ? abc : 0));
      on += output != 0;
      assert(input_action_step(1, 0, true, &phase[1]) == 1);
      assert(phase[1] == 0);
      assert((output | 1) & 1);
   }
   assert(on == 30);
   input_action_step(abc, 6, true, &phase[0]);
   assert(phase[0] == 1);
   assert(input_action_step(abc, 6, false, &phase[0]) == 0);
   assert(phase[0] == 0);
   assert(input_action_step(abc, 6, true, &phase[0]) == abc);
   assert(input_action_step(1, 1, true, &phase[0]) == 0);
   assert(input_action_step(1, 61, true, &phase[0]) == 0);
   assert(input_action_step(0x10000, 0, true, &phase[0]) == 0);
   assert(phase[0] == 0);
   puts("PASS: turbo cadence, simultaneous outputs, independent players and release reset");
   return 0;
}
