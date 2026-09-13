#include <jni.h>
#include <android/log.h>

#include <stdio.h>
#include <string.h>

#include "handheld/bridge/hh_bridge.h"
#include "handheld/runtime/hh_runtime.h"

#define HH_GAMEGO_E2E_TAG "GAMEGO_E2E"
#define HH_GAMEGO_E2E_LINE_MAX 2048

static const char *hh_gamego_e2e_page(hh_quick_menu_page_t page)
{
   switch (page)
   {
      case HH_QUICK_MENU_PAGE_SAVE: return "SAVE";
      case HH_QUICK_MENU_PAGE_LOAD: return "LOAD";
      default: return "MAIN";
   }
}

static const char *hh_gamego_e2e_state(hh_quick_menu_state_t state)
{
   switch (state)
   {
      case HH_QUICK_MENU_OPENING: return "OPENING";
      case HH_QUICK_MENU_OPEN: return "OPEN";
      case HH_QUICK_MENU_CONFIRM_DIALOG: return "CONFIRM_DIALOG";
      case HH_QUICK_MENU_ACTION_PENDING: return "ACTION_PENDING";
      case HH_QUICK_MENU_CLOSING: return "CLOSING";
      default: return "CLOSED";
   }
}

static const char *hh_gamego_e2e_action(hh_ui_action_t action)
{
   return hh_ui_action_to_string(action);
}

static jstring hh_gamego_e2e_status(JNIEnv *env)
{
   char line[HH_GAMEGO_E2E_LINE_MAX];
   hh_runtime_snapshot_t runtime;
#ifdef HAVE_GAMEGO_E2E_HARNESS
   hh_runtime_e2e_state_io_observation_t io;
#endif
   hh_bridge_t *bridge = hh_bridge_active();
   const hh_quick_menu_t *menu;
   const hh_quick_menu_view_t *view;

   if (!bridge || hh_runtime_get_snapshot(&runtime) != HH_OK)
      return (*env)->NewStringUTF(env, "CONTENT_STATE=UNKNOWN RUNTIME_STATE=UNINITIALIZED");
   menu = hh_bridge_menu(bridge);
   view = hh_quick_menu_get_view(menu);
#ifdef HAVE_GAMEGO_E2E_HARNESS
   memset(&io, 0, sizeof(io));
   hh_runtime_e2e_get_state_io_observation(&io);
#endif
   snprintf(line, sizeof(line),
         "CONTENT_STATE=%s QUICK_MENU_OPEN=%s QUICK_MENU_PAGE=%s "
         "QUICK_MENU_SELECTION=%u QUICK_MENU_BUSY=%s RUNTIME_STATE=%s "
         "PAUSED=%s PENDING_ACTION=%s PENDING_REQUEST_ID=%llu "
         "MENU_STATE=%s CAP_SAVE=%s CAP_LOAD=%s SLOT0_OCCUPIED=%s "
         "FEEDBACK=%u MESSAGE=%s BRIDGE_PENDING_ACTION=%s "
         "BRIDGE_PENDING_REQUEST_ID=%llu "
         "SAVE_SUBMIT_REQUEST_ID=%llu SAVE_ACCEPTED_REQUEST_ID=%llu "
         "SAVE_COMPLETED_REQUEST_ID=%llu SAVE_GENERATION=%llu "
         "LOAD_SUBMIT_REQUEST_ID=%llu LOAD_ACCEPTED_REQUEST_ID=%llu "
         "LOAD_COMPLETED_REQUEST_ID=%llu LOAD_GENERATION=%llu",
         runtime.content_loaded ? "RUNNING" : "NO_CONTENT",
         hh_bridge_is_open(bridge) ? "YES" : "NO",
         view ? hh_gamego_e2e_page(view->page) : "MAIN",
         view ? (unsigned)view->selected_index : 0U,
         view && view->busy ? "YES" : "NO",
         hh_runtime_mode_to_string(runtime.runtime_mode),
         runtime.paused ? "YES" : "NO",
         view ? hh_gamego_e2e_action(view->pending_action) : "NONE",
         (unsigned long long)bridge->pending_request_id,
         menu ? hh_gamego_e2e_state(hh_quick_menu_get_state(menu)) : "CLOSED",
         hh_runtime_has_capability(HH_CAP_SAVE_STATE) ? "YES" : "NO",
         hh_runtime_has_capability(HH_CAP_LOAD_STATE) ? "YES" : "NO",
         view && view->slots[0].occupied ? "YES" : "NO",
         view ? (unsigned)view->feedback : 0U,
         view ? view->message : "",
         hh_gamego_e2e_action(bridge->pending_action),
         (unsigned long long)bridge->pending_request_id,
         (unsigned long long)io.save_submit_request_id,
         (unsigned long long)io.save_accepted_request_id,
         (unsigned long long)io.save_completed_request_id,
         (unsigned long long)io.save_generation,
         (unsigned long long)io.load_submit_request_id,
         (unsigned long long)io.load_accepted_request_id,
         (unsigned long long)io.load_completed_request_id,
         (unsigned long long)io.load_generation);
   __android_log_print(ANDROID_LOG_INFO, HH_GAMEGO_E2E_TAG, "%s", line);
   return (*env)->NewStringUTF(env, line);
}

static bool hh_gamego_e2e_input(const char *command,
      hh_quick_menu_input_t *input)
{
   if (!strcmp(command, "up")) *input = HH_QUICK_MENU_INPUT_UP;
   else if (!strcmp(command, "down")) *input = HH_QUICK_MENU_INPUT_DOWN;
   else if (!strcmp(command, "left")) *input = HH_QUICK_MENU_INPUT_LEFT;
   else if (!strcmp(command, "right")) *input = HH_QUICK_MENU_INPUT_RIGHT;
   else if (!strcmp(command, "confirm")) *input = HH_QUICK_MENU_INPUT_CONFIRM;
   else if (!strcmp(command, "back")) *input = HH_QUICK_MENU_INPUT_BACK;
   else return false;
   return true;
}

JNIEXPORT jstring JNICALL
Java_com_retroarch_gamego_GameGoE2eHarnessReceiver_nativeCommand(
      JNIEnv *env, jclass clazz, jstring command_obj)
{
   const char *command;
   hh_bridge_t *bridge;
   hh_quick_menu_input_t input;
   (void)clazz;

   if (!command_obj)
      return (*env)->NewStringUTF(env, "RESULT=INVALID_ARGUMENT");
   command = (*env)->GetStringUTFChars(env, command_obj, NULL);
   if (!command)
      return (*env)->NewStringUTF(env, "RESULT=INTERNAL");

   bridge = hh_bridge_active();
   if (!strcmp(command, "status"))
   {
      (*env)->ReleaseStringUTFChars(env, command_obj, command);
      return hh_gamego_e2e_status(env);
   }
   if (!bridge)
   {
      (*env)->ReleaseStringUTFChars(env, command_obj, command);
      return (*env)->NewStringUTF(env, "RESULT=BRIDGE_UNAVAILABLE");
   }
   if (!strcmp(command, "menu"))
      hh_bridge_test_toggle(bridge);
   else if (hh_gamego_e2e_input(command, &input))
      hh_bridge_test_input(bridge, input);
   else
   {
      (*env)->ReleaseStringUTFChars(env, command_obj, command);
      return (*env)->NewStringUTF(env, "RESULT=INVALID_ARGUMENT");
   }
   (*env)->ReleaseStringUTFChars(env, command_obj, command);
   return hh_gamego_e2e_status(env);
}
