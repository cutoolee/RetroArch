#include "mock_quick_menu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct svg_painter
{
   FILE *file;
   int clip;
} svg_painter_t;

static void escape(FILE *file, const char *text)
{
   for (; *text; text++)
      switch (*text)
      {
         case '&': fputs("&amp;", file); break;
         case '<': fputs("&lt;", file); break;
         case '>': fputs("&gt;", file); break;
         case '"': fputs("&quot;", file); break;
         default: fputc(*text, file); break;
      }
}

static void svg_rect(void *data, hh_ui_rect_t r, unsigned long fill,
      unsigned long border, float radius, float stroke)
{
   svg_painter_t *svg = (svg_painter_t *)data;
   fprintf(svg->file, "<rect x='%g' y='%g' width='%g' height='%g' rx='%g' "
         "fill='#%06lx' fill-opacity='%g' stroke='#%06lx' stroke-opacity='%g' "
         "stroke-width='%g'/>\n", r.x, r.y, r.width, r.height, radius,
         fill >> 8, (fill & 255) / 255.0, border >> 8,
         (border & 255) / 255.0, stroke);
}

static bool svg_controller(void *data, hh_ui_rect_t r)
{
   static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
   svg_painter_t *svg = (svg_painter_t *)data;
   FILE *file = fopen("pkg/android/phoenix/gamego-assets/gamego-defaults/icons/controller-layout-solid.png", "rb");
   int a, b, c;
   if (!file)
      return false;
   fprintf(svg->file, "<image x='%g' y='%g' width='%g' height='%g' href='data:image/png;base64,", r.x, r.y, r.width, r.height);
   while ((a = fgetc(file)) != EOF)
   {
      b = fgetc(file);
      c = b == EOF ? EOF : fgetc(file);
      fputc(alphabet[a >> 2], svg->file);
      fputc(alphabet[((a & 3) << 4) | (b == EOF ? 0 : b >> 4)], svg->file);
      fputc(b == EOF ? '=' : alphabet[((b & 15) << 2) | (c == EOF ? 0 : c >> 6)], svg->file);
      fputc(c == EOF ? '=' : alphabet[c & 63], svg->file);
   }
   fclose(file);
   fputs("'/>", svg->file);
   return true;
}

static void svg_quad(void *data, const float *v,
      unsigned long top, unsigned long bottom)
{
   svg_painter_t *svg = (svg_painter_t *)data;
   int id = svg->clip++;
   fprintf(svg->file, "<defs><linearGradient id='g%d' x1='0' y1='0' x2='0' y2='1'>"
         "<stop stop-color='#%06lx' stop-opacity='%g'/><stop offset='1' "
         "stop-color='#%06lx' stop-opacity='%g'/></linearGradient></defs>"
         "<path d='M%g %gL%g %gL%g %gL%g %gZ' fill='url(#g%d)' shape-rendering='%s'/>\n",
         id, top >> 8, (top & 255) / 255.0, bottom >> 8, (bottom & 255) / 255.0,
         v[0], v[1], v[2], v[3], v[6], v[7], v[4], v[5], id,
         v[0] == v[4] && v[1] == v[5] ? "crispEdges" : "auto");
}

static void svg_text_aligned(void *data, hh_ui_rect_t r, const char *text,
      unsigned long color, float size, bool bold, bool centered)
{
   svg_painter_t *svg = (svg_painter_t *)data;
   int clip = svg->clip++;
   fprintf(svg->file, "<defs><clipPath id='c%d'><rect x='%g' y='%g' "
         "width='%g' height='%g'/></clipPath></defs>\n", clip, r.x, r.y, r.width, r.height);
   fprintf(svg->file, "<text x='%g' y='%g' fill='#%06lx' font-size='%g' "
         "font-weight='%s' font-family='PingFang SC,Noto Sans CJK SC,sans-serif' "
         "text-anchor='%s' clip-path='url(#c%d)'>",
         r.x + (centered ? r.width / 2 : 0), r.y + (r.height + size * 0.72) / 2,
         color >> 8, size, bold ? "600" : "400", centered ? "middle" : "start", clip);
   escape(svg->file, text);
   fputs("</text>\n", svg->file);
}

static void svg_text(void *data, hh_ui_rect_t r, const char *text,
      unsigned long color, float size, bool bold)
{
   svg_text_aligned(data, r, text, color, size, bold, false);
}

static void svg_text_centered(void *data, hh_ui_rect_t r, const char *text,
      unsigned long color, float size, bool bold)
{
   svg_text_aligned(data, r, text, color, size, bold, true);
}

static void svg_icon(void *data, hh_ui_rect_t r,
      hh_quick_menu_icon_t icon, unsigned long color)
{
   static const char *names[] = {"play", "save", "folder-open", "rotate-ccw",
      "sliders-horizontal", "log-out", "chevron-right", "gamepad-2",
      "menu-confirm", "menu-back", "menu-select"};
   svg_painter_t *svg = (svg_painter_t *)data;
   char path[256];
   char source[4096];
   char *body, *end;
   size_t length;
   FILE *file;
   strcpy(path, "handheld/ui/icons/");
   strcat(path, names[icon]);
   strcat(path, ".svg");
   file = fopen(path, "r");
   if (!file) return;
   length = fread(source, 1, sizeof(source) - 1, file);
   fclose(file);
   source[length] = '\0';
   body = strstr(source, "<svg");
   if (!body || !(body = strchr(body, '>'))) return;
   end = strstr(body, "</svg>");
   if (!end) return;
   *end = '\0';
   fprintf(svg->file, "<g transform='translate(%g %g) scale(%g %g)' "
         "fill='none' color='#%06lx' stroke='currentColor' stroke-width='2' "
         "stroke-linecap='round' stroke-linejoin='round'>%s</g>\n",
         r.x, r.y, r.width / 24, r.height / 24, color >> 8, body + 1);
}

static bool svg_preview(void *data, hh_ui_rect_t r, int slot)
{
   hh_ui_rect_t field = r;
   (void)slot;
   svg_rect(data, r, 0x598b85ffUL, 0, 0, 0);
   field.y += r.height * 0.45f;
   field.height = r.height * 0.55f;
   svg_rect(data, field, 0x355b42ffUL, 0, 0, 0);
   field.x += r.width * 0.43f;
   field.width = r.width * 0.15f;
   svg_rect(data, field, 0xc8b589ffUL, 0, 0, 0);
   svg_text(data, r, "MOCK PREVIEW", 0xffffffffUL, r.height * 0.1f, true);
   return true;
}

static bool svg_thumbnail(void *data, hh_ui_rect_t r, const char *path)
{
   (void)path;
   return svg_preview(data, r, 0);
}

static hh_ui_action_t input(hh_quick_menu_t *menu, hh_quick_menu_input_t key)
{
   hh_ui_action_t action = hh_quick_menu_input(menu, key);
   if (action != HH_UI_ACTION_NONE)
      printf("Mock output: %s slot=%d\n", hh_ui_action_to_string(action), menu->view.pending_slot);
   return action;
}

static void scenario(hh_quick_menu_t *menu, int state)
{
   int i;
   mock_quick_menu(menu);
   if (state == 11 || state == 12)
   {
      menu->view.selected_index = HH_QUICK_MENU_ITEM_RECENT;
      input(menu, HH_QUICK_MENU_INPUT_CONFIRM);
      if (state == 11)
      {
         static const char *titles[] = {"拳皇97", "恐龙快打", "合金弹头",
            "魂斗罗", "黄金太阳", "超级马里奥", "塞尔达传说", "口袋妖怪"};
         menu->view.recent_count = 8;
         for (i = 0; i < HH_QUICK_MENU_RECENT_ROWS; i++)
         {
            strcpy(menu->view.recent[i].title, titles[i]);
            strcpy(menu->view.recent[i].thumbnail, "mock.png");
            if (i < 3)
               strcpy(menu->view.recent[i].video, "mock.mp4");
         }
         strcpy(menu->view.recent_thumbnail, "mock.png");
      }
      return;
   }
   if (state == 13 || state == 14)
   {
      static const char *sources[16] = {"A / 下方键", "X / 左侧键", "Select", "Start",
         "方向上", "方向下", "方向左", "方向右", "B / 右侧键", "Y / 上方键",
         "L1", "R1", "L2", "R2", "L3", "R3"};
      menu->view.page = state == 13 ? HH_QUICK_MENU_PAGE_CONTROLS : HH_QUICK_MENU_PAGE_CONTROL_EDIT;
      menu->view.controls_valid = true;
      menu->view.controls.player_count = 4;
      menu->view.controls.device_count = 1;
      strcpy(menu->view.controls.devices[0], "GameGo Controller");
      for (i = 0; i < 16; i++)
      {
         menu->view.controls.available[i] = true;
         menu->view.controls.masks[i] = 1U << i;
         strcpy(menu->view.controls.sources[i], sources[i]);
         strcpy(menu->view.controls.targets[i], sources[i]);
      }
      strcpy(menu->view.controls.targets[0], "B / 下方键");
      menu->view.control_selected = state == 13 ? 2 : 0;
      menu->view.control_mask = 1;
      menu->view.control_period = 6;
      return;
   }
   if (state == 0) return;
   if (state == 1)
   {
      for (i = 0; i < 4; i++) input(menu, HH_QUICK_MENU_INPUT_DOWN);
      return;
   }
   if (state == 7 || state == 8)
   {
      for (i = 0; i < (state == 7 ? 3 : 5); i++) input(menu, HH_QUICK_MENU_INPUT_DOWN);
      input(menu, HH_QUICK_MENU_INPUT_CONFIRM);
      return;
   }
   input(menu, HH_QUICK_MENU_INPUT_DOWN);
   if (state == 3 || state == 9) input(menu, HH_QUICK_MENU_INPUT_DOWN);
   input(menu, HH_QUICK_MENU_INPUT_CONFIRM);
   if (state == 3) input(menu, HH_QUICK_MENU_INPUT_DOWN);
   if (state == 10)
   {
      input(menu, HH_QUICK_MENU_INPUT_DOWN);
      input(menu, HH_QUICK_MENU_INPUT_DOWN);
   }
   if (state == 4 || state == 5 || state == 6 || state == 9)
      input(menu, HH_QUICK_MENU_INPUT_CONFIRM);
   if (state == 5)
      hh_quick_menu_action_result(menu, HH_QUICK_MENU_SAVE_SUCCESS, NULL);
   if (state == 6)
      hh_quick_menu_action_result(menu, HH_QUICK_MENU_ACTION_ERROR, "保存失败，请重试");
   if (state == 9)
      hh_quick_menu_action_result(menu, HH_QUICK_MENU_LOAD_SUCCESS, NULL);
}

int main(int argc, char **argv)
{
   static const int sizes[][2] = {{640,480}, {1280,720}, {1920,1080}};
   static const char *dimensions[] = {"640x480", "1280x720", "1920x1080"};
   static const char *names[] = {"main", "disabled", "save-preview", "load-empty",
      "pending", "save-success", "error", "restart", "exit", "load-success", "no-preview",
      "recent", "recent-empty", "controls", "control-edit"};
   hh_quick_menu_t menu;
   hh_quick_menu_painter_t painter;
   svg_painter_t svg;
   char path[4096];
   FILE *html;
   int i, j;
   if (argc != 2 || strlen(argv[1]) > sizeof(path) - 100) return 1;
   memset(&painter, 0, sizeof(painter));
   strcpy(path, argv[1]);
   strcat(path, "/index.html");
   html = fopen(path, "w");
   if (!html) return 1;
   fputs("<!doctype html><meta charset='utf-8'><title>GameGo Quick Menu · C renderer</title>"
         "<style>body{background:#09131b;color:#eef4f1;font:16px sans-serif;margin:32px}"
         "a{color:#a3efb4}section{margin:40px 0}img{max-width:100%;display:block;margin:12px 0}"
         "summary{cursor:pointer;padding:12px}nav a{margin-right:16px}</style>"
         "<h1>GameGo / Quick Menu</h1><p>离线 Mock · 由正式 C renderer 输出 · 未连接 Runtime</p>"
         "<p>7 个主菜单循环导航；Slot 0–9 边界停止；确认弹窗默认取消。</p><nav>", html);
   for (j = 0; j < 15; j++) fprintf(html, "<a href='#%s'>%s</a>", names[j], names[j]);
   fputs("</nav>", html);
   painter.userdata = &svg;
   painter.rect = svg_rect;
   painter.text = svg_text;
   painter.text_centered = svg_text_centered;
   painter.controller = svg_controller;
   painter.preview = svg_preview;
   painter.icon = svg_icon;
   painter.quad = svg_quad;
   painter.thumbnail = svg_thumbnail;
   painter.video = svg_thumbnail;
   painter.video_frame = svg_thumbnail;
   for (j = 0; j < 15; j++)
   {
      fprintf(html, "<section id='%s'><h2>%s</h2>", names[j], names[j]);
      scenario(&menu, j);
      for (i = 0; i < 3; i++)
      {
         strcpy(path, argv[1]);
         strcat(path, "/");
         strcat(path, names[j]);
         strcat(path, "-");
         strcat(path, dimensions[i]);
         strcat(path, ".svg");
         svg.file = fopen(path, "w");
         if (!svg.file) return 1;
         svg.clip = 0;
         fprintf(svg.file, "<svg xmlns='http://www.w3.org/2000/svg' width='%d' height='%d' "
               "viewBox='0 0 %d %d'><rect width='100%%' height='100%%' fill='#456657'/>\n",
               sizes[i][0], sizes[i][1], sizes[i][0], sizes[i][1]);
         hh_quick_menu_render(&menu, sizes[i][0], sizes[i][1], NULL, &painter);
         fputs("</svg>\n", svg.file);
         fclose(svg.file);
         fprintf(html, "<details %s><summary>%d × %d</summary><img src='%s-%dx%d.svg' "
               "alt='%s at %dx%d'></details>", i == 0 ? "open" : "", sizes[i][0], sizes[i][1],
               names[j], sizes[i][0], sizes[i][1], names[j], sizes[i][0], sizes[i][1]);
      }
      fputs("</section>", html);
   }
   fclose(html);
   puts("PASS: 45 SVG previews generated by the C renderer");
   return 0;
}
