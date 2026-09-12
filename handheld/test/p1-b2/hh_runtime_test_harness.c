#include <jni.h>
#include <android/log.h>
#include <sys/time.h>

#include <stdio.h>
#include <string.h>

#include "handheld/runtime/hh_runtime.h"

#define HH_P1B2_TAG "HH_P1B2"
#define HH_P1B2_LINE_MAX 1024

static bool hh_p1b2_callback_installed;

static unsigned long long hh_p1b2_timestamp_ms(void)
{
   struct timeval tv;
   gettimeofday(&tv, NULL);
   return (unsigned long long)tv.tv_sec * 1000ULL
      + (unsigned long long)(tv.tv_usec / 1000);
}

static const char *hh_p1b2_event_command(hh_event_type_t type)
{
   switch (type)
   {
      case HH_EVENT_PAUSED:             return "PAUSE";
      case HH_EVENT_RESUMED:            return "RESUME";
      case HH_EVENT_RESET:              return "RESET";
      case HH_EVENT_CONTENT_CLOSED:     return "CLOSE_CONTENT";
      case HH_EVENT_STATE_SLOT_CHANGED: return "SET_STATE_SLOT";
      case HH_EVENT_STATE_SAVE_ACCEPTED: return "SAVE_STATE";
      case HH_EVENT_STATE_LOAD_ACCEPTED: return "LOAD_STATE";
      case HH_EVENT_SCREENSHOT_TAKEN:   return "SCREENSHOT";
      case HH_EVENT_RA_MENU_OPENED:     return "OPEN_RA_MENU";
      case HH_EVENT_RA_MENU_CLOSED:     return "OPEN_RA_MENU";
      default:                          return "UNKNOWN";
   }
}

static void hh_p1b2_log_event(const hh_runtime_event_t *event)
{
   if (!event)
      return;
   __android_log_print(ANDROID_LOG_INFO, HH_P1B2_TAG,
         "timestamp_ms=%llu stage=COMPLETED request_id=%llu command=%s "
         "event=%s result=%s runtime_mode=%s initialized=%d "
         "content_loaded=%d paused=%d ra_menu_open=%d state_slot=%d",
         hh_p1b2_timestamp_ms(),
         (unsigned long long)event->request_id,
         hh_p1b2_event_command(event->type),
         hh_event_type_to_string(event->type),
         hh_result_to_string(event->result),
         hh_runtime_mode_to_string(event->snapshot.runtime_mode),
         event->snapshot.initialized ? 1 : 0,
         event->snapshot.content_loaded ? 1 : 0,
         event->snapshot.paused ? 1 : 0,
         event->snapshot.ra_menu_open ? 1 : 0,
         event->snapshot.state_slot);
}

static hh_result_t hh_p1b2_ensure_initialized(void)
{
   hh_result_t result = hh_runtime_init();
   if (result != HH_OK)
      return result;
   if (!hh_p1b2_callback_installed)
   {
      result = hh_runtime_set_event_callback(hh_p1b2_log_event, NULL);
      if (result == HH_OK)
         hh_p1b2_callback_installed = true;
   }
   return result;
}

static jstring hh_p1b2_new_string(JNIEnv *env, const char *text)
{
   return (*env)->NewStringUTF(env, text ? text : "");
}

static jstring hh_p1b2_snapshot(JNIEnv *env)
{
   char line[HH_P1B2_LINE_MAX];
   hh_runtime_snapshot_t snapshot;
   hh_result_t result = hh_runtime_get_snapshot(&snapshot);

   if (result != HH_OK)
   {
      snprintf(line, sizeof(line),
            "timestamp_ms=%llu stage=SNAPSHOT result=%s",
            hh_p1b2_timestamp_ms(), hh_result_to_string(result));
      __android_log_print(ANDROID_LOG_INFO, HH_P1B2_TAG, "%s", line);
      return hh_p1b2_new_string(env, line);
   }

   snprintf(line, sizeof(line),
         "timestamp_ms=%llu stage=SNAPSHOT result=%s runtime_mode=%s "
         "initialized=%d content_loaded=%d paused=%d ra_menu_open=%d "
         "state_slot=%d content=%s core=%s core_version=%s "
         "operation_busy=NOT_EXPOSED_PUBLIC_API capabilities="
         "pause:%d,reset:%d,save:%d,load:%d,screenshot:%d,ra_menu:%d",
         hh_p1b2_timestamp_ms(), hh_result_to_string(result),
         hh_runtime_mode_to_string(snapshot.runtime_mode),
         snapshot.initialized ? 1 : 0,
         snapshot.content_loaded ? 1 : 0,
         snapshot.paused ? 1 : 0,
         snapshot.ra_menu_open ? 1 : 0,
         snapshot.state_slot,
         snapshot.content_name,
         snapshot.core_name,
         snapshot.core_version,
         hh_runtime_has_capability(HH_CAP_PAUSE) ? 1 : 0,
         hh_runtime_has_capability(HH_CAP_RESET) ? 1 : 0,
         hh_runtime_has_capability(HH_CAP_SAVE_STATE) ? 1 : 0,
         hh_runtime_has_capability(HH_CAP_LOAD_STATE) ? 1 : 0,
         hh_runtime_has_capability(HH_CAP_SCREENSHOT) ? 1 : 0,
         hh_runtime_has_capability(HH_CAP_RA_MENU) ? 1 : 0);
   __android_log_print(ANDROID_LOG_INFO, HH_P1B2_TAG, "%s", line);
   return hh_p1b2_new_string(env, line);
}

static bool hh_p1b2_parse_command(const char *name,
      hh_command_type_t *type)
{
   if (!strcmp(name, "pause"))
      *type = HH_CMD_PAUSE;
   else if (!strcmp(name, "resume"))
      *type = HH_CMD_RESUME;
   else if (!strcmp(name, "reset"))
      *type = HH_CMD_RESET;
   else if (!strcmp(name, "close_content"))
      *type = HH_CMD_CLOSE_CONTENT;
   else if (!strcmp(name, "set_slot"))
      *type = HH_CMD_SET_STATE_SLOT;
   else if (!strcmp(name, "save"))
      *type = HH_CMD_SAVE_STATE;
   else if (!strcmp(name, "load"))
      *type = HH_CMD_LOAD_STATE;
   else if (!strcmp(name, "screenshot"))
      *type = HH_CMD_SCREENSHOT;
   else if (!strcmp(name, "open_ra_menu"))
      *type = HH_CMD_OPEN_RA_MENU;
   else
      return false;
   return true;
}

JNIEXPORT jstring JNICALL
Java_com_retroarch_p1b2_P1B2HarnessReceiver_nativeInit(
      JNIEnv *env, jclass clazz)
{
   char line[HH_P1B2_LINE_MAX];
   hh_result_t result;
   (void)clazz;

   result = hh_p1b2_ensure_initialized();
   snprintf(line, sizeof(line), "timestamp_ms=%llu stage=INIT result=%s",
         hh_p1b2_timestamp_ms(), hh_result_to_string(result));
   __android_log_print(ANDROID_LOG_INFO, HH_P1B2_TAG, "%s", line);
   return hh_p1b2_new_string(env, line);
}

JNIEXPORT jstring JNICALL
Java_com_retroarch_p1b2_P1B2HarnessReceiver_nativeSnapshot(
      JNIEnv *env, jclass clazz)
{
   (void)clazz;
   return hh_p1b2_snapshot(env);
}

JNIEXPORT jstring JNICALL
Java_com_retroarch_p1b2_P1B2HarnessReceiver_nativeCommand(
      JNIEnv *env, jclass clazz, jstring command, jint int_arg)
{
   char line[HH_P1B2_LINE_MAX];
   const char *name;
   hh_command_type_t type;
   hh_result_t result;
   uint64_t request_id = 0;
   (void)clazz;

   if (!command)
      return hh_p1b2_new_string(env, "stage=SUBMIT result=INVALID_ARGUMENT");

   name = (*env)->GetStringUTFChars(env, command, NULL);
   if (!name)
      return hh_p1b2_new_string(env, "stage=SUBMIT result=INTERNAL");

   result = hh_p1b2_ensure_initialized();
   if (result == HH_OK && !hh_p1b2_parse_command(name, &type))
      result = HH_ERR_INVALID_ARGUMENT;
   if (result == HH_OK)
      result = hh_runtime_submit_command(type, (int)int_arg, &request_id);

   snprintf(line, sizeof(line),
         "timestamp_ms=%llu stage=SUBMITTED request_id=%llu command=%s "
         "int_arg=%d result=%s",
         hh_p1b2_timestamp_ms(), (unsigned long long)request_id,
         name, (int)int_arg, hh_result_to_string(result));
   __android_log_print(ANDROID_LOG_INFO, HH_P1B2_TAG, "%s", line);
   (*env)->ReleaseStringUTFChars(env, command, name);
   return hh_p1b2_new_string(env, line);
}
