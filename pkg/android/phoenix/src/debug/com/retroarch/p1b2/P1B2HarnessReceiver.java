package com.retroarch.p1b2;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

/** Debug-only ADB bridge for the P1-B2 Runtime smoke test. */
public final class P1B2HarnessReceiver extends BroadcastReceiver {
  public static final String ACTION = "com.retroarch.p1b2.COMMAND";
  public static final String EXTRA_COMMAND = "command";
  public static final String EXTRA_INT_ARG = "int_arg";
  private static final String TAG = "HH_P1B2";

  static {
    System.loadLibrary("retroarch-activity");
  }

  private static native String nativeInit();
  private static native String nativeSnapshot();
  private static native String nativeCommand(String command, int intArg);

  @Override
  public void onReceive(Context context, Intent intent) {
    if (intent == null || !ACTION.equals(intent.getAction())) {
      return;
    }

    String command = intent.getStringExtra(EXTRA_COMMAND);
    String result;
    if ("init".equals(command)) {
      result = nativeInit();
    } else if ("snapshot".equals(command)) {
      result = nativeSnapshot();
    } else {
      result = nativeCommand(command, intent.getIntExtra(EXTRA_INT_ARG, 0));
    }
    Log.i(TAG, "HH_P1B2 " + result);
  }
}
