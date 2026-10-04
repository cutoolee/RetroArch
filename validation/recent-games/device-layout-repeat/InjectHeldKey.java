import android.os.SystemClock;
import android.view.InputDevice;
import android.view.InputEvent;
import android.view.KeyEvent;
import java.lang.reflect.Method;

public final class InjectHeldKey {
  public static void main(String[] args) throws Exception {
    int key = Integer.parseInt(args[0]);
    long duration = Long.parseLong(args[1]);
    int device = Integer.parseInt(args[2]);
    Class<?> type = Class.forName("android.hardware.input.InputManager");
    Object manager = type.getMethod("getInstance").invoke(null);
    Method inject = type.getMethod("injectInputEvent", InputEvent.class, int.class);
    long start = SystemClock.uptimeMillis();
    KeyEvent down = new KeyEvent(start, start, KeyEvent.ACTION_DOWN,
        key, 0, 0, device, 0, 0, InputDevice.SOURCE_GAMEPAD);
    if (!((Boolean) inject.invoke(manager, down, 2)))
      throw new IllegalStateException("Key down rejected");
    try {
      SystemClock.sleep(duration);
    } finally {
      KeyEvent up = new KeyEvent(start, SystemClock.uptimeMillis(),
          KeyEvent.ACTION_UP, key, 0, 0, device, 0, 0, InputDevice.SOURCE_GAMEPAD);
      if (!((Boolean) inject.invoke(manager, up, 2)))
        throw new IllegalStateException("Key up rejected");
    }
    System.out.println("HELD_KEY=" + key + " DURATION_MS=" + duration);
  }
}
