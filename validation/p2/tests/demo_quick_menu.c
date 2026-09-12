#include "mock_quick_menu.h"
#include <stdio.h>
#include <stdlib.h>

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

static void svg_text(void *data, hh_ui_rect_t r, const char *text,
      unsigned long color, float size, bool bold)
{
   svg_painter_t *svg = (svg_painter_t *)data;
   int clip = svg->clip++;
   fprintf(svg->file, "<defs><clipPath id='c%d'><rect x='%g' y='%g' "
         "width='%g' height='%g'/></clipPath></defs>\n", clip, r.x, r.y, r.width, r.height);
   fprintf(svg->file, "<text x='%g' y='%g' fill='#%06lx' font-size='%g' "
         "font-weight='%s' font-family='PingFang SC,Noto Sans CJK SC,sans-serif' "
         "clip-path='url(#c%d)'>", r.x, r.y + (r.height + size * 0.72) / 2,
         color >> 8, size, bold ? "600" : "400", clip);
   escape(svg->file, text);
   fputs("</text>\n", svg->file);
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
      "pending", "save-success", "error", "restart", "exit", "load-success", "no-preview"};
   hh_quick_menu_t menu;
   hh_quick_menu_painter_t painter;
   svg_painter_t svg;
   char path[4096];
   FILE *html;
   int i, j;
   if (argc != 2 || strlen(argv[1]) > sizeof(path) - 100) return 1;
   strcpy(path, argv[1]);
   strcat(path, "/index.html");
   html = fopen(path, "w");
   if (!html) return 1;
   fputs("<!doctype html><meta charset='utf-8'><title>GameGo Quick Menu · C renderer</title>"
         "<style>body{background:#09131b;color:#eef4f1;font:16px sans-serif;margin:32px}"
         "a{color:#a3efb4}section{margin:40px 0}img{max-width:100%;display:block;margin:12px 0}"
         "summary{cursor:pointer;padding:12px}nav a{margin-right:16px}</style>"
         "<h1>GameGo / Quick Menu</h1><p>离线 Mock · 由正式 C renderer 输出 · 未连接 Runtime</p>"
         "<p>6 个主菜单循环导航；Slot 0–9 边界停止；确认弹窗默认取消。</p><nav>", html);
   for (j = 0; j < 11; j++) fprintf(html, "<a href='#%s'>%s</a>", names[j], names[j]);
   fputs("</nav>", html);
   painter.userdata = &svg;
   painter.rect = svg_rect;
   painter.text = svg_text;
   painter.preview = svg_preview;
   for (j = 0; j < 11; j++)
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
   puts("PASS: 33 SVG previews generated by the C renderer");
   return 0;
}
