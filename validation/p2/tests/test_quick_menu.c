#include "mock_quick_menu.h"
#include <assert.h>
#include <stdio.h>

#define INPUT(m, key) hh_quick_menu_input(m, HH_QUICK_MENU_INPUT_##key)

static void select_item(hh_quick_menu_t *menu, int index)
{
   int i;
   hh_quick_menu_init(menu);
   hh_quick_menu_open(menu);
   assert(menu->state == HH_QUICK_MENU_OPENING);
   assert(INPUT(menu, CONFIRM) == HH_UI_ACTION_NONE);
   hh_quick_menu_finish_open(menu);
   for (i = 0; i < index; i++)
      INPUT(menu, DOWN);
}

static void test_main(void)
{
   hh_quick_menu_t m;
   int i;
   static const char *labels[] = {
      "继续游戏", "保存进度", "读取进度", "着色器", "按键配置", "重新开始", "高级菜单", "最近游戏", "退出游戏"
   };
   select_item(&m, 0);
   assert(m.view.item_count == HH_QUICK_MENU_ITEM_COUNT && m.view.selected_index == 0);
   for (i = 0; i < HH_QUICK_MENU_ITEM_COUNT; i++)
   {
      assert(m.view.items[i].id == (hh_quick_menu_item_id_t)i);
      assert(strcmp(m.view.items[i].label, labels[i]) == 0);
      assert(hh_quick_menu_item_state(&m, i) == (i == 0
               ? HH_QUICK_MENU_ITEM_FOCUSED : HH_QUICK_MENU_ITEM_NORMAL));
   }
   INPUT(&m, UP);
   assert(m.view.selected_index == HH_QUICK_MENU_ITEM_COUNT - 1);
   INPUT(&m, DOWN);
   assert(m.view.selected_index == 0);
   INPUT(&m, LEFT);
   assert(m.view.selected_index == HH_QUICK_MENU_ITEM_COUNT - 1);
   INPUT(&m, RIGHT);
   assert(m.view.selected_index == 0 && m.view.state_slot == 0);
   INPUT(&m, DOWN);
   INPUT(&m, UP);
   assert(INPUT(&m, CONFIRM) == HH_UI_ACTION_CONTINUE);
   assert(m.view.pending_action == HH_UI_ACTION_CONTINUE && m.view.pending_slot == -1);
   hh_quick_menu_action_complete(&m);
   INPUT(&m, BACK);
   assert(m.state == HH_QUICK_MENU_CLOSING);
   assert(INPUT(&m, CONFIRM) == HH_UI_ACTION_NONE);
   hh_quick_menu_finish_close(&m);
   assert(m.state == HH_QUICK_MENU_CLOSED);
   select_item(&m, HH_QUICK_MENU_ITEM_ADVANCED_MENU);
   assert(INPUT(&m, CONFIRM) == HH_UI_ACTION_ADVANCED_MENU);
}

static void test_dialogs(void)
{
   hh_quick_menu_t m;
   int i;
   for (i = HH_QUICK_MENU_ITEM_RESET; i <= HH_QUICK_MENU_ITEM_EXIT; i += 3)
   {
      select_item(&m, i);
      assert(INPUT(&m, CONFIRM) == HH_UI_ACTION_NONE);
      assert(m.state == HH_QUICK_MENU_CONFIRM_DIALOG);
      assert(m.view.dialog.visible && !m.view.dialog.confirm_selected);
      assert(strcmp(m.view.dialog.detail, "未保存的游戏进度可能丢失。") == 0);
      assert(INPUT(&m, CONFIRM) == HH_UI_ACTION_NONE);
      assert(m.state == HH_QUICK_MENU_OPEN && !m.view.dialog.visible);
      INPUT(&m, CONFIRM);
      INPUT(&m, RIGHT);
      assert(m.view.dialog.confirm_selected);
      INPUT(&m, LEFT);
      assert(!m.view.dialog.confirm_selected);
      INPUT(&m, BACK);
      assert(m.state == HH_QUICK_MENU_OPEN);
      INPUT(&m, CONFIRM);
      INPUT(&m, RIGHT);
      hh_quick_menu_set_busy(&m, true);
      assert(INPUT(&m, CONFIRM) == HH_UI_ACTION_NONE);
      hh_quick_menu_set_busy(&m, false);
      assert(INPUT(&m, CONFIRM) == (i == HH_QUICK_MENU_ITEM_RESET ? HH_UI_ACTION_RESET : HH_UI_ACTION_EXIT));
      assert(m.state == HH_QUICK_MENU_ACTION_PENDING);
      assert(INPUT(&m, CONFIRM) == HH_UI_ACTION_NONE);
   }
   select_item(&m, HH_QUICK_MENU_ITEM_RESET);
   INPUT(&m, CONFIRM);
   INPUT(&m, RIGHT);
   m.view.capabilities.reset_enabled = false;
   assert(INPUT(&m, CONFIRM) == HH_UI_ACTION_NONE);
   assert(m.state == HH_QUICK_MENU_OPEN);
}

static void test_slots(void)
{
   hh_quick_menu_t m;
   hh_quick_menu_slot_t slot;
   int i;
   select_item(&m, 1);
   assert(INPUT(&m, CONFIRM) == HH_UI_ACTION_NONE);
   assert(m.view.page == HH_QUICK_MENU_PAGE_SAVE);
   INPUT(&m, UP);
   assert(m.view.state_slot == 0);
   for (i = 0; i < 10; i++)
   {
      assert(m.view.state_slot == i && m.view.slots[i].index == i);
      assert(!hh_quick_menu_slot_disabled(&m, i));
      assert(INPUT(&m, CONFIRM) == HH_UI_ACTION_SAVE);
      assert(m.view.pending_action == HH_UI_ACTION_SAVE && m.view.pending_slot == i);
      assert(INPUT(&m, CONFIRM) == HH_UI_ACTION_NONE);
      INPUT(&m, DOWN);
      INPUT(&m, BACK);
      hh_quick_menu_close(&m);
      assert(m.state == HH_QUICK_MENU_ACTION_PENDING && m.view.state_slot == i);
      hh_quick_menu_action_result(&m, HH_QUICK_MENU_SAVE_SUCCESS, NULL);
      assert(m.view.feedback == HH_QUICK_MENU_SAVE_SUCCESS);
      assert(!m.view.slots[i].occupied);
      INPUT(&m, DOWN);
   }
   assert(m.view.state_slot == 9);
   INPUT(&m, RIGHT);
   assert(m.view.state_slot == 9);
   INPUT(&m, LEFT);
   assert(m.view.state_slot == 8);
   INPUT(&m, BACK);
   assert(m.view.page == HH_QUICK_MENU_PAGE_MAIN && m.view.selected_index == 1);
   INPUT(&m, DOWN);
   assert(INPUT(&m, CONFIRM) == HH_UI_ACTION_NONE);
   assert(m.view.page == HH_QUICK_MENU_PAGE_LOAD);
   for (i = 0; i < 10; i++)
   {
      m.view.state_slot = i;
      assert(hh_quick_menu_slot_disabled(&m, i));
      assert(INPUT(&m, CONFIRM) == HH_UI_ACTION_NONE);
   }
   memset(&slot, 0, sizeof(slot));
   slot.index = 9;
   slot.occupied = true;
   hh_quick_menu_set_slot(&m, &slot);
   assert(INPUT(&m, CONFIRM) == HH_UI_ACTION_LOAD);
   assert(m.view.pending_slot == 9);
   hh_quick_menu_action_result(&m, HH_QUICK_MENU_LOAD_SUCCESS, NULL);
   assert(strcmp(m.view.message, "读取成功") == 0);
   slot.disabled = true;
   hh_quick_menu_set_slot(&m, &slot);
   assert(INPUT(&m, CONFIRM) == HH_UI_ACTION_NONE);
   assert(hh_quick_menu_slot_disabled(&m, -1));
   assert(hh_quick_menu_slot_disabled(&m, 10));
}

static void test_capabilities_feedback(void)
{
   hh_quick_menu_t m;
   hh_quick_menu_capabilities_t caps;
   hh_quick_menu_game_t game;
   char message[80];
   int i;
   for (i = 1; i <= HH_QUICK_MENU_ITEM_ADVANCED_MENU; i++)
   {
      if (i == HH_QUICK_MENU_ITEM_CONTROLS) continue;
      select_item(&m, i);
      memset(&caps, 0, sizeof(caps));
      hh_quick_menu_set_capabilities(&m, &caps);
      assert(hh_quick_menu_item_state(&m, i) == HH_QUICK_MENU_ITEM_DISABLED);
      assert(INPUT(&m, CONFIRM) == HH_UI_ACTION_NONE);
      INPUT(&m, DOWN);
      assert(m.view.selected_index == (size_t)(i + 1));
   }
   select_item(&m, 1);
   m.view.items[1].disabled = true;
   assert(INPUT(&m, CONFIRM) == HH_UI_ACTION_NONE);
   m.view.items[1].disabled = false;
   hh_quick_menu_set_busy(&m, true);
   INPUT(&m, DOWN);
   assert(m.view.selected_index == 1);
   assert(INPUT(&m, CONFIRM) == HH_UI_ACTION_NONE);
   hh_quick_menu_set_busy(&m, false);
   INPUT(&m, CONFIRM);
   m.view.capabilities.save_enabled = false;
   assert(INPUT(&m, CONFIRM) == HH_UI_ACTION_NONE);
   m.view.capabilities.save_enabled = true;
   INPUT(&m, CONFIRM);
   assert(hh_quick_menu_item_state(&m, 1) == HH_QUICK_MENU_ITEM_PENDING);
   hh_quick_menu_action_result(&m, HH_QUICK_MENU_ACTION_ERROR, "保存失败，请重试");
   assert(m.view.feedback == HH_QUICK_MENU_ACTION_ERROR);
   assert(strcmp(m.view.message, "保存失败，请重试") == 0);
   assert(m.state == HH_QUICK_MENU_OPEN && m.view.pending_slot == -1);
   assert(INPUT(&m, CONFIRM) == HH_UI_ACTION_SAVE);
   assert(m.view.feedback == HH_QUICK_MENU_FEEDBACK_NONE);
   hh_quick_menu_action_complete(&m);
   assert(m.view.feedback == HH_QUICK_MENU_ACTION_SUCCESS);
   hh_quick_menu_clear_feedback(&m);
   assert(!m.view.message[0]);
   memset(&game, 'x', sizeof(game));
   hh_quick_menu_set_game(&m, &game);
   assert(strlen(m.view.game.title) == HH_QUICK_MENU_TEXT_MAX - 1);
   memset(&game, 0, sizeof(game));
   for (i = 0; i < 42; i++) strcat(game.title, "游");
   game.title[126] = (char)0xe6;
   game.title[127] = (char)0xb8;
   hh_quick_menu_set_game(&m, &game);
   assert(strlen(m.view.game.title) == 126);
   memset(message, 'x', 62);
   strcpy(message + 62, "保存失败");
   INPUT(&m, CONFIRM);
   hh_quick_menu_action_result(&m, HH_QUICK_MENU_ACTION_ERROR, message);
   assert(strlen(m.view.message) == 62);
}

static void test_duplicate_all_actions(void)
{
   hh_quick_menu_t m;
   int i, j;
   for (i = 0; i < HH_QUICK_MENU_ITEM_COUNT; i++)
   {
      if (i == HH_QUICK_MENU_ITEM_RECENT) continue;
      select_item(&m, i);
      if (i == 2) m.view.slots[0].occupied = true;
      INPUT(&m, CONFIRM);
      if (i == 1 || i == 2) INPUT(&m, CONFIRM);
      if (i == HH_QUICK_MENU_ITEM_RESET || i == HH_QUICK_MENU_ITEM_EXIT)
      {
         INPUT(&m, RIGHT);
         INPUT(&m, CONFIRM);
      }
      assert(m.state == HH_QUICK_MENU_ACTION_PENDING);
      for (j = 0; j < 6; j++)
         assert(hh_quick_menu_input(&m, (hh_quick_menu_input_t)j) == HH_UI_ACTION_NONE);
      assert(m.state == HH_QUICK_MENU_ACTION_PENDING);
      assert(m.view.selected_index == (size_t)i && m.view.state_slot == 0);
   }
}

static void within(hh_ui_rect_t inner, hh_ui_rect_t outer)
{
   const float epsilon = 0.02f;
   assert(inner.width > 0 && inner.height > 0);
   assert(inner.x >= outer.x - epsilon && inner.y >= outer.y - epsilon);
   assert(inner.x + inner.width <= outer.x + outer.width + epsilon);
   assert(inner.y + inner.height <= outer.y + outer.height + epsilon);
}

typedef struct recorder
{
   int videos, thumbnails, video_frames;
   bool video_result, frame_result;
   hh_ui_rect_t viewport;
   int texts, rects, previews, placeholders, pending, disabled;
   int unflushed_texts, flushes, centered_texts;
   bool preview_result, carousel;
   int pause_bars, regular_dots, saved_dots, current_dots, edge_cards;
} recorder_t;

static void within_paint(hh_ui_rect_t r, const recorder_t *rec)
{
   hh_ui_rect_t viewport = rec->viewport;
   if (rec->carousel)
   {
      viewport.x -= rec->viewport.width;
      viewport.width *= 3;
   }
   within(r, viewport);
}

static void record_rect(void *data, hh_ui_rect_t r, unsigned long fill,
      unsigned long border, float radius, float stroke)
{
   recorder_t *rec = (recorder_t *)data;
   float s = rec->viewport.width / 1920;
   (void)radius; (void)stroke;
   within_paint(r, rec);
   if (fill == 0x080b11ccUL) assert(rec->unflushed_texts == 0);
   if (fill == 0xaab3c28cUL) rec->pause_bars++;
   if (rec->carousel && r.width == r.height && border == 0xa5b6c8ffUL)
   {
      if (fill == 0xa5b6c8ffUL) rec->saved_dots++;
      else rec->regular_dots++;
   }
   if (rec->carousel && r.width == r.height && fill == 0x76e2b2ffUL)
      rec->current_dots++;
   if (rec->carousel && r.width == 304 * s
         && r.x + r.width > rec->viewport.width)
      rec->edge_cards++;
   rec->rects++;
}

static void record_text(void *data, hh_ui_rect_t r, const char *text,
      unsigned long color, float size, bool bold)
{
   recorder_t *rec = (recorder_t *)data;
   (void)color; (void)bold;
   within_paint(r, rec);
   assert(size > 0 && size <= r.height);
   rec->texts++;
   rec->unflushed_texts++;
   if (strcmp(text, "No Preview") == 0 || strcmp(text, "暂无预览") == 0
         || strcmp(text, "空卡槽") == 0) rec->placeholders++;
   if (strstr(text, "处理中")) rec->pending++;
   if (strcmp(text, "不可用") == 0) rec->disabled++;
}

static void record_text_centered(void *data, hh_ui_rect_t r, const char *text,
      unsigned long color, float size, bool bold)
{
   recorder_t *rec = (recorder_t *)data;
   float s = rec->viewport.width / 1920;
   assert(r.width >= 192 * s - 0.01f && r.width <= 192 * s + 0.01f);
   record_text(data, r, text, color, size, bold);
   rec->centered_texts++;
}

static void record_quad(void *data, const float *v,
      unsigned long top, unsigned long bottom)
{
   recorder_t *rec = (recorder_t *)data;
   int i;
   (void)top;
   (void)bottom;
   for (i = 0; i < 4; i++)
   {
      assert(v[i * 2] >= (rec->carousel ? -rec->viewport.width : 0)
            && v[i * 2] <= rec->viewport.width * (rec->carousel ? 2 : 1));
      if (rec->carousel && v[i * 2] > rec->viewport.width)
         rec->edge_cards++;
      assert(v[i * 2 + 1] >= 0 && v[i * 2 + 1] <= rec->viewport.height);
   }
   assert(top || bottom);
   rec->rects++;
}

static void record_icon(void *data, hh_ui_rect_t r,
      hh_quick_menu_icon_t icon, unsigned long color)
{
   recorder_t *rec = (recorder_t *)data;
   (void)icon; (void)color;
   within_paint(r, rec);
   assert(r.width > 0 && r.width == r.height);
}

static void test_slot_touch(void)
{
   static const int sizes[][2] = {{640,480}, {1280,720}, {1920,1080}};
   hh_quick_menu_layout_t l;
   hh_quick_menu_t m;
   hh_quick_menu_input_t input;
   float x, y;
   hh_ui_rect_t card;
   int i, j, k, visible;
   for (i = 0; i < 3; i++)
   {
      assert(hh_quick_menu_compute_layout(sizes[i][0], sizes[i][1], &l));
      select_item(&m, 1);
      INPUT(&m, CONFIRM);
      for (j = 0; j < 10; j++)
      {
         for (k = 0; k <= 9; k += 9)
         {
            m.view.state_slot = k;
            x = l.slot_dots[j].x + l.slot_dots[j].width * 0.5f;
            y = l.slot_dots[j].y + l.slot_dots[j].height * 0.5f;
            assert(hh_quick_menu_touch(&m, x, y, true,
                     sizes[i][0], sizes[i][1], &input));
            assert(m.view.state_slot == j);
            assert(hh_quick_menu_touch(&m, x, y, false,
                     sizes[i][0], sizes[i][1], &input));
            assert(input == HH_QUICK_MENU_INPUT_CONFIRM);
            assert(m.view.state_slot == j);
            assert(m.view.pending_action == HH_UI_ACTION_NONE);
         }
      }
      m.view.state_slot = 9;
      x = l.slot_dots[0].x + 2 * l.scale;
      y = l.slot_dots[0].y + 2 * l.scale;
      hh_quick_menu_touch(&m, x, y, true, sizes[i][0], sizes[i][1], &input);
      hh_quick_menu_touch(&m, x, y, false, sizes[i][0], sizes[i][1], &input);
      assert(m.view.state_slot == 9 && input == HH_QUICK_MENU_INPUT_NONE);
      for (j = 0; j < 4; j++)
      {
         x = l.footer.x + (j == 0 ? 72 : j == 1 ? 260 :
               j == 2 ? 166 : 360) * l.scale;
         y = l.footer.y + 26 * l.scale;
         hh_quick_menu_touch(&m, x, y, true, sizes[i][0], sizes[i][1], &input);
         hh_quick_menu_touch(&m, x, y, false, sizes[i][0], sizes[i][1], &input);
         assert(input == (j == 0 ? HH_QUICK_MENU_INPUT_CONFIRM :
                  j == 1 ? HH_QUICK_MENU_INPUT_BACK : HH_QUICK_MENU_INPUT_NONE));
      }
      for (k = 0; k < 10; k++)
      {
         visible = 0;
         for (j = 0; j < 10; j++)
         {
            if (!hh_quick_menu_slot_card_bounds(&l, hh_quick_menu_slot_scroll(k), j, &card))
               continue;
            visible++;
            assert(card.width == 304 * l.scale);
            assert(card.height == 270 * l.scale);
            assert(card.y == l.slot_card.y);
            assert(card.x < l.viewport.width && card.x + card.width > 0);
            m.view.state_slot = k;
            m.slot_scroll = hh_quick_menu_slot_scroll(k);
            x = card.x + card.width * 0.5f;
            if (x >= l.viewport.width) x = l.viewport.width - l.scale;
            if (x < 0) x = l.scale;
            y = card.y + card.height * 0.5f;
            assert(hh_quick_menu_touch(&m, x, y, true,
                     sizes[i][0], sizes[i][1], &input));
            assert(m.view.state_slot == j);
            assert(hh_quick_menu_touch(&m, x, y, false,
                     sizes[i][0], sizes[i][1], &input));
            assert(m.view.state_slot == j && input == HH_QUICK_MENU_INPUT_CONFIRM);
            assert(m.view.pending_action == HH_UI_ACTION_NONE);
         }
         assert(visible >= 6 && visible <= 7);
      }
   }
}

static void test_main_touch(void)
{
   hh_quick_menu_t m;
   hh_quick_menu_layout_t l;
   hh_quick_menu_input_t key;
   hh_ui_rect_t item;
   float x, y;
   int i;
   assert(hh_quick_menu_compute_layout(1920, 1080, &l));
   for (i = 0; i < HH_QUICK_MENU_ITEM_COUNT; i++)
   {
      select_item(&m, i);
      m.main_scroll = hh_quick_menu_main_scroll(m.view.selected_index);
      assert(hh_quick_menu_main_item_bounds(&l, m.main_scroll, i, &item));
      x = item.x + item.width / 2;
      y = item.y + item.height / 2;
      hh_quick_menu_touch(&m, x, y, true, 1920, 1080, &key);
      hh_quick_menu_touch(&m, x, y, false, 1920, 1080, &key);
      assert(m.view.selected_index == (size_t)i && key == HH_QUICK_MENU_INPUT_CONFIRM);
      x = item.x + 2;
      y = item.y + 2;
      hh_quick_menu_touch(&m, x, y, true, 1920, 1080, &key);
      hh_quick_menu_touch(&m, x, y, false, 1920, 1080, &key);
      assert(key == HH_QUICK_MENU_INPUT_NONE);
   }
   select_item(&m, 0);
   hh_quick_menu_touch(&m, 950, 800, true, 1920, 1080, &key);
   hh_quick_menu_touch(&m, 850, 800, true, 1920, 1080, &key);
   hh_quick_menu_touch(&m, 850, 800, false, 1920, 1080, &key);
   assert(key == HH_QUICK_MENU_INPUT_NONE);
   assert(m.main_scroll > 0.4f && m.main_scroll < 0.5f);
   assert(m.view.selected_index == 0);
   hh_quick_menu_touch(&m, 950, 800, true, 1920, 1080, &key);
   hh_quick_menu_touch(&m, 950, 700, true, 1920, 1080, &key);
   hh_quick_menu_touch(&m, 950, 700, false, 1920, 1080, &key);
   assert(key == HH_QUICK_MENU_INPUT_NONE);
   x = l.main_footer.x + 200;
   y = l.main_footer.y + 26;
   hh_quick_menu_touch(&m, x, y, true, 1920, 1080, &key);
   hh_quick_menu_touch(&m, x, y, false, 1920, 1080, &key);
   assert(key == HH_QUICK_MENU_INPUT_BACK);
   hh_quick_menu_set_busy(&m, true);
   assert(!hh_quick_menu_touch(&m, x, y, true, 1920, 1080, &key));
}

static bool record_preview(void *data, hh_ui_rect_t r, int slot)
{
   recorder_t *rec = (recorder_t *)data;
   within_paint(r, rec);
   assert(slot == 0);
   rec->previews++;
   return rec->preview_result;
}

static void record_flush(void *data)
{
   recorder_t *rec = (recorder_t *)data;
   assert(rec->unflushed_texts > 0);
   rec->unflushed_texts = 0;
   rec->flushes++;
}

static void test_main_overflow(void)
{
   static const int sizes[][2] = {
      {640,480}, {1280,720}, {1920,1080}, {2560,1080}, {720,1280}
   };
   hh_quick_menu_layout_t l;
   hh_quick_menu_t m;
   hh_quick_menu_input_t key;
   hh_ui_rect_t item, selected;
   float x, y;
   int i, j, k;
   for (i = 0; i < 5; i++)
   {
      assert(hh_quick_menu_compute_layout(sizes[i][0], sizes[i][1], &l));
      for (j = 0; j < HH_QUICK_MENU_ITEM_COUNT; j++)
      {
         assert(hh_quick_menu_main_item_bounds(&l, hh_quick_menu_main_scroll(j), j, &selected));
         within(selected, l.viewport);
         assert(selected.width == 240 * l.scale);
         assert(l.viewport.width - selected.x - selected.width >= 217 * l.scale);
         for (k = 0; k < HH_QUICK_MENU_ITEM_COUNT; k++)
         {
            if (!hh_quick_menu_main_item_bounds(&l, hh_quick_menu_main_scroll(j), k, &item))
               continue;
            assert(item.width == selected.width);
            x = item.x + item.width / 2;
            y = item.y + item.height / 2;
            if (x < 0 || x >= l.viewport.width)
               continue;
            select_item(&m, j);
            m.main_scroll = hh_quick_menu_main_scroll(j);
            assert(hh_quick_menu_touch(&m, x, y, true,
                     sizes[i][0], sizes[i][1], &key));
            assert(hh_quick_menu_touch(&m, x, y, false,
                     sizes[i][0], sizes[i][1], &key));
            assert(m.view.selected_index == (size_t)k);
            assert(key == HH_QUICK_MENU_INPUT_CONFIRM);
         }
      }
      select_item(&m, 0);
      x = l.viewport.width + 1;
      y = l.items[0].y + l.items[0].height / 2;
      hh_quick_menu_touch(&m, x, y, true, sizes[i][0], sizes[i][1], &key);
      hh_quick_menu_touch(&m, x, y, false, sizes[i][0], sizes[i][1], &key);
      assert(key == HH_QUICK_MENU_INPUT_NONE && m.view.selected_index == 0);
   }
   puts("PASS: main menu keeps button width and focused items visible; touch follows overflow across 5 resolutions");
}

static void test_layout_render(void)
{
   static const int sizes[][2] = {{640,480}, {1280,720}, {1920,1080}};
   hh_quick_menu_layout_t l;
   hh_quick_menu_t m;
   hh_quick_menu_painter_t painter;
   recorder_t rec;
   int i, j;
   memset(&painter, 0, sizeof(painter));
   painter.userdata = &rec;
   painter.rect = record_rect;
   painter.text = record_text;
   painter.text_centered = record_text_centered;
   painter.preview = record_preview;
   painter.icon = record_icon;
   painter.quad = record_quad;
   painter.flush = record_flush;
   for (i = 0; i < 3; i++)
   {
      assert(hh_quick_menu_compute_layout(sizes[i][0], sizes[i][1], &l));
      within(l.panel, l.viewport);
      assert(l.panel.x >= 24 * l.scale && l.panel.y >= 24 * l.scale);
      within(l.header, l.viewport);
      assert(l.header.x == l.main_header.x && l.header.y == l.main_header.y);
      within(l.dialog, l.panel);
      within(l.footer, l.viewport);
      within(l.feedback, l.viewport);
      within(l.slot_preview, l.slot_card);
      within(l.slot_info, l.slot_card);
      within(l.slot_card, l.viewport);
      for (j = 0; j < HH_QUICK_MENU_ITEM_COUNT; j++)
      {
         assert(l.items[j].width == 240 * l.scale);
         if (j) assert(l.items[j-1].x < l.items[j].x
               && l.items[j-1].y == l.items[j].y);
      }
      assert(l.items[HH_QUICK_MENU_ITEM_COUNT - 1].x
            + l.items[HH_QUICK_MENU_ITEM_COUNT - 1].width > l.viewport.width);
      assert(l.items[HH_QUICK_MENU_ITEM_COUNT - 1].y + l.items[HH_QUICK_MENU_ITEM_COUNT - 1].height < l.feedback.y);
      assert(l.feedback.y + l.feedback.height < l.footer.y);
      for (j = 0; j < 10; j++)
      {
         within(l.slot_dots[j], l.viewport);
         within(l.slot_cards[j], l.viewport);
         assert(l.slot_cards[j].width > 0 && l.slot_cards[j].height > 0);
      }
      for (j = 0; j < 2; j++) within(l.dialog_buttons[j], l.dialog);
      memset(&rec, 0, sizeof(rec));
      rec.viewport = l.viewport;
      rec.carousel = true;
      mock_quick_menu(&m);
      hh_quick_menu_render(&m, sizes[i][0], sizes[i][1], NULL, &painter);
      assert(rec.disabled == 1 && rec.texts > 6 && rec.centered_texts > 0);
      INPUT(&m, DOWN);
      INPUT(&m, CONFIRM);
      rec.carousel = true;
      hh_quick_menu_render(&m, sizes[i][0], sizes[i][1], NULL, &painter);
      assert(rec.pause_bars == 4 && rec.edge_cards > 0);
      assert(rec.regular_dots == 8 && rec.saved_dots == 1 && rec.current_dots == 1);
      assert(rec.previews == 1 && rec.placeholders > 0);
      rec.preview_result = true;
      hh_quick_menu_render(&m, sizes[i][0], sizes[i][1], NULL, &painter);
      assert(rec.previews == 2 && rec.placeholders > 0);
      painter.preview = NULL;
      hh_quick_menu_render(&m, sizes[i][0], sizes[i][1], NULL, &painter);
      assert(rec.previews == 2 && rec.placeholders > 0);
      painter.preview = record_preview;
      m.view.capabilities.screenshot_enabled = false;
      hh_quick_menu_render(&m, sizes[i][0], sizes[i][1], NULL, &painter);
      assert(rec.previews == 2 && rec.placeholders > 0);
      INPUT(&m, CONFIRM);
      hh_quick_menu_render(&m, sizes[i][0], sizes[i][1], NULL, &painter);
      assert(rec.pending > 0);
      hh_quick_menu_action_result(&m, HH_QUICK_MENU_ACTION_ERROR, "保存失败");
      hh_quick_menu_render(&m, sizes[i][0], sizes[i][1], NULL, &painter);
      select_item(&m, HH_QUICK_MENU_ITEM_RESET);
      INPUT(&m, CONFIRM);
      rec.carousel = true;
      hh_quick_menu_render(&m, sizes[i][0], sizes[i][1], NULL, &painter);
      assert(rec.flushes == 1);
      hh_quick_menu_back(&m);
      hh_quick_menu_close(&m);
      hh_quick_menu_finish_close(&m);
      j = rec.texts;
      hh_quick_menu_render(&m, sizes[i][0], sizes[i][1], NULL, &painter);
      assert(rec.texts == j);
      select_item(&m, HH_QUICK_MENU_ITEM_CONTROLS);
      m.view.page = HH_QUICK_MENU_PAGE_CONTROLS;
      m.view.controls_valid = true;
      m.view.controls.player_count = 8;
      m.view.feedback = HH_QUICK_MENU_ACTION_ERROR;
      strcpy(m.view.message, "保存失败");
      memset(&rec, 0, sizeof(rec));
      rec.viewport = l.viewport;
      for (j = 0; j < HH_QUICK_MENU_CONTROL_ROWS; j++)
         within(l.control_rows[j], l.viewport);
      assert(HH_QUICK_MENU_CONTROL_ROWS == 8);
      assert(l.control_rows[0].y < l.main_header.y + 150 * l.scale);
      hh_quick_menu_render(&m, sizes[i][0], sizes[i][1], NULL, &painter);
      assert(rec.texts >= 12);
   }
   assert(!hh_quick_menu_compute_layout(0, 480, &l));
   assert(!hh_quick_menu_compute_layout(640, -1, &l));
   assert(!hh_quick_menu_compute_layout(640, 480, NULL));
}

static void test_main_animation(void)
{
   static const int sizes[][2] = {{640,480}, {1280,720}, {1920,1080}};
   hh_quick_menu_t m;
   hh_quick_menu_layout_t l;
   hh_ui_rect_t before, during, after;
   hh_quick_menu_input_t input;
   float position, x, y;
   int i;
   for (i = 0; i < 3; i++)
   {
      select_item(&m, 2);
      assert(hh_quick_menu_compute_layout(sizes[i][0], sizes[i][1], &l));
      hh_quick_menu_update_animation(&m, 1000);
      x = l.viewport.width / 2;
      y = l.items[0].y + l.items[0].height + 100 * l.scale;
      INPUT(&m, RIGHT);
      assert(m.view.selected_index == 3 && m.main_scroll == 0);
      assert(hh_quick_menu_main_item_bounds(&l, m.main_scroll, 3, &before));
      hh_quick_menu_update_animation(&m, 1016);
      hh_quick_menu_update_animation(&m, 1126);
      assert(m.main_scroll > 0 && m.main_scroll < 1);
      assert(hh_quick_menu_main_item_bounds(&l, m.main_scroll, 3, &during));
      assert(during.x < before.x && during.x > before.x - 240 * l.scale);
      x = during.x + during.width / 2;
      y = during.y + during.height / 2;
      hh_quick_menu_touch(&m, x, y, true, sizes[i][0], sizes[i][1], &input);
      hh_quick_menu_touch(&m, x, y, false, sizes[i][0], sizes[i][1], &input);
      assert(m.view.selected_index == 3 && input == HH_QUICK_MENU_INPUT_CONFIRM);
      hh_quick_menu_update_animation(&m, 1236);
      assert(m.main_scroll == 1);
      assert(hh_quick_menu_main_item_bounds(&l, m.main_scroll, 3, &after));
      assert(after.x >= before.x - 240 * l.scale - 0.01f
            && after.x <= before.x - 240 * l.scale + 0.01f);
      INPUT(&m, LEFT);
      hh_quick_menu_update_animation(&m, 1252);
      hh_quick_menu_update_animation(&m, 1362);
      assert(m.main_scroll > 0 && m.main_scroll < 1);
      position = m.main_scroll;
      INPUT(&m, RIGHT);
      INPUT(&m, RIGHT);
      hh_quick_menu_update_animation(&m, 1378);
      assert(m.main_scroll == position);
      hh_quick_menu_update_animation(&m, 1488);
      assert(m.main_scroll > position && m.main_scroll < 2);
      hh_quick_menu_update_animation(&m, 1598);
      assert(m.main_scroll == 2);
      m.view.selected_index = HH_QUICK_MENU_ITEM_COUNT - 1;
      hh_quick_menu_update_animation(&m, 1614);
      hh_quick_menu_update_animation(&m, 1834);
      assert(m.main_scroll == 4);
      assert(hh_quick_menu_main_item_bounds(&l, m.main_scroll,
               m.view.selected_index, &after));
      within(after, l.viewport);
      assert(l.viewport.width - after.x - after.width >= 450 * l.scale);
      INPUT(&m, RIGHT);
      assert(m.view.selected_index == 0);
      hh_quick_menu_update_animation(&m, 1850);
      hh_quick_menu_update_animation(&m, 1960);
      assert(m.main_scroll > 0 && m.main_scroll < 4);
      hh_quick_menu_update_animation(&m, 2070);
      assert(m.main_scroll == 0 && m.slot_scroll == 0);
   }
   puts("PASS: main menu animates both directions in 220ms, retargets smoothly, wraps and keeps animated touch bounds aligned");
}

static void test_slot_animation(void)
{
   hh_quick_menu_t m;
   hh_quick_menu_layout_t l;
   hh_ui_rect_t before, during, after;
   hh_quick_menu_input_t input;
   float position, x, y;
   select_item(&m, 1);
   INPUT(&m, CONFIRM);
   assert(hh_quick_menu_compute_layout(1920, 1080, &l));
   hh_quick_menu_update_animation(&m, 1000);
   INPUT(&m, RIGHT);
   INPUT(&m, RIGHT);
   INPUT(&m, RIGHT);
   assert(m.view.state_slot == 3 && m.slot_scroll == 0);
   assert(hh_quick_menu_slot_card_bounds(&l, m.slot_scroll, 3, &before));
   hh_quick_menu_update_animation(&m, 1016);
   hh_quick_menu_update_animation(&m, 1126);
   assert(m.slot_scroll > 0 && m.slot_scroll < 1);
   assert(hh_quick_menu_slot_card_bounds(&l, m.slot_scroll, 3, &during));
   assert(during.x < before.x && during.x > before.x - 320);
   x = during.x + during.width / 2;
   y = during.y + during.height / 2;
   hh_quick_menu_touch(&m, x, y, true, 1920, 1080, &input);
   hh_quick_menu_touch(&m, x, y, false, 1920, 1080, &input);
   assert(m.view.state_slot == 3);
   hh_quick_menu_update_animation(&m, 1236);
   assert(m.slot_scroll == 1);
   assert(hh_quick_menu_slot_card_bounds(&l, m.slot_scroll, 3, &after));
   assert(after.x == before.x - 320);
   INPUT(&m, LEFT);
   hh_quick_menu_update_animation(&m, 1252);
   hh_quick_menu_update_animation(&m, 1362);
   assert(m.slot_scroll > 0 && m.slot_scroll < 1);
   position = m.slot_scroll;
   INPUT(&m, RIGHT);
   INPUT(&m, RIGHT);
   hh_quick_menu_update_animation(&m, 1378);
   assert(m.slot_scroll == position);
   hh_quick_menu_update_animation(&m, 1488);
   assert(m.slot_scroll > position && m.slot_scroll < 2);
   hh_quick_menu_update_animation(&m, 1598);
   assert(m.slot_scroll == 2);
   INPUT(&m, BACK);
   m.view.state_slot = 9;
   INPUT(&m, CONFIRM);
   assert(m.slot_scroll == 5);
}

static void test_recent(void)
{
   static const int sizes[][2] = {{640,480}, {1280,720}, {1920,1080}};
   hh_quick_menu_t m;
   hh_quick_menu_layout_t layout;
   hh_quick_menu_input_t input;
   int i;
   float x, y;

   select_item(&m, HH_QUICK_MENU_ITEM_RECENT);
   assert(INPUT(&m, CONFIRM) == HH_UI_ACTION_NONE);
   assert(m.view.page == HH_QUICK_MENU_PAGE_RECENT);
   assert(INPUT(&m, CONFIRM) == HH_UI_ACTION_NONE);
   m.view.recent_count = 8;
   INPUT(&m, UP);
   assert(m.view.recent_selected == 0);
   for (i = 0; i < 9; i++)
      INPUT(&m, DOWN);
   assert(m.view.recent_selected == 7 && m.view.recent_first == 2);
   m.view.recent[5].disabled = true;
   assert(INPUT(&m, CONFIRM) == HH_UI_ACTION_NONE);
   m.view.recent[5].disabled = false;
   assert(INPUT(&m, CONFIRM) == HH_UI_ACTION_RECENT);
   assert(INPUT(&m, CONFIRM) == HH_UI_ACTION_NONE);
   INPUT(&m, UP);
   assert(m.view.recent_selected == 7);
   hh_quick_menu_action_result(&m, HH_QUICK_MENU_ACTION_ERROR, "读取失败");
   INPUT(&m, LEFT);
   assert(m.view.recent_selected == 6);
   assert(m.view.feedback == HH_QUICK_MENU_FEEDBACK_NONE);
   INPUT(&m, BACK);
   assert(m.view.page == HH_QUICK_MENU_PAGE_MAIN);
   assert(m.view.selected_index == HH_QUICK_MENU_ITEM_RECENT);
   for (i = 0; i < 3; i++)
   {
      hh_ui_rect_t card;
      assert(hh_quick_menu_compute_layout(sizes[i][0], sizes[i][1], &layout));
      INPUT(&m, CONFIRM);
      assert(hh_quick_menu_recent_card_bounds(&layout, m.slot_scroll, 3, &card));
      within(card, layout.viewport);
      assert((card.width - 36 * layout.scale) * 3
            == (card.height - 90 * layout.scale) * 4);
      x = card.x + card.width / 2;
      y = card.y + card.height / 2;
      assert(hh_quick_menu_touch(&m, x, y, true,
               sizes[i][0], sizes[i][1], &input));
      assert(hh_quick_menu_touch(&m, x, y, false,
               sizes[i][0], sizes[i][1], &input));
      assert(input == HH_QUICK_MENU_INPUT_CONFIRM && m.view.recent_selected == 3);
      assert(m.view.state_slot == 0);
      INPUT(&m, BACK);
   }
}

static void test_button_repeat(void)
{
   hh_quick_menu_t m;
   unsigned bit, triggers;
   int i;
   select_item(&m, HH_QUICK_MENU_ITEM_RECENT);
   INPUT(&m, CONFIRM);
   m.view.recent_count = 20;
   bit = 1U << HH_QUICK_MENU_INPUT_DOWN;
   triggers = hh_quick_menu_poll_buttons(&m, bit, 1000);
   assert(triggers == bit);
   INPUT(&m, DOWN);
   assert(m.view.recent_selected == 1);
   assert(hh_quick_menu_poll_buttons(&m, bit, 1349) == 0);
   assert(hh_quick_menu_poll_buttons(&m, bit, 1350) == bit);
   INPUT(&m, DOWN);
   assert(m.view.recent_selected == 2);
   assert(hh_quick_menu_poll_buttons(&m, bit, 1469) == 0);
   assert(hh_quick_menu_poll_buttons(&m, bit, 1470) == bit);
   INPUT(&m, DOWN);
   assert(m.view.recent_selected == 3);
   assert(hh_quick_menu_poll_buttons(&m, bit, 5000) == bit);
   assert(hh_quick_menu_poll_buttons(&m, bit, 5000) == 0);
   assert(hh_quick_menu_poll_buttons(&m, 0, 5001) == 0);
   assert(hh_quick_menu_poll_buttons(&m, 0, 9000) == 0);
   assert(hh_quick_menu_poll_buttons(&m, bit, 9001) == bit);
   m.view.busy = true;
   assert(hh_quick_menu_poll_buttons(&m, bit, 10000) == 0);
   m.view.busy = false;
   assert(hh_quick_menu_poll_buttons(&m, bit, 10349) == 0);
   assert(hh_quick_menu_poll_buttons(&m, bit, 10350) == bit);
   for (i = HH_QUICK_MENU_INPUT_UP; i <= HH_QUICK_MENU_INPUT_BACK; i++)
   {
      bit = 1U << i;
      hh_quick_menu_poll_buttons(&m, 0, 11000);
      assert(hh_quick_menu_poll_buttons(&m, bit, 11001) == bit);
      triggers = hh_quick_menu_poll_buttons(&m, bit, 12000);
      assert(triggers == (i <= HH_QUICK_MENU_INPUT_RIGHT ? bit : 0));
   }
   hh_quick_menu_close(&m);
   assert(hh_quick_menu_poll_buttons(&m, bit, 13000) == 0);
   hh_quick_menu_finish_close(&m);
   hh_quick_menu_open(&m);
   hh_quick_menu_finish_open(&m);
   assert(hh_quick_menu_poll_buttons(&m, bit, 13001) == bit);
   puts("PASS: held directions repeat after 350ms, then every 120ms; release/busy reset and confirm/back remain single press");
}

static bool record_recent_video(void *data, hh_ui_rect_t r, const char *path)
{
   recorder_t *rec = (recorder_t*)data;
   within_paint(r, rec);
   assert(strcmp(path, "demo.mp4") == 0);
   rec->videos++;
   return rec->video_result;
}

static bool record_recent_thumbnail(void *data, hh_ui_rect_t r, const char *path)
{
   recorder_t *rec = (recorder_t*)data;
   within_paint(r, rec);
   assert(strcmp(path, "cover.png") == 0);
   rec->thumbnails++;
   return true;
}

static bool record_recent_video_frame(void *data, hh_ui_rect_t r, const char *path)
{
   recorder_t *rec = (recorder_t*)data;
   within_paint(r, rec);
   assert(strcmp(path, "demo.mp4") == 0);
   rec->video_frames++;
   return rec->frame_result;
}

static void test_recent_video_priority(void)
{
   hh_quick_menu_t m;
   hh_quick_menu_layout_t layout;
   hh_quick_menu_painter_t painter;
   recorder_t rec;
   int i;
   select_item(&m, HH_QUICK_MENU_ITEM_RECENT);
   INPUT(&m, CONFIRM);
   m.view.recent_count = 3;
   for (i = 0; i < 3; i++)
      strcpy(m.view.recent[i].thumbnail, "cover.png");
   strcpy(m.view.recent[0].video, "demo.mp4");
   strcpy(m.view.recent[1].video, "demo.mp4");
   memset(&rec, 0, sizeof(rec));
   assert(hh_quick_menu_compute_layout(1920, 1080, &layout));
   rec.viewport = layout.viewport;
   rec.video_result = rec.frame_result = true;
   memset(&painter, 0, sizeof(painter));
   painter.userdata = &rec;
   painter.rect = record_rect;
   painter.text = record_text;
   painter.flush = record_flush;
   painter.video = record_recent_video;
   painter.video_frame = record_recent_video_frame;
   painter.thumbnail = record_recent_thumbnail;
   hh_quick_menu_render(&m, 1920, 1080, NULL, &painter);
   assert(rec.videos == 1 && rec.video_frames == 2 && rec.thumbnails == 1);
   m.state = HH_QUICK_MENU_OPENING;
   hh_quick_menu_render(&m, 1920, 1080, NULL, &painter);
   assert(rec.videos == 1 && rec.video_frames == 4 && rec.thumbnails == 2);
   m.state = HH_QUICK_MENU_OPEN;
   INPUT(&m, RIGHT);
   hh_quick_menu_render(&m, 1920, 1080, NULL, &painter);
   assert(rec.videos == 2 && rec.video_frames == 6 && rec.thumbnails == 3);
   rec.video_result = rec.frame_result = false;
   hh_quick_menu_render(&m, 1920, 1080, NULL, &painter);
   assert(rec.videos == 3 && rec.video_frames == 8 && rec.thumbnails == 6);
   INPUT(&m, RIGHT);
   rec.frame_result = true;
   hh_quick_menu_render(&m, 1920, 1080, NULL, &painter);
   assert(rec.videos == 3 && rec.video_frames == 10 && rec.thumbnails == 7);
   m.view.recent[0].video[0] = m.view.recent[1].video[0] = '\0';
   hh_quick_menu_render(&m, 1920, 1080, NULL, &painter);
   assert(rec.videos == 3 && rec.video_frames == 10 && rec.thumbnails == 10);
   puts("PASS: only focused recent video plays; neighbors use first frames; absent/failed videos use images without cover flash");
}

static void test_recent_carousel(void)
{
   hh_quick_menu_t m;
   hh_quick_menu_layout_t layout;
   hh_quick_menu_painter_t painter;
   recorder_t rec;
   hh_ui_rect_t card;
   size_t i;
   unsigned long time = 100;
   select_item(&m, HH_QUICK_MENU_ITEM_RECENT);
   INPUT(&m, CONFIRM);
   m.view.recent_count = 200;
   assert(hh_quick_menu_recent_scroll(0, 0) == 0);
   assert(hh_quick_menu_recent_scroll(0, 1) == 0);
   assert(hh_quick_menu_recent_scroll(7, 8) == 3);
   assert(hh_quick_menu_recent_scroll(199, 200) == 195);
   assert(hh_quick_menu_compute_layout(1920, 1080, &layout));
   memset(&painter, 0, sizeof(painter));
   painter.userdata = &rec;
   painter.rect = record_rect;
   painter.text = record_text;
   painter.flush = record_flush;
   for (i = 0; i < 200; i++)
   {
      hh_quick_menu_update_animation(&m, time);
      hh_quick_menu_update_animation(&m, time + 110);
      if (i > 2 && i < 197)
         assert(m.slot_scroll > m.slot_scroll_origin && m.slot_scroll < m.slot_scroll_target);
      hh_quick_menu_update_animation(&m, time + 220);
      assert(m.slot_scroll == hh_quick_menu_recent_scroll(i, 200));
      assert(hh_quick_menu_recent_card_bounds(&layout, m.slot_scroll, i, &card));
      within(card, layout.viewport);
      assert(i >= m.view.recent_first && i - m.view.recent_first < HH_QUICK_MENU_RECENT_ROWS);
      memset(&rec, 0, sizeof(rec));
      rec.viewport = layout.viewport;
      rec.carousel = true;
      hh_quick_menu_render(&m, 1920, 1080, NULL, &painter);
      assert(rec.current_dots == 1);
      INPUT(&m, RIGHT);
      time += 300;
   }
   assert(m.view.recent_selected == 199);
   puts("PASS: recent carousel scrolls across 200 games, keeps focus visible and bounds the page indicators");
}

static void test_shaders(void)
{
   static const int sizes[][2] = {{640,480}, {1280,720}, {1920,1080}};
   hh_quick_menu_t m;
   hh_quick_menu_layout_t layout;
   hh_quick_menu_input_t input;
   hh_quick_menu_painter_t painter;
   recorder_t rec;
   int i, j;
   float x, y;
   select_item(&m, HH_QUICK_MENU_ITEM_SHADER);
   assert(INPUT(&m, CONFIRM) == HH_UI_ACTION_SHADER_BEGIN);
   assert(INPUT(&m, CONFIRM) == HH_UI_ACTION_NONE);
   hh_quick_menu_action_complete(&m);
   m.view.page = HH_QUICK_MENU_PAGE_SHADER;
   m.view.shader_count = 8;
   m.view.shader_valid = true;
   m.view.shader_recommended = true;
   for (i = 0; i < 8; i++)
   {
      m.view.shaders[i].id = i;
      strcpy(m.view.shaders[i].name, "液晶像素");
      strcpy(m.view.shaders[i].description, "模拟掌机屏幕纹理");
   }
   INPUT(&m, DOWN);
   assert(m.view.shader_selected == 1 && m.view.state_slot == 0);
   assert(INPUT(&m, CONFIRM) == HH_UI_ACTION_NONE);
   m.view.shader_active_id = 1;
   INPUT(&m, RIGHT);
   assert(m.view.shader_fullscreen);
   INPUT(&m, DOWN);
   assert(m.view.shader_selected == 1);
   INPUT(&m, BACK);
   assert(!m.view.shader_fullscreen && m.view.page == HH_QUICK_MENU_PAGE_SHADER);
   INPUT(&m, LEFT);
   assert(!m.view.shader_recommended);
   INPUT(&m, CONFIRM);
   assert(m.view.page == HH_QUICK_MENU_PAGE_SHADER_SCOPE);
   for (i = 0; i < 4; i++) INPUT(&m, DOWN);
   assert(m.view.shader_scope == 4);
   assert(INPUT(&m, CONFIRM) == HH_UI_ACTION_NONE);
   m.view.shader_removable[0] = true;
   assert(INPUT(&m, CONFIRM) == HH_UI_ACTION_SHADER_APPLY);
   hh_quick_menu_action_complete(&m);
   INPUT(&m, BACK);
   assert(m.view.page == HH_QUICK_MENU_PAGE_SHADER);
   for (i = 0; i < 10; i++) INPUT(&m, DOWN);
   assert(m.view.shader_selected == 7 && m.view.shader_first == 4);
   for (i = 0; i < 3; i++)
   {
      assert(hh_quick_menu_compute_layout(sizes[i][0], sizes[i][1], &layout));
      within(layout.shader_panel, layout.viewport);
      within(layout.shader_filter, layout.shader_panel);
      within(layout.shader_fullscreen, layout.shader_panel);
      for (j = 0; j < HH_QUICK_MENU_SHADER_ROWS; j++)
         within(layout.shader_rows[j], layout.shader_panel);
      for (j = 0; j < HH_QUICK_MENU_SHADER_SCOPES; j++)
         within(layout.shader_scopes[j], layout.shader_panel);
      memset(&painter, 0, sizeof(painter));
      memset(&rec, 0, sizeof(rec));
      rec.viewport = layout.viewport;
      painter.userdata = &rec;
      painter.rect = record_rect;
      painter.text = record_text;
      hh_quick_menu_render(&m, sizes[i][0], sizes[i][1], NULL, &painter);
      assert(rec.pause_bars == 0);
      x = layout.shader_rows[0].x + layout.shader_rows[0].width / 2;
      y = layout.shader_rows[0].y + layout.shader_rows[0].height / 2;
      hh_quick_menu_touch(&m, x, y, true, sizes[i][0], sizes[i][1], &input);
      hh_quick_menu_touch(&m, x, y, false, sizes[i][0], sizes[i][1], &input);
      assert(m.view.shader_selected == m.view.shader_first);
      assert(input == HH_QUICK_MENU_INPUT_CONFIRM);
   }
   assert(INPUT(&m, BACK) == HH_UI_ACTION_SHADER_CANCEL);
   assert(INPUT(&m, BACK) == HH_UI_ACTION_NONE);
}

static void test_controls(void)
{
   hh_quick_menu_t m;
   unsigned i;
   select_item(&m, HH_QUICK_MENU_ITEM_CONTROLS);
   assert(INPUT(&m, CONFIRM) == HH_UI_ACTION_CONTROLS_BEGIN);
   hh_quick_menu_action_complete(&m);
   m.view.page = HH_QUICK_MENU_PAGE_CONTROLS;
   m.view.controls_valid = true;
   m.view.controls.player_count = 4;
   m.view.controls.device_count = 2;
   m.view.controls.device_ids[0] = 0;
   m.view.controls.device_ids[1] = 3;
   for (i = 0; i < 3; i++)
      m.view.controls.available[i] = true;
   INPUT(&m, RIGHT);
   assert(m.view.control_player == 1);
   INPUT(&m, LEFT);
   assert(m.view.control_player == 0);
   INPUT(&m, DOWN);
   assert(INPUT(&m, RIGHT) == HH_UI_ACTION_CONTROLS_DEVICE);
   assert(m.view.control_device == 3);
   hh_quick_menu_action_complete(&m);
   INPUT(&m, DOWN);
   INPUT(&m, CONFIRM);
   assert(m.view.page == HH_QUICK_MENU_PAGE_CONTROL_EDIT);
   INPUT(&m, RIGHT);
   INPUT(&m, RIGHT);
   assert(m.view.control_mode == 2 && m.view.control_mask == 1);
   INPUT(&m, DOWN);
   INPUT(&m, RIGHT);
   assert(m.view.control_period == 4);
   INPUT(&m, UP);
   INPUT(&m, RIGHT);
   assert(m.view.control_mode == 3);
   INPUT(&m, DOWN);
   INPUT(&m, DOWN);
   INPUT(&m, DOWN);
   INPUT(&m, CONFIRM);
   INPUT(&m, DOWN);
   INPUT(&m, CONFIRM);
   assert(m.view.control_mask == 7);
   INPUT(&m, DOWN);
   assert(INPUT(&m, RIGHT) == HH_UI_ACTION_NONE);
   assert(INPUT(&m, CONFIRM) == HH_UI_ACTION_CONTROLS_SET);
   assert(INPUT(&m, CONFIRM) == HH_UI_ACTION_NONE);
   hh_quick_menu_action_complete(&m);
   INPUT(&m, BACK);
   assert(m.view.page == HH_QUICK_MENU_PAGE_CONTROLS);
   INPUT(&m, CONFIRM);
   INPUT(&m, BACK);
   assert(m.view.controls.masks[0] == 0);
   m.view.control_selected = 18;
   assert(INPUT(&m, CONFIRM) == HH_UI_ACTION_CONTROLS_SAVE);
   assert(m.view.control_save_scope == 0);
   hh_quick_menu_action_complete(&m);
   m.view.controls_valid = false;
   m.view.control_selected = 2;
   assert(INPUT(&m, CONFIRM) == HH_UI_ACTION_NONE);
   assert(m.view.page == HH_QUICK_MENU_PAGE_CONTROLS);
   puts("PASS: controller player/device selection, turbo, ABC combination, apply/cancel and save");
}

static void test_touch_scroll(void)
{
   static const int sizes[][2] = {{640,480}, {1280,720}, {1920,1080}};
   hh_quick_menu_t m;
   hh_quick_menu_layout_t l;
   hh_quick_menu_input_t input;
   hh_ui_rect_t rect;
   hh_quick_menu_painter_t painter;
   recorder_t rec;
   float x, y, scroll;
   int i, page;
   for (i = 0; i < 3; i++)
   {
      assert(hh_quick_menu_compute_layout(sizes[i][0], sizes[i][1], &l));
      select_item(&m, 0);
      x = l.items[2].x + 120 * l.scale;
      y = l.items[2].y + 108 * l.scale;
      hh_quick_menu_touch(&m, x, y, true, sizes[i][0], sizes[i][1], &input);
      assert(m.view.selected_index == 2);
      assert(hh_quick_menu_item_state(&m, 2) == HH_QUICK_MENU_ITEM_FOCUSED);
      hh_quick_menu_touch(&m, x - 600 * l.scale, y, true,
            sizes[i][0], sizes[i][1], &input);
      assert(m.main_scroll == 2.5f && input == HH_QUICK_MENU_INPUT_NONE);
      hh_quick_menu_update_animation(&m, 1000);
      assert(m.main_scroll == 2.5f);
      hh_quick_menu_touch(&m, x - 600 * l.scale, y, false,
            sizes[i][0], sizes[i][1], &input);
      assert(input == HH_QUICK_MENU_INPUT_NONE && m.state == HH_QUICK_MENU_OPEN);
      hh_quick_menu_update_animation(&m, 2000);
      assert(m.main_scroll == 2.5f);
      for (page = HH_QUICK_MENU_PAGE_SAVE; page <= HH_QUICK_MENU_PAGE_RECENT; page++)
      {
         select_item(&m, 0);
         m.view.page = (hh_quick_menu_page_t)page;
         m.view.recent_count = 200;
         x = l.slot_card.x + 152 * l.scale;
         y = l.slot_card.y + 135 * l.scale;
         hh_quick_menu_touch(&m, x, y, true, sizes[i][0], sizes[i][1], &input);
         hh_quick_menu_touch(&m, x - 800 * l.scale, y, true,
               sizes[i][0], sizes[i][1], &input);
         assert(m.slot_scroll == 2.5f);
         hh_quick_menu_touch(&m, x - 800 * l.scale, y, false,
               sizes[i][0], sizes[i][1], &input);
         assert(input == HH_QUICK_MENU_INPUT_NONE);
         hh_quick_menu_update_animation(&m, 2000);
         assert(m.slot_scroll == 2.5f);
      }
      for (page = 0; page < 3; page++)
      {
         select_item(&m, 0);
         m.view.page = page == 0 ? HH_QUICK_MENU_PAGE_SHADER :
            page == 1 ? HH_QUICK_MENU_PAGE_CONTROLS : HH_QUICK_MENU_PAGE_CONTROL_EDIT;
         m.view.shader_count = HH_QUICK_MENU_SHADER_COUNT;
         memset(m.view.controls.available, 1, sizeof(m.view.controls.available));
         rect = page == 0 ? l.shader_rows[0] : l.control_rows[0];
         x = rect.x + rect.width / 2;
         y = rect.y + rect.height / 2;
         scroll = (page == 0 ? 76 : 70) * l.scale;
         hh_quick_menu_touch(&m, x, y, true, sizes[i][0], sizes[i][1], &input);
         hh_quick_menu_touch(&m, x, y - 2.5f * scroll, true,
               sizes[i][0], sizes[i][1], &input);
         assert((page == 0 ? m.view.shader_first : m.view.control_first) == 2);
         assert(m.list_scroll_fraction > 0.49f && m.list_scroll_fraction < 0.51f);
         hh_quick_menu_touch(&m, x, y - 2.5f * scroll, false,
               sizes[i][0], sizes[i][1], &input);
         assert(input == HH_QUICK_MENU_INPUT_NONE);
         assert(hh_quick_menu_list_row_bounds(&m, &l, 1, &rect));
         memset(&painter, 0, sizeof(painter));
         memset(&rec, 0, sizeof(rec));
         rec.viewport = l.viewport;
         painter.userdata = &rec;
         painter.rect = record_rect;
         painter.text = record_text;
         hh_quick_menu_render(&m, sizes[i][0], sizes[i][1], NULL, &painter);
         y = rect.y + rect.height / 2;
         hh_quick_menu_touch(&m, x, y, true, sizes[i][0], sizes[i][1], &input);
         assert((page == 0 ? m.view.shader_selected : m.view.control_selected) == 3);
         hh_quick_menu_touch(&m, x, y, false, sizes[i][0], sizes[i][1], &input);
         assert(input == HH_QUICK_MENU_INPUT_CONFIRM);
         hh_quick_menu_touch(&m, x, y, true, sizes[i][0], sizes[i][1], &input);
         hh_quick_menu_touch(&m, x, y + 4000 * l.scale, false,
               sizes[i][0], sizes[i][1], &input);
         assert((page == 0 ? m.view.shader_first : m.view.control_first) == 0);
         assert(m.list_scroll_fraction == 0 && input == HH_QUICK_MENU_INPUT_NONE);
      }
   }
   puts("PASS: touch highlights on press, scrolls continuously across subpages, clamps and never confirms a drag (3 resolutions)");
}

int main(void)
{
   test_touch_scroll();
   test_controls();
   test_main();
   test_dialogs();
   test_slots();
   test_capabilities_feedback();
   test_duplicate_all_actions();
   test_slot_touch();
   test_slot_animation();
   test_main_animation();
   test_main_touch();
   test_main_overflow();
   test_layout_render();
   test_recent();
   test_button_repeat();
   test_recent_video_priority();
   test_recent_carousel();
   test_shaders();
   puts("PASS: navigation, actions, dialogs, slots, feedback, duplicate protection, layout/render (3 resolutions)");
   return 0;
}
