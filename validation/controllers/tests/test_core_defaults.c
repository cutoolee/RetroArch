#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "core_option_manager.h"
#include "msg_hash.h"

const char *msg_hash_to_str(enum msg_hash_enums key)
{ (void)key; return "label"; }

config_file_t *config_file_new_from_path_to_string(const char *path)
{
   char contents[256];
   size_t size;
   FILE *file = fopen(path, "r");
   if (!file)
      return NULL;
   size = fread(contents, 1, sizeof(contents) - 1, file);
   contents[size] = '\0';
   fclose(file);
   return config_file_new_from_string(contents, path);
}

static void check_value(core_option_manager_t *manager, const char *expected)
{
   assert(manager);
   assert(strcmp(core_option_manager_get_val(manager, 0), expected) == 0);
   assert(strcmp(core_option_manager_get_val(manager, 1), "original") == 0);
   core_option_manager_free(manager);
}

int main(void)
{
   const char *path = "output.opt";
   const char *source = "source.opt";
   static struct retro_core_option_v2_definition definitions[] = {
      { "fceumm_turbo_enable", "Turbo", NULL, NULL, NULL, NULL,
        {{"None", NULL}, {"Player 1", NULL}, {"Player 2", NULL}, {"Both", NULL}, {NULL, NULL}}, "None" },
      { "unrelated", "Unrelated", NULL, NULL, NULL, NULL,
        {{"original", NULL}, {"other", NULL}, {NULL, NULL}}, "original" },
      {0}
   };
   struct retro_core_options_v2 options = {NULL, definitions};
   static struct retro_variable variables[] = {
      {"fceumm_turbo_enable", "Turbo; None|Player 1|Player 2|Both"},
      {"unrelated", "Unrelated; original|other"},
      {NULL, NULL}
   };
   FILE *file;
   const char *expected = "None";
#ifdef HAVE_GAMEGO_PRODUCT
   expected = "Both";
#endif
   remove(path);
   remove(source);
   check_value(core_option_manager_new(path, NULL, &options, false), expected);
   check_value(core_option_manager_new_vars(path, NULL, variables), expected);
   file = fopen(path, "w");
   assert(file);
   fputs("fceumm_turbo_enable = \"None\"\n", file);
   fclose(file);
   check_value(core_option_manager_new(path, NULL, &options, false), "None");
   check_value(core_option_manager_new_vars(path, NULL, variables), "None");
   file = fopen(source, "w");
   assert(file);
   fputs("fceumm_turbo_enable = \"Player 2\"\n", file);
   fclose(file);
   check_value(core_option_manager_new(path, source, &options, false), "Player 2");
   check_value(core_option_manager_new_vars(path, source, variables), "Player 2");
   remove(path);
   remove(source);
   puts("PASS: product turbo default, stored disable/source priority and unrelated options");
   return 0;
}
