
#include "settings.h"

#include "ResourceManager.h"
#include "common/misc.h"
#include "Utils/Logger.h"
#include "input.h"

#include "libretro/LibretroManager.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

const uint32_t SETTINGS_VERSION = (('7' << 24) + ('S' << 16) + ('X' << 8) + 'N'); // serves as both a version and magic

Settings normal_settings;
Settings *settings = &normal_settings;

static bool tryload(Settings *setfile)
{
  LibretroManager::getInstance()->getSettings(setfile);
  // TODO: RETRO_ENVIRONMENT_GET_LANGUAGE
  // std::vector<std::string> langs = ResourceManager::getInstance()->languages();
  // bool found                     = false;
  // for (auto &l : langs)
  // {
  //   if (strcmp(settings->language, l.c_str()) == 0)
  //   {
  //     found = true;
  //     break;
  //   }
  // }
  // if (!found)
  // {
  //   memset(setfile->language, 0, 256);
  //   strncpy(setfile->language, "english", 255);
  // }
  return 1;
}

bool settings_load(Settings *setfile)
{
  if (!setfile)
    setfile = &normal_settings;

  {
    LOG_INFO("No saved settings; using defaults.");

    memset(setfile, 0, sizeof(Settings));
    setfile->resolution     = -1; // 640x480 Windowed, should be safe value
    setfile->last_save_slot = 0;
    setfile->fullscreen     = false;

    setfile->sound_enabled = true;
    setfile->music_enabled = 1; // both Boss and Regular music
    setfile->new_music     = 0;
    setfile->rumble        = false;
    setfile->sfx_volume = 100;
    setfile->music_volume = 100;
    setfile->music_interpolation = 0;
    memset(setfile->language, 0, 256);
    strncpy(setfile->language, "english", 255);

    //return 1;
  }
  tryload(settings);

  return 0;
}

/*
void c------------------------------() {}
*/

bool settings_save(Settings *setfile)
{
  // Saving to the frontend is not possible as far as I know.
  return 0;
}
