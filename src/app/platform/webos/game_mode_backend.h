#pragma once
#include <stdbool.h>
char *webos_game_mode_get_setting(const char *category, const char *key);
bool webos_game_mode_set_setting(const char *category, const char *key, const char *value);
