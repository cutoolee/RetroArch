package com.retroarch.browser.retroactivity;

import android.app.Activity;
import android.graphics.Bitmap;
import android.media.MediaMetadataRetriever;
import android.media.MediaPlayer;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.Gravity;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;

import java.io.File;
import java.io.FileOutputStream;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

final class GameGoRecentVideo implements SurfaceHolder.Callback {
  private final Activity activity;
  private final Handler handler = new Handler(Looper.getMainLooper());
  private final ExecutorService thumbnails = Executors.newSingleThreadExecutor();
  private final Map<String, String> thumbnailKeys = new HashMap<>();
  private final Set<String> pendingThumbnails = new HashSet<>();
  private final Set<String> failedThumbnails = new HashSet<>();
  private SurfaceView view;
  private String displayedPath = "";
  private SurfaceHolder activeHolder;
  private MediaPlayer player;
  private String requestedPath = "";
  private volatile String failedPath = "";
  private int x, y, width, height, frameWidth, frameHeight;
  private int videoWidth, videoHeight;
  private int panelX, panelY, panelWidth, panelHeight;
  private boolean suspended;
  private boolean destroyed;

  GameGoRecentVideo(Activity activity) {
    this.activity = activity;
  }

  synchronized String thumbnail(final String path) {
    if (destroyed || path == null || path.isEmpty())
      return null;
    final File source = new File(path);
    if (!source.isFile())
      return null;
    final String signature = path + "\n" + source.lastModified() + "\n" + source.length();
    String cachedKey = thumbnailKeys.get(signature);
    if (cachedKey == null) {
      try {
        byte[] digest = MessageDigest.getInstance("SHA-256").digest(
            signature.getBytes(StandardCharsets.UTF_8));
        StringBuilder name = new StringBuilder();
        for (byte value : digest)
          name.append(String.format("%02x", value & 0xff));
        cachedKey = name.toString();
        if (thumbnailKeys.size() >= 256)
          thumbnailKeys.clear();
        thumbnailKeys.put(signature, cachedKey);
      } catch (Exception error) {
        return null;
      }
    }
    final String key = cachedKey;
    final File directory = new File(activity.getCacheDir(), "gamego-video-preview");
    final File target = new File(directory, key + ".png");
    if (target.isFile())
      return target.getAbsolutePath();
    if (failedThumbnails.contains(key))
      return null;
    if (pendingThumbnails.add(key)) {
      thumbnails.execute(new Runnable() {
        @Override
        public void run() {
          MediaMetadataRetriever retriever = new MediaMetadataRetriever();
          Bitmap frame = null;
          File temporary = null;
          boolean saved = false;
          try {
            retriever.setDataSource(path);
            frame = retriever.getFrameAtTime(0, MediaMetadataRetriever.OPTION_CLOSEST);
            if (frame != null && (directory.isDirectory() || directory.mkdirs())) {
              int longest = Math.max(frame.getWidth(), frame.getHeight());
              if (longest > 512) {
                Bitmap scaled = Bitmap.createScaledBitmap(frame,
                    Math.max(1, frame.getWidth() * 512 / longest),
                    Math.max(1, frame.getHeight() * 512 / longest), true);
                if (scaled != frame)
                  frame.recycle();
                frame = scaled;
              }
              temporary = File.createTempFile(key, ".tmp", directory);
              try (FileOutputStream output = new FileOutputStream(temporary)) {
                saved = frame.compress(Bitmap.CompressFormat.PNG, 100, output);
              }
              synchronized (GameGoRecentVideo.this) {
                saved = saved && !destroyed && temporary.renameTo(target);
              }
            }
          } catch (Exception error) {
            Log.w("GAMEGO_RECENT", "VIDEO_FRAME_FAILED " + path, error);
          } finally {
            if (frame != null)
              frame.recycle();
            try {
              retriever.release();
            } catch (Exception ignored) {
            }
            if (temporary != null)
              temporary.delete();
            synchronized (GameGoRecentVideo.this) {
              pendingThumbnails.remove(key);
              if (!saved)
                failedThumbnails.add(key);
            }
          }
        }
      });
    }
    return "";
  }

  synchronized boolean set(String path, int x, int y, int width, int height,
      int frameWidth, int frameHeight) {
    if (destroyed)
      return false;
    if (path == null)
      path = "";
    if (!path.equals(requestedPath) || x != this.x || y != this.y
        || width != this.width || height != this.height
        || frameWidth != this.frameWidth || frameHeight != this.frameHeight) {
      if (!path.equals(requestedPath))
        failedPath = "";
      requestedPath = path;
      this.x = x;
      this.y = y;
      this.width = width;
      this.height = height;
      this.frameWidth = frameWidth;
      this.frameHeight = frameHeight;
      handler.removeCallbacks(update);
      handler.post(update);
    }
    return !path.isEmpty() && !path.equals(failedPath);
  }

  private final Runnable update = new Runnable() {
    @Override
    public void run() {
      synchronized (GameGoRecentVideo.this) {
        if (destroyed || suspended || requestedPath.isEmpty()
            || requestedPath.equals(failedPath) || frameWidth <= 0 || frameHeight <= 0) {
          removeView();
          return;
        }
        boolean reuse = view != null && requestedPath.equals(displayedPath);
        if (!reuse) {
          removeView();
          displayedPath = requestedPath;
          view = new SurfaceView(activity);
          view.setZOrderOnTop(true);
          view.setFocusable(false);
          view.setClickable(false);
          view.getHolder().addCallback(GameGoRecentVideo.this);
        }
        View content = activity.findViewById(android.R.id.content);
        float sx = (float) content.getWidth() / frameWidth;
        float sy = (float) content.getHeight() / frameHeight;
        FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(
            Math.max(1, Math.round(width * sx)), Math.max(1, Math.round(height * sy)),
            Gravity.TOP | Gravity.LEFT);
        params.leftMargin = Math.round(x * sx);
        params.topMargin = Math.round(y * sy);
        panelX = params.leftMargin;
        panelY = params.topMargin;
        panelWidth = params.width;
        panelHeight = params.height;
        if (reuse) {
          view.setLayoutParams(params);
          fitVideo();
        } else {
          activity.addContentView(view, params);
        }
      }
    }
  };

  private void removeView() {
    releasePlayer();
    displayedPath = "";
    if (view != null) {
      ViewGroup parent = (ViewGroup) view.getParent();
      if (parent != null)
        parent.removeView(view);
      view = null;
    }
  }

  private void releasePlayer() {
    if (player != null) {
      player.release();
      player = null;
    }
    videoWidth = videoHeight = 0;
    activeHolder = null;
  }

  private void fitVideo() {
    if (view == null || videoWidth <= 0 || videoHeight <= 0)
      return;
    float scale = Math.min((float) panelWidth / videoWidth,
        (float) panelHeight / videoHeight);
    FrameLayout.LayoutParams params = (FrameLayout.LayoutParams) view.getLayoutParams();
    params.width = Math.max(1, Math.round(videoWidth * scale));
    params.height = Math.max(1, Math.round(videoHeight * scale));
    params.leftMargin = panelX + (panelWidth - params.width) / 2;
    params.topMargin = panelY + (panelHeight - params.height) / 2;
    view.setLayoutParams(params);
  }

  @Override
  public synchronized void surfaceCreated(SurfaceHolder holder) {
    if (destroyed || suspended || view == null || view.getHolder() != holder)
      return;
    final String path = displayedPath;
    try {
      activeHolder = holder;
      player = new MediaPlayer();
      player.setDisplay(holder);
      player.setVolume(0, 0);
      player.setLooping(true);
      player.setOnPreparedListener(new MediaPlayer.OnPreparedListener() {
        @Override
        public void onPrepared(MediaPlayer ready) {
          synchronized (GameGoRecentVideo.this) {
            if (ready != player || suspended || destroyed)
              return;
            videoWidth = ready.getVideoWidth();
            videoHeight = ready.getVideoHeight();
            fitVideo();
            ready.start();
            Log.i("GAMEGO_RECENT", "VIDEO_PLAYING " + path);
          }
        }
      });
      player.setOnErrorListener(new MediaPlayer.OnErrorListener() {
        @Override
        public boolean onError(MediaPlayer failed, int what, int extra) {
          synchronized (GameGoRecentVideo.this) {
            if (failed == player) {
              failedPath = path;
              releasePlayer();
              if (view != null)
                view.setVisibility(View.GONE);
              Log.w("GAMEGO_RECENT", "VIDEO_FAILED " + path);
            }
          }
          return true;
        }
      });
      player.setDataSource(path);
      player.prepareAsync();
    } catch (Exception error) {
      failedPath = path;
      releasePlayer();
      view.setVisibility(View.GONE);
      Log.w("GAMEGO_RECENT", "VIDEO_FAILED " + path, error);
    }
  }

  @Override
  public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
  }

  @Override
  public synchronized void surfaceDestroyed(SurfaceHolder holder) {
    if (activeHolder == holder)
      releasePlayer();
  }

  synchronized void pause() {
    suspended = true;
    handler.removeCallbacks(update);
    removeView();
  }

  synchronized void resume() {
    suspended = false;
    handler.post(update);
  }

  synchronized void destroy() {
    destroyed = true;
    handler.removeCallbacks(update);
    thumbnails.shutdownNow();
    removeView();
  }
}
