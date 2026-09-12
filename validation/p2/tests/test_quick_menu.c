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
      "继续游戏", "保存进度", "读取进度", "重新开始", "高级菜单", "退出游戏"
   };
   select_item(&m, 0);
   assert(m.view.item_count == 6 && m.view.selected_index == 0);
   for (i = 0; i < 6; i++)
   {
      assert(m.view.items[i].id == (hh_quick_menu_item_id_t)i);
      assert(strcmp(m.view.items[i].label, labels[i]) == 0);
      assert(hh_quick_menu_item_state(&m, i) == (i == 0
               ? HH_QUICK_MENU_ITEM_FOCUSED : HH_QUICK_MENU_ITEM_NORMAL));
   }
   INPUT(&m, UP);
   assert(m.view.selected_index == 5);
   INPUT(&m, DOWN);
   assert(m.view.selected_index == 0);
   INPUT(&m, LEFT);
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
   select_item(&m, 4);
   assert(INPUT(&m, CONFIRM) == HH_UI_ACTION_ADVANCED_MENU);
}

static void test_dialogs(void)
{
   hh_quick_menu_t m;
   int i;
   for (i = 3; i <= 5; i += 2)
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
      assert(INPUT(&m, CONFIRM) == (i == 3 ? HH_UI_ACTION_RESET : HH_UI_ACTION_EXIT));
      assert(m.state == HH_QUICK_MENU_ACTION_PENDING);
      assert(INPUT(&m, CONFIRM) == HH_UI_ACTION_NONE);
   }
   select_item(&m, 3);
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
   for (i = 1; i < 5; i++)
   {
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
   for (i = 0; i < 6; i++)
   {
      select_item(&m, i);
      if (i == 2) m.view.slots[0].occupied = true;
      INPUT(&m, CONFIRM);
      if (i == 1 || i == 2) INPUT(&m, CONFIRM);
      if (i == 3 || i == 5)
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
   hh_ui_rect_t viewport;
   int texts, rects, previews, placeholders, pending, disabled;
   bool preview_result;
} recorder_t;

static void record_rect(void *data, hh_ui_rect_t r, unsigned long fill,
      unsigned long border, float radius, float stroke)
{
   recorder_t *rec = (recorder_t *)data;
   (void)fill; (void)border; (void)radius; (void)stroke;
   within(r, rec->viewport);
   rec->rects++;
}

static void record_text(void *data, hh_ui_rect_t r, const char *text,
      unsigned long color, float size, bool bold)
{
   recorder_t *rec = (recorder_t *)data;
   (void)color; (void)bold;
   within(r, rec->viewport);
   assert(size > 0 && size <= r.height);
   rec->texts++;
   if (strcmp(text, "No Preview") == 0) rec->placeholders++;
   if (strstr(text, "处理中")) rec->pending++;
   if (strcmp(text, "不可用") == 0) rec->disabled++;
}

static bool record_preview(void *data, hh_ui_rect_t r, int slot)
{
   recorder_t *rec = (recorder_t *)data;
   within(r, rec->viewport);
   assert(slot == 0);
   rec->previews++;
   return rec->preview_result;
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
   painter.preview = record_preview;
   for (i = 0; i < 3; i++)
   {
      assert(hh_quick_menu_compute_layout(sizes[i][0], sizes[i][1], &l));
      within(l.panel, l.viewport);
      assert(l.panel.x >= 24 * l.scale && l.panel.y >= 24 * l.scale);
      within(l.header, l.panel);
      within(l.dialog, l.panel);
      within(l.footer, l.panel);
      within(l.feedback, l.panel);
      within(l.slot_preview, l.slot_card);
      within(l.slot_info, l.slot_card);
      within(l.slot_card, l.panel);
      for (j = 0; j < 6; j++)
      {
         within(l.items[j], l.panel);
         if (j) assert(l.items[j-1].y + l.items[j-1].height < l.items[j].y);
      }
      assert(l.items[5].y + l.items[5].height < l.feedback.y);
      assert(l.feedback.y + l.feedback.height < l.footer.y);
      for (j = 0; j < 10; j++) within(l.slot_numbers[j], l.panel);
      for (j = 0; j < 2; j++) within(l.dialog_buttons[j], l.dialog);
      memset(&rec, 0, sizeof(rec));
      rec.viewport = l.viewport;
      mock_quick_menu(&m);
      hh_quick_menu_render(&m, sizes[i][0], sizes[i][1], NULL, &painter);
      assert(rec.disabled == 1 && rec.texts > 6);
      INPUT(&m, DOWN);
      INPUT(&m, CONFIRM);
      hh_quick_menu_render(&m, sizes[i][0], sizes[i][1], NULL, &painter);
      assert(rec.previews == 1 && rec.placeholders == 1);
      rec.preview_result = true;
      hh_quick_menu_render(&m, sizes[i][0], sizes[i][1], NULL, &painter);
      assert(rec.previews == 2 && rec.placeholders == 1);
      painter.preview = NULL;
      hh_quick_menu_render(&m, sizes[i][0], sizes[i][1], NULL, &painter);
      assert(rec.previews == 2 && rec.placeholders == 2);
      painter.preview = record_preview;
      m.view.capabilities.screenshot_enabled = false;
      hh_quick_menu_render(&m, sizes[i][0], sizes[i][1], NULL, &painter);
      assert(rec.previews == 2 && rec.placeholders == 3);
      INPUT(&m, CONFIRM);
      hh_quick_menu_render(&m, sizes[i][0], sizes[i][1], NULL, &painter);
      assert(rec.pending > 0);
      hh_quick_menu_action_result(&m, HH_QUICK_MENU_ACTION_ERROR, "保存失败");
      hh_quick_menu_render(&m, sizes[i][0], sizes[i][1], NULL, &painter);
      select_item(&m, 3);
      INPUT(&m, CONFIRM);
      hh_quick_menu_render(&m, sizes[i][0], sizes[i][1], NULL, &painter);
      hh_quick_menu_back(&m);
      hh_quick_menu_close(&m);
      hh_quick_menu_finish_close(&m);
      j = rec.texts;
      hh_quick_menu_render(&m, sizes[i][0], sizes[i][1], NULL, &painter);
      assert(rec.texts == j);
   }
   assert(!hh_quick_menu_compute_layout(0, 480, &l));
   assert(!hh_quick_menu_compute_layout(640, -1, &l));
   assert(!hh_quick_menu_compute_layout(640, 480, NULL));
}

int main(void)
{
   test_main();
   test_dialogs();
   test_slots();
   test_capabilities_feedback();
   test_duplicate_all_actions();
   test_layout_render();
   puts("PASS: navigation, actions, dialogs, slots, feedback, duplicate protection, layout/render (3 resolutions)");
   return 0;
}
