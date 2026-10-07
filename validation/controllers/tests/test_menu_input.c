#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "input/input_driver.h"
#include "menu/menu_driver.h"
#include "handheld/bridge/hh_bridge.h"

static settings_t settings;
static struct menu_state menu_st;
static bool handheld_open;
static unsigned buttons, keyboard_key;

settings_t *config_get_ptr(void) { return &settings; }
struct menu_state *menu_state_get_ptr(void) { return &menu_st; }
bool menu_input_dialog_get_display_kb(void) { return false; }
void menu_entry_get(menu_entry_t *entry, size_t stack, size_t index, void *userdata, bool use_representation)
{ (void)stack; (void)index; (void)userdata; (void)use_representation; memset(entry, 0, sizeof(*entry)); }
hh_bridge_t *hh_bridge_active(void) { return NULL; }
bool hh_bridge_is_open(const hh_bridge_t *bridge)
{ (void)bridge; return handheld_open; }

static int16_t test_input(void *data, const input_device_driver_t *joypad,
      const input_device_driver_t *secondary, rarch_joypad_info_t *info,
      const retro_keybind_set *binds, bool blocked, unsigned port,
      unsigned device, unsigned index, unsigned id)
{
   (void)data; (void)joypad; (void)secondary; (void)info;
   (void)binds; (void)blocked; (void)index;
   if (port)
      return 0;
   if (device == RETRO_DEVICE_KEYBOARD)
      return id == keyboard_key && keyboard_key != 0;
   if (device == RETRO_DEVICE_JOYPAD)
      return id == RETRO_DEVICE_ID_JOYPAD_MASK ? (int16_t)buttons :
         id < 16 && (buttons & (1U << id)) ? 1 : 0;
   return 0;
}

int main(void)
{
   input_driver_t driver;
   input_driver_state_t *input_st = input_state_get_ptr();
   input_bits_t bits;
   unsigned i, frontend;
   memset(&driver, 0, sizeof(driver));
   driver.input_state = test_input;
   input_st->current_driver = &driver;
   settings.uints.input_max_users = 1;
   settings.bools.input_remap_binds_enable = true;
   for (i = 0; i < RARCH_BIND_LIST_END; i++)
   {
      input_config_binds[0][i].attr = RETRO_KEYBIND_VALID_BIT;
      input_config_binds[0][i].joykey = NO_BTN;
      input_config_binds[0][i].joyaxis = AXIS_NONE;
      input_autoconf_binds[0][i].joykey = NO_BTN;
      input_autoconf_binds[0][i].joyaxis = AXIS_NONE;
   }
   settings.uints.input_remap_ids[0][RETRO_DEVICE_ID_JOYPAD_A] = RETRO_DEVICE_ID_JOYPAD_B;
   settings.uints.input_action_mask[0][RETRO_DEVICE_ID_JOYPAD_A] =
      (1U << RETRO_DEVICE_ID_JOYPAD_B) | (1U << RETRO_DEVICE_ID_JOYPAD_X);
   settings.uints.input_action_period[0][RETRO_DEVICE_ID_JOYPAD_A] = 6;
   BIT256_SET(input_st->mapper.buttons[0], RETRO_DEVICE_ID_JOYPAD_B);
   for (frontend = 0; frontend < 4; frontend++)
   {
      handheld_open = (frontend & 1) == 0;
      settings.bools.input_menu_swap_ok_cancel_buttons = frontend >= 2;
      settings.bools.menu_unified_controls = handheld_open;
      menu_st.flags = handheld_open ? 0 : MENU_ST_FLAG_ALIVE;
      buttons = 1U << RETRO_DEVICE_ID_JOYPAD_A;
      BIT256_CLEAR_ALL_PTR(&bits);
      input_driver_collect_system_input(input_st, &settings, &bits);
      assert(BIT256_GET(bits, RETRO_DEVICE_ID_JOYPAD_A));
      assert(!BIT256_GET(bits, RETRO_DEVICE_ID_JOYPAD_B));
      assert(!BIT256_GET(bits, RETRO_DEVICE_ID_JOYPAD_X));
      buttons = 1U << RETRO_DEVICE_ID_JOYPAD_B;
      BIT256_CLEAR_ALL_PTR(&bits);
      input_driver_collect_system_input(input_st, &settings, &bits);
      assert(BIT256_GET(bits, RETRO_DEVICE_ID_JOYPAD_B));
      assert(!BIT256_GET(bits, RETRO_DEVICE_ID_JOYPAD_A));
      buttons = 0;
      keyboard_key = RETROK_RETURN;
      BIT256_CLEAR_ALL_PTR(&bits);
      input_driver_collect_system_input(input_st, &settings, &bits);
      assert(BIT256_GET(bits, frontend >= 2 ?
               RETRO_DEVICE_ID_JOYPAD_B : RETRO_DEVICE_ID_JOYPAD_A));
      keyboard_key = RETROK_BACKSPACE;
      BIT256_CLEAR_ALL_PTR(&bits);
      input_driver_collect_system_input(input_st, &settings, &bits);
      assert(BIT256_GET(bits, frontend >= 2 ?
               RETRO_DEVICE_ID_JOYPAD_A : RETRO_DEVICE_ID_JOYPAD_B));
      if (handheld_open)
      {
         keyboard_key = RETROK_ESCAPE;
         BIT256_CLEAR_ALL_PTR(&bits);
         input_driver_collect_system_input(input_st, &settings, &bits);
         assert(BIT256_GET(bits, frontend >= 2 ?
                  RETRO_DEVICE_ID_JOYPAD_A : RETRO_DEVICE_ID_JOYPAD_B));
      }
      keyboard_key = 0;
   }
   puts("PASS: GameGo and RetroArch menus share OK/cancel setting and keep raw input with remap/turbo/combinations");
   return 0;
}
